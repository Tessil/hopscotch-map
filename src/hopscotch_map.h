/**
 * MIT License
 * 
 * Copyright (c) 2016 Tessil
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __HOPSCOTCH_MAP_H__
#define __HOPSCOTCH_MAP_H__


#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <list>
#include <memory>
#include <ratio>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>


namespace tsl {

namespace detail {
/**
 * Common class used by hopscotch_map and hopscotch_set.
 * 
 * ValueType is what will be stored by hopscotch_hash (usually std::pair<Key, T> for map and Key for set).
 * 
 * KeySelect should be a FunctionObject which take ValueType in parameter and return a reference to the key.
 * 
 * ValueSelect should be a FunctionObject which take ValueType in parameter and return a reference to the value. 
 * ValueSelect should be void if there is no value (in set for example).
 */
template<class ValueType,
         class KeySelect,
         class ValueSelect,
         class Hash,
         class KeyEqual,
         class Allocator,
         unsigned int NeighborhoodSize,
         class GrowthFactor>
class hopscotch_hash {
private:    
    using Key = typename KeySelect::key_type;
    
private:
    /*
     * smallest_type_for_min_bits::type returns the smallest type that can fit MinBits.
     */
    static const size_t SMALLEST_TYPE_MAX_BITS_SUPPORTED = 64;
    template<unsigned int MinBits, typename Enable = void>
    class smallest_type_for_min_bits {
    };

    template<unsigned int MinBits>
    class smallest_type_for_min_bits<MinBits, typename std::enable_if<(MinBits > 0) && (MinBits <= 8)>::type> {
    public:
        using type = std::uint8_t;
    };

    template<unsigned int MinBits>
    class smallest_type_for_min_bits<MinBits, typename std::enable_if<(MinBits > 8) && (MinBits <= 16)>::type> {
    public:
        using type = std::uint16_t;
    };

    template<unsigned int MinBits>
    class smallest_type_for_min_bits<MinBits, typename std::enable_if<(MinBits > 16) && (MinBits <= 32)>::type> {
    public:
        using type = std::uint32_t;
    };

    template<unsigned int MinBits>
    class smallest_type_for_min_bits<MinBits, typename std::enable_if<(MinBits > 32) && (MinBits <= 64)>::type> {
    public:
        using type = std::uint64_t;
    };
    
private:
    static const size_t NB_RESERVED_BITS_IN_NEIGHBORHOOD = 2; 
    static const size_t MAX_NEIGHBORHOOD_SIZE = SMALLEST_TYPE_MAX_BITS_SUPPORTED - NB_RESERVED_BITS_IN_NEIGHBORHOOD; 
    
    /*
     * NeighborhoodSize need to be between 0 and MAX_NEIGHBORHOOD_SIZE.
     */
    static_assert(NeighborhoodSize > 0, "NeighborhoodSize should be > 0.");
    static_assert(NeighborhoodSize <= MAX_NEIGHBORHOOD_SIZE, "NeighborhoodSize should be <= 62.");
    
    
    using neighborhood_bitmap = typename smallest_type_for_min_bits<NeighborhoodSize + NB_RESERVED_BITS_IN_NEIGHBORHOOD>::type;
    
public:
    template<bool is_const>
    class hopscotch_iterator;
    
    using key_type = Key;
    using value_type = ValueType;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = hopscotch_iterator<false>;
    using const_iterator = hopscotch_iterator<true>;
    
private:
    /*
     * Each bucket stores two elements:
     * - An aligned storage to store a value_type object with placement-new
     * - An unsigned integer of type neighborhood_bitmap used to tell us which buckets in the neighborhood of the current bucket
     *   contain a value with a hash belonging to the current bucket. 
     * 
     * For a bucket 'b' a bit 'i' (counting from 0 and from the least significant bit to the most significant) 
     * set to 1 means that the bucket 'b+i' contains a value with a hash belonging to bucket 'b'.
     * The bits used for that, start from the third least significant bit.
     * 
     * The least significant bit is set to 1 if there is a value in the bucket storage.
     * The second least significant bit is set to 1 if there is an overflow. More than NeighborhoodSize values give the same hash,
     * all overflow values are stored in the m_overflow_elements list of the map.
     */
    class hopscotch_bucket {
    public:
        hopscotch_bucket() noexcept : m_neighborhood_infos(0) {
            assert(is_empty());
        }
        
        hopscotch_bucket(const hopscotch_bucket& bucket) noexcept(std::is_nothrow_copy_constructible<value_type>::value) : 
                    m_neighborhood_infos(0) 
        {
            if(!bucket.is_empty()) {
                ::new (static_cast<void*>(std::addressof(m_value_type))) value_type(bucket.get_value_type());
            }
            
            m_neighborhood_infos = bucket.m_neighborhood_infos;
        }
        
        hopscotch_bucket(hopscotch_bucket&& bucket) noexcept(std::is_nothrow_move_constructible<value_type>::value) : 
                    m_neighborhood_infos(0) 
        {
            if(!bucket.is_empty()) {
                ::new (static_cast<void*>(std::addressof(m_value_type))) value_type(std::move(bucket.get_value_type()));
            }
            
            m_neighborhood_infos = bucket.m_neighborhood_infos;
        }
        
        hopscotch_bucket& operator=(const hopscotch_bucket& bucket) noexcept(std::is_nothrow_copy_constructible<value_type>::value) {
            if(this != &bucket) {
                if(!is_empty()) {
                    destroy_value_type();
                }
                
                if(!bucket.is_empty()) {
                    ::new (static_cast<void*>(std::addressof(m_value_type))) value_type(bucket.get_value_type());
                }
                
                m_neighborhood_infos = bucket.m_neighborhood_infos;
            }
            
            return *this;
        }
        
        hopscotch_bucket& operator=(hopscotch_bucket&& bucket) noexcept(std::is_nothrow_move_constructible<value_type>::value) {
            if(!is_empty()) {
                destroy_value_type();
            }
            
            if(!bucket.is_empty()) {
                ::new (static_cast<void*>(std::addressof(m_value_type))) value_type(std::move(bucket.get_value_type()));
            }
            
            m_neighborhood_infos = bucket.m_neighborhood_infos;
            
            return *this;
        }
        
        ~hopscotch_bucket() noexcept {
            if(!is_empty()) {
                destroy_value_type();
            }
            
            m_neighborhood_infos = 0;
        }
        
        neighborhood_bitmap get_neighborhood_infos() const noexcept {
            return static_cast<neighborhood_bitmap>(m_neighborhood_infos >> NB_RESERVED_BITS_IN_NEIGHBORHOOD);
        }
        
        void set_overflow(bool has_overflow) noexcept {
            if(has_overflow) {
                m_neighborhood_infos = static_cast<neighborhood_bitmap>(m_neighborhood_infos | 2);
            }
            else {
                m_neighborhood_infos = static_cast<neighborhood_bitmap>(m_neighborhood_infos & ~2);
            }
        }
        
        bool has_overflow() const noexcept {
            return (m_neighborhood_infos & 2) != 0;
        }
        
        bool is_empty() const noexcept {
            return (m_neighborhood_infos & 1) == 0;
        }
        
        template<typename P>
        void set_value_type(P&& value) {
            if(!is_empty()) {
                destroy_value_type();
                ::new (static_cast<void*>(std::addressof(m_value_type))) value_type(std::forward<P>(value));
            }
            else {
                ::new (static_cast<void*>(std::addressof(m_value_type))) value_type(std::forward<P>(value));
                set_is_empty(false);
            }
        }
        
        void remove_value_type() noexcept {
            if(!is_empty()) {
                destroy_value_type();
                set_is_empty(true);
            }
        }
        
        void toggle_neighbor_presence(std::size_t ineighbor) noexcept {
            assert(ineighbor <= NeighborhoodSize);
            m_neighborhood_infos = static_cast<neighborhood_bitmap>(m_neighborhood_infos ^ (1ull << (ineighbor + NB_RESERVED_BITS_IN_NEIGHBORHOOD)));
        }
        
        bool check_neighbor_presence(std::size_t ineighbor) const noexcept {
            assert(ineighbor <= NeighborhoodSize);
            if(((m_neighborhood_infos >> (ineighbor + NB_RESERVED_BITS_IN_NEIGHBORHOOD)) & 1) == 1) {
                return true;
            }
            
            return false;
        }
        
        value_type& get_value_type() noexcept {
            assert(!is_empty());
            return *reinterpret_cast<value_type*>(std::addressof(m_value_type));
        }
        
        const value_type& get_value_type() const noexcept {
            assert(!is_empty());
            return *reinterpret_cast<const value_type*>(std::addressof(m_value_type));
        }
        
        void swap_value_type_into_empty_bucket(hopscotch_bucket& empty_bucket) {
            assert(empty_bucket.is_empty());
            if(!is_empty()) {
                ::new (static_cast<void*>(std::addressof(empty_bucket.m_value_type))) value_type(std::move(get_value_type()));
                empty_bucket.set_is_empty(false);
                
                destroy_value_type();
                set_is_empty(true);
            }
        }
        
        void clear() noexcept {
            if(!is_empty()) {
                destroy_value_type();
            }
            
            m_neighborhood_infos = 0;
            assert(is_empty());
        }
        
    private:
        void set_is_empty(bool is_empty) noexcept {
            if(is_empty) {
                m_neighborhood_infos = static_cast<neighborhood_bitmap>(m_neighborhood_infos & ~1);
            }
            else {
                m_neighborhood_infos = static_cast<neighborhood_bitmap>(m_neighborhood_infos | 1);
            }
        }
        
        void destroy_value_type() noexcept {
            try {
                assert(!is_empty());
                
                get_value_type().~value_type();
            }
            catch(...) {
                std::terminate();
            }
        }
        
    private:
        using storage = typename std::aligned_storage<sizeof(value_type), alignof(value_type)>::type;
        
        neighborhood_bitmap m_neighborhood_infos;
        storage m_value_type;
    };
    
    
    using buckets_allocator = typename std::allocator_traits<allocator_type>::template rebind_alloc<hopscotch_bucket>;
    using overflow_elements_allocator = typename std::allocator_traits<allocator_type>::template rebind_alloc<value_type>;  
    
    using iterator_buckets = typename std::vector<hopscotch_bucket, buckets_allocator>::iterator; 
    using const_iterator_buckets = typename std::vector<hopscotch_bucket, buckets_allocator>::const_iterator;
    
    using iterator_overflow = typename std::list<value_type, overflow_elements_allocator>::iterator; 
    using const_iterator_overflow = typename std::list<value_type, overflow_elements_allocator>::const_iterator; 
    
public:    
    template<bool is_const>
    class hopscotch_iterator {
        friend class hopscotch_hash;
    private:
        using iterator_bucket = typename std::conditional<is_const, 
                                                            typename hopscotch_hash::const_iterator_buckets, 
                                                            typename hopscotch_hash::iterator_buckets>::type;
        using iterator_overflow = typename std::conditional<is_const, 
                                                            typename hopscotch_hash::const_iterator_overflow, 
                                                            typename hopscotch_hash::iterator_overflow>::type;
    
        
        hopscotch_iterator(iterator_bucket buckets_iterator, iterator_bucket buckets_end_iterator, 
                           iterator_overflow overflow_iterator) noexcept : 
            m_buckets_iterator(buckets_iterator), m_buckets_end_iterator(buckets_end_iterator),
            m_overflow_iterator(overflow_iterator)
        {
        }
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const typename hopscotch_hash::value_type;
        using difference_type = std::ptrdiff_t;
        using reference = value_type&;
        using pointer = value_type*;
        
        
        hopscotch_iterator() noexcept {
        }
        
        hopscotch_iterator(const hopscotch_iterator<false>& other) noexcept :
            m_buckets_iterator(other.m_buckets_iterator), m_buckets_end_iterator(other.m_buckets_end_iterator),
            m_overflow_iterator(other.m_overflow_iterator)
        {
        }
        
        const typename hopscotch_hash::key_type& key() const {
            if(m_buckets_iterator != m_buckets_end_iterator) {
                return KeySelect()(m_buckets_iterator->get_value_type());
            }
            
            return KeySelect()(*m_overflow_iterator);
        }

        template<class U = ValueSelect, typename std::enable_if<!std::is_same<U, void>::value>::type* = nullptr>
        typename std::conditional<is_const, const typename U::value_type&, typename U::value_type&>::type value() const {
            if(m_buckets_iterator != m_buckets_end_iterator) {
                return m_buckets_iterator->get_value_type().second;
            }
            
            return m_overflow_iterator->second;
        }
        
        reference operator*() const { 
            if(m_buckets_iterator != m_buckets_end_iterator) {
                return m_buckets_iterator->get_value_type();
            }
            
            return *m_overflow_iterator;
        }
        
        pointer operator->() const { 
            if(m_buckets_iterator != m_buckets_end_iterator) {
                return std::addressof(m_buckets_iterator->get_value_type()); 
            }
            
            return std::addressof(*m_overflow_iterator); 
        }
        
        hopscotch_iterator& operator++() {
            if(m_buckets_iterator == m_buckets_end_iterator) {
                ++m_overflow_iterator;
                return *this;
            }
            
            do {
                ++m_buckets_iterator;
            } while(m_buckets_iterator != m_buckets_end_iterator && m_buckets_iterator->is_empty());
            
            return *this; 
        }
        hopscotch_iterator operator++(int) {
            hopscotch_iterator tmp(*this);
            ++*this;
            
            return tmp;
        }
        
        friend bool operator==(const hopscotch_iterator& lhs, const hopscotch_iterator& rhs) { 
            return lhs.m_buckets_iterator == rhs.m_buckets_iterator && 
                   lhs.m_buckets_end_iterator == rhs.m_buckets_end_iterator &&
                   lhs.m_overflow_iterator == rhs.m_overflow_iterator; 
        }
        
        friend bool operator!=(const hopscotch_iterator& lhs, const hopscotch_iterator& rhs) { 
            return !(lhs == rhs); 
        }
    private:
        iterator_bucket m_buckets_iterator;
        iterator_bucket m_buckets_end_iterator;
        iterator_overflow m_overflow_iterator;
    };
    

    
public:
    hopscotch_hash(size_type bucket_count, 
                  const Hash& hash,
                  const KeyEqual& equal,
                  const Allocator& alloc,
                  float max_load_factor) :  m_buckets(alloc), 
                                            m_overflow_elements(alloc),
                                            m_nb_elements(0), 
                                            m_max_probes_for_empty_bucket(DEFAULT_MAX_PROBES_FOR_EMPTY_BUCKET),
                                            m_hash(hash), m_key_equal(equal)
    {
        const size_type init_size = (USE_POWER_OF_TWO_MOD?round_up_to_power_of_two(bucket_count):bucket_count) + 
                                     NeighborhoodSize - 1;
        const size_type min_size = MIN_BUCKETS_SIZE + NeighborhoodSize - 1;
        
        m_buckets.resize(std::max(min_size, init_size));
        this->max_load_factor(max_load_factor);
    }
    
    allocator_type get_allocator() const {
        return m_buckets.get_allocator();
    }
    
    
    /*
     * Iterators
     */
    iterator begin() noexcept {
        return iterator(get_first_non_empty_buckets_iterator(), m_buckets.end(), m_overflow_elements.begin());
    }
    
    const_iterator begin() const noexcept {
        return cbegin();
    }
    
    const_iterator cbegin() const noexcept {
        return const_iterator(get_first_non_empty_buckets_iterator(), m_buckets.cend(), m_overflow_elements.cbegin());
    }
    
    iterator end() noexcept {
        return iterator(m_buckets.end(), m_buckets.end(), m_overflow_elements.end());
    }
    
    const_iterator end() const noexcept {
        return cend();
    }
    
    const_iterator cend() const noexcept {
        return const_iterator(m_buckets.cend(), m_buckets.cend(), m_overflow_elements.cend());
    }
    
    
    /*
     * Capacity
     */
    bool empty() const noexcept {
        return m_nb_elements == 0;
    }
    
    size_type size() const noexcept {
        return m_nb_elements;
    }
    
    size_type max_size() const noexcept {
        return m_buckets.max_size();
    }
    
    /*
     * Modifiers
     */
    void clear() noexcept {
        for(auto& bucket : m_buckets) {
            bucket.clear();
        }
        
        m_overflow_elements.clear();
        m_nb_elements = 0;
    }
    
    std::pair<iterator, bool> insert(const value_type& value) {
        return insert_internal(value);
    }
    
    std::pair<iterator, bool> insert(value_type&& value) {
        return insert_internal(std::move(value));
    }
    
    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        for(; first != last; ++first) {
            insert(*first);
        }
    }
    
    template<class... Args>
    std::pair<iterator,bool> emplace(Args&&... args) {
        return insert(value_type(std::forward<Args>(args)...));
    }
    
    template <class... Args>
    std::pair<iterator, bool> try_emplace(const key_type& k, Args&&... args) {
        return try_emplace_internal(k, std::forward<Args>(args)...);
    }
    
    template <class... Args>
    std::pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
        return try_emplace_internal(std::move(k), std::forward<Args>(args)...);
    }
    
    iterator erase(const_iterator pos) {
        const std::size_t ibucket_for_hash = bucket_for_hash(m_hash(pos.key()));
        
        if(pos.m_buckets_iterator != pos.m_buckets_end_iterator) {
            auto it_bucket = m_buckets.begin() + std::distance(m_buckets.cbegin(), pos.m_buckets_iterator);
            erase_from_bucket(it_bucket, ibucket_for_hash);
            
            return ++iterator(it_bucket, m_buckets.end(), m_overflow_elements.begin()); 
        }
        else {
            auto it_next_overflow = erase_from_overflow(pos.m_overflow_iterator, ibucket_for_hash);
            return iterator(m_buckets.end(), m_buckets.end(), it_next_overflow);
        }
    }
    
    
    
    iterator erase(const_iterator first, const_iterator last) {
        if(first == last) {
            if(first.m_buckets_iterator != first.m_buckets_end_iterator) {
                // Get a non-const iterator
                auto it = m_buckets.begin() + std::distance(m_buckets.cbegin(), first.m_buckets_iterator);
                return iterator(it, m_buckets.end(), m_overflow_elements.begin());
            }
            else {
                // Get a non-const iterator
                auto it = m_overflow_elements.erase(first.m_overflow_iterator, first.m_overflow_iterator);
                return iterator(m_buckets.end(), m_buckets.end(), it);
            }
        }
        
        auto to_delete = erase(first);
        while(to_delete != last) {
            to_delete = erase(to_delete);
        }
        
        return to_delete;
    }
    
    size_type erase(const key_type& key) {
        const std::size_t ibucket_for_hash = bucket_for_hash(m_hash(key));
        
        auto it_find = find_in_buckets(key, m_buckets.begin() + ibucket_for_hash);
        if(it_find != m_buckets.end()) {
            erase_from_bucket(it_find, ibucket_for_hash);

            return 1;
        }
        
        if(m_buckets[ibucket_for_hash].has_overflow()) {
            for(auto it_overflow = m_overflow_elements.begin(); it_overflow != m_overflow_elements.end(); ++it_overflow) {
                if(m_key_equal(key, KeySelect()(*it_overflow))) {
                    erase_from_overflow(it_overflow, ibucket_for_hash);
                    
                    return 1;
                }
            }
        }
        
        return 0;
    }
    
    
    
    /*
     * Lookup
     */    
    size_type count(const Key& key) const {
        if(find(key) == end()) {
            return 0;
        }
        else {
            return 1;
        }
    }
    
    iterator find(const Key& key) {
        const std::size_t ibucket_for_hash =  bucket_for_hash(m_hash(key));
        
        return find_internal(key, m_buckets.begin() + ibucket_for_hash);
    }
    
    const_iterator find(const Key& key) const {
        const std::size_t ibucket_for_hash =  bucket_for_hash(m_hash(key));
        
        return find_internal(key, m_buckets.begin() + ibucket_for_hash);
    }
    
    /*
     * Bucket interface 
     */
    size_type bucket_count() const {
        /*
         * So that the last bucket can have NeighborhoodSize neighbors, the size of the bucket array is a little
         * bigger than the real number of buckets. We could use some of the buckets at the beginning, but
         * it is easier this way and we avoid weird behaviour with iterators.
         */
        return m_buckets.size() - NeighborhoodSize + 1; 
    }
    
    
    /*
     *  Hash policy 
     */
    float load_factor() const {
        return (1.0f*m_nb_elements)/bucket_count();
    }
    
    float max_load_factor() const {
        return m_max_load_factor;
    }
    
    void max_load_factor(float ml) {
        m_max_load_factor = ml;
        m_load_threshold = static_cast<size_type>(bucket_count()*m_max_load_factor);
    }
    
    void rehash(size_type count) {
        count = std::max(count, static_cast<size_type>(std::ceil(size()/max_load_factor())));
        rehash_internal(count);
    }
    
    void reserve(size_type count) {
        rehash(static_cast<size_type>(std::ceil(count/max_load_factor())));
    }
    
    
    /*
     * Observers
     */
    hasher hash_function() const {
        return m_hash;
    }
    
    key_equal key_eq() const {
        return m_key_equal;
    }
    
    
    /*
     * Other
     */
    
    /*
     * Avoid the creation of an iterator to just get the value for operator[] and at() in maps. Faster this way.
     *
     * Return null if no value for key (TODO use std::optional when available).
     */
    template<class U = ValueSelect, typename std::enable_if<!std::is_same<U, void>::value>::type* = nullptr>
    const typename U::value_type* find_value(const Key& key) const {
        const std::size_t ibucket_for_hash = bucket_for_hash(m_hash(key));
        
        auto it_find = find_in_buckets(key, m_buckets.begin() + ibucket_for_hash);
        if(it_find != m_buckets.end()) {
            return &ValueSelect()(it_find->get_value_type());
        }
        
        if(m_buckets[ibucket_for_hash].has_overflow()) {
            for(auto it_overflow = m_overflow_elements.begin(); it_overflow != m_overflow_elements.end(); ++it_overflow) {
                if(m_key_equal(key, KeySelect()(*it_overflow))) {
                    return &ValueSelect()(*it_overflow);
                }
            }
        }
        
        return nullptr;
    }
    
    std::size_t max_probes_for_empty_bucket() const {
        return m_max_probes_for_empty_bucket;
    }
    
    void max_probes_for_empty_bucket(std::size_t max_probes) {
        m_max_probes_for_empty_bucket = std::max(std::size_t(1), max_probes);
    }
    
private:
    std::size_t bucket_for_hash(std::size_t hash) const {
        assert(bucket_count() > 0);
        if(USE_POWER_OF_TWO_MOD) {
            assert(is_power_of_two(bucket_count()));
            return hash & (bucket_count() - 1);
        }
        else {
            return hash % bucket_count();
        }
    }
    
    std::size_t bucket_for_hash(std::size_t hash, std::size_t nb_buckets) const {
        assert(nb_buckets > 0);
        if(USE_POWER_OF_TWO_MOD) {
            assert(is_power_of_two(nb_buckets));
            return hash & (nb_buckets - 1);
        }
        else {
            return hash % nb_buckets;
        }
    }
    
    template<typename U = value_type, typename std::enable_if<!std::is_nothrow_move_constructible<U>::value>::type* = nullptr>
    void rehash_internal(size_type count) {
        hopscotch_hash tmp_map(count, m_hash, m_key_equal, get_allocator(), m_max_load_factor);
        
        for(const auto& value : *this) {
            const std::size_t ibucket_for_hash = tmp_map.bucket_for_hash(tmp_map.m_hash(KeySelect()(value)));
            tmp_map.insert_internal(value, ibucket_for_hash);
        }
        
        std::swap(*this, tmp_map);
    }   
    
    template<typename U = value_type, typename std::enable_if<std::is_nothrow_move_constructible<U>::value>::type* = nullptr>
    void rehash_internal(size_type count) {
        /*
         * Little more complicate here. We want to avoid any exception so we can't just insert elements
         * in the map trough insert_internal. The method may throw even if value_type is nothrow_move_constructible
         * due to the m_overflow_elements.push_back in the method.
         * 
         * So we first move safely all the elements from m_buckets in the new map (we know they will not go into overflow). 
         * We then swap m_overflow_elements into the new map and we update all necessary overflow flags.
         * 
         * This way we don't do any memory allocation (outside the construction of m_buckets) and we avoid
         * any exception which may hinder the strong exception-safe guarantee.
         */
        hopscotch_hash tmp_map(count, m_hash, m_key_equal, get_allocator(), m_max_load_factor);
        
        for(hopscotch_bucket& bucket : m_buckets) {
            if(bucket.is_empty()) {
                continue;
            }
            
            const std::size_t ibucket_for_hash = tmp_map.bucket_for_hash(tmp_map.m_hash(KeySelect()(bucket.get_value_type())));
            tmp_map.insert_internal(std::move(bucket.get_value_type()), ibucket_for_hash);
        }
        
        assert(tmp_map.m_overflow_elements.empty());
        if(!m_overflow_elements.empty()) {
            tmp_map.m_overflow_elements.swap(m_overflow_elements);
            tmp_map.m_nb_elements += tmp_map.m_overflow_elements.size();
            
            for(const value_type& value : tmp_map.m_overflow_elements) {
                const std::size_t ibucket_for_hash = tmp_map.bucket_for_hash(tmp_map.m_hash(KeySelect()(value)));
                tmp_map.m_buckets[ibucket_for_hash].set_overflow(true);
            }
        }
        
        std::swap(*this, tmp_map);
    } 
    
    /*
     * Find in m_overflow_elements an element for which the bucket it initially belongs to, equals original_bucket_for_hash.
     * Return m_overflow_elements.end() if none.
     */
    iterator_overflow find_in_overflow_from_bucket(iterator_overflow search_start, 
                                                   std::size_t original_bucket_for_hash) 
    {
        for(auto it = search_start; it != m_overflow_elements.end(); ++it) {
            const std::size_t bucket_for_overflow_hash = bucket_for_hash(m_hash(KeySelect()(*it)));
            if(bucket_for_overflow_hash == original_bucket_for_hash) {
                return it;
            }
        }
        
        return m_overflow_elements.end();
    }
    
    // iterator is in overflow list
    iterator_overflow erase_from_overflow(const_iterator_overflow pos, std::size_t ibucket_for_hash) {
        auto it_next = m_overflow_elements.erase(pos);
        m_nb_elements--;
        
        // Check if we can remove the overflow flag
        assert(m_buckets[ibucket_for_hash].has_overflow());
        if(find_in_overflow_from_bucket(m_overflow_elements.begin(), ibucket_for_hash) == m_overflow_elements.end()) {
            m_buckets[ibucket_for_hash].set_overflow(false);
        }
        
        return it_next;
    }
    
    // iterator is in bucket
    void erase_from_bucket(iterator_buckets pos, std::size_t ibucket_for_hash) {
        const std::size_t ibucket_for_key = std::distance(m_buckets.begin(), pos);
        
        m_buckets[ibucket_for_key].remove_value_type();
        m_buckets[ibucket_for_hash].toggle_neighbor_presence(ibucket_for_key - ibucket_for_hash);
        m_nb_elements--;
    }
    
        
    template <typename P, class... Args>
    std::pair<iterator, bool> try_emplace_internal(P&& key, Args&&... args_value) {
        const std::size_t ibucket_for_hash = bucket_for_hash(m_hash(key));
        
        // Check if already presents
        auto it_find = find_internal(key, m_buckets.begin() + ibucket_for_hash);
        if(it_find != end()) {
            return std::make_pair(it_find, false);
        }
        

        return insert_internal(value_type(std::piecewise_construct, 
                                          std::forward_as_tuple(std::forward<P>(key)), 
                                          std::forward_as_tuple(std::forward<Args>(args_value)...)), 
                               ibucket_for_hash);
    }
    
    template<typename P>
    std::pair<iterator, bool> insert_internal(P&& value) {
        const std::size_t ibucket_for_hash = bucket_for_hash(m_hash(KeySelect()(value)));
        
        // Check if already presents
        auto it_find = find_internal(KeySelect()(value), m_buckets.begin() + ibucket_for_hash);
        if(it_find != end()) {
            return std::make_pair(it_find, false);
        }
        
        
        return insert_internal(std::forward<P>(value), ibucket_for_hash);
    }
    
    template<typename P>
    std::pair<iterator, bool> insert_internal(P&& value, std::size_t ibucket_for_hash) {
        if((m_nb_elements - m_overflow_elements.size() + 1) > m_load_threshold) {
            rehash_internal(get_expand_size());
            ibucket_for_hash = bucket_for_hash(m_hash(KeySelect()(value)));
        }
        
        std::size_t ibucket_empty = find_empty_bucket(ibucket_for_hash);
        if(ibucket_empty < m_buckets.size()) {
            do {
                // Empty bucket is in range of NeighborhoodSize, use it
                if(ibucket_empty - ibucket_for_hash < NeighborhoodSize) {
                    auto it = insert_in_bucket(std::forward<P>(value), ibucket_empty, ibucket_for_hash);
                    return std::make_pair(iterator(it, m_buckets.end(), m_overflow_elements.begin()), true);
                }
            }
            // else, try to swap values to get a closer empty bucket
            while(swap_empty_bucket_closer(ibucket_empty));
        }
            
        // A rehash will not change the neighborhood, put the value in overflow list
        if(!will_neighborhood_change_on_rehash(ibucket_for_hash)) {
            m_overflow_elements.push_back(std::forward<P>(value));
            m_buckets[ibucket_for_hash].set_overflow(true);
            m_nb_elements++;
            
            return std::make_pair(iterator(m_buckets.end(), m_buckets.end(), std::prev(m_overflow_elements.end())), true);
        }
    
        rehash_internal(get_expand_size());
        
        ibucket_for_hash = bucket_for_hash(m_hash(KeySelect()(value)));
        return insert_internal(std::forward<P>(value), ibucket_for_hash);
    }    
    
    /*
     * Return true if a rehash will change the position of a key-value in the neighborhood of ibucket_neighborhood_check.
     * In this case a rehash is needed instead of puting the value in overflow list.
     */
    bool will_neighborhood_change_on_rehash(size_t ibucket_neighborhood_check) const {
        for(size_t ibucket = ibucket_neighborhood_check; 
            ibucket < m_buckets.size() && (ibucket - ibucket_neighborhood_check) < NeighborhoodSize; 
            ++ibucket)
        {
            assert(!m_buckets[ibucket].is_empty());
            
            const size_t hash = m_hash(KeySelect()(m_buckets[ibucket].get_value_type()));
            if(bucket_for_hash(hash) != bucket_for_hash(hash, get_expand_size())) {
                return true;
            }
        }
        
        return false;
    }
    
    /*
     * Return the index of an empty bucket in m_buckets.
     * If none, the returned index equals m_buckets.size()
     */
    std::size_t find_empty_bucket(std::size_t ibucket_start) const {
        const std::size_t limit = std::min(ibucket_start + m_max_probes_for_empty_bucket, m_buckets.size());
        for(; ibucket_start < limit; ibucket_start++) {
            if(m_buckets[ibucket_start].is_empty()) {
                return ibucket_start;
            }
        }
        
        return m_buckets.size();
    }
    
    /*
     * Insert value in ibucket_empty where value originally belongs to ibucket_for_hash
     * 
     * Return bucket iterator to ibucket_empty
     */
    template<typename P>
    iterator_buckets insert_in_bucket(P&& value, std::size_t ibucket_empty, std::size_t ibucket_for_hash) {        
        assert(ibucket_empty >= ibucket_for_hash );
        assert(m_buckets[ibucket_empty].is_empty());
        m_buckets[ibucket_empty].set_value_type(std::forward<P>(value));
        
        assert(!m_buckets[ibucket_for_hash].is_empty());
        m_buckets[ibucket_for_hash].toggle_neighbor_presence(ibucket_empty - ibucket_for_hash);
        m_nb_elements++;
        
        return m_buckets.begin() + ibucket_empty;
    }
    
    /*
     * Try to swap the bucket ibucket_empty_in_out with a bucket preceding it while keeping the neighborhood conditions correct.
     * 
     * If a swap was possible, the position of ibucket_empty_in_out will be closer to 0 and true will re returned.
     */
    bool swap_empty_bucket_closer(std::size_t& ibucket_empty_in_out) {
        assert(ibucket_empty_in_out >= NeighborhoodSize);
        const std::size_t neighborhood_start = ibucket_empty_in_out - NeighborhoodSize + 1;
        
        for(std::size_t to_check = neighborhood_start; to_check < ibucket_empty_in_out; to_check++) {
            neighborhood_bitmap neighborhood_infos = m_buckets[to_check].get_neighborhood_infos();
            std::size_t to_swap = to_check;
            
            while(neighborhood_infos != 0 && to_swap < ibucket_empty_in_out) {
                if((neighborhood_infos & 1) == 1) {
                    assert(m_buckets[ibucket_empty_in_out].is_empty());
                    assert(!m_buckets[to_swap].is_empty());
                    
                    m_buckets[to_swap].swap_value_type_into_empty_bucket(m_buckets[ibucket_empty_in_out]);
                    
                    assert(!m_buckets[to_check].check_neighbor_presence(ibucket_empty_in_out - to_check));
                    assert(m_buckets[to_check].check_neighbor_presence(to_swap - to_check));
                    
                    m_buckets[to_check].toggle_neighbor_presence(ibucket_empty_in_out - to_check);
                    m_buckets[to_check].toggle_neighbor_presence(to_swap - to_check);
                    
                    
                    ibucket_empty_in_out = to_swap;
                    
                    return true;
                }
                
                to_swap++;
                neighborhood_infos = static_cast<neighborhood_bitmap>(neighborhood_infos >> 1);
            }
        }
        
        return false;
    }
    
    iterator_buckets get_first_non_empty_buckets_iterator() {
        auto it_first = static_cast<const hopscotch_hash*>(this)->get_first_non_empty_buckets_iterator();
        return m_buckets.begin() + std::distance(m_buckets.cbegin(), it_first); 
    }
    
    const_iterator_buckets get_first_non_empty_buckets_iterator() const {
        auto begin = m_buckets.cbegin();
        while(begin != m_buckets.cend() && begin->is_empty()) {
            ++begin;
        }
        
        return begin;
    }
    
    iterator find_internal(const Key& key, iterator_buckets it_bucket) {
        auto it = find_in_buckets(key, it_bucket);
        if(it != m_buckets.cend()) {
            return iterator(it, m_buckets.end(), m_overflow_elements.begin());
        }
        
        if(!it_bucket->has_overflow()) {
            return end();
        }
        
        return iterator(m_buckets.end(), m_buckets.end(), std::find_if(m_overflow_elements.begin(), m_overflow_elements.end(), 
                                                                        [&](const value_type& value) { 
                                                                            return m_key_equal(key, KeySelect()(value)); 
                                                                        }));
    }
    
    const_iterator find_internal(const Key& key, const_iterator_buckets it_bucket) const {
        auto it = find_in_buckets(key, it_bucket);
        if(it != m_buckets.cend()) {
            return const_iterator(it, m_buckets.cend(), m_overflow_elements.cbegin());
        }
        
        if(!it_bucket->has_overflow()) {
            return cend();
        }

        
        return const_iterator(m_buckets.cend(), m_buckets.cend(), std::find_if(m_overflow_elements.cbegin(), m_overflow_elements.cend(), 
                                                                                [&](const value_type& value) { 
                                                                                    return m_key_equal(key, KeySelect()(value)); 
                                                                                }));        
    }
    
    iterator_buckets find_in_buckets(const Key& key, iterator_buckets it_bucket) {   
        auto it_find = static_cast<const hopscotch_hash*>(this)->find_in_buckets(key, it_bucket); 
        return m_buckets.begin() + std::distance(m_buckets.cbegin(), it_find);
    }

    
    const_iterator_buckets find_in_buckets(const Key& key, const_iterator_buckets it_bucket) const {      
        // TODO Try to optimize the function. 
        // I tried to use ffs and  __builtin_ffs functions but I could not reduce the time the function takes with -march=native
        
        neighborhood_bitmap neighborhood_infos = it_bucket->get_neighborhood_infos();
        while(neighborhood_infos != 0) {
            if((neighborhood_infos & 1) == 1) {
                if(m_key_equal(KeySelect()(it_bucket->get_value_type()), key)) {
                    return it_bucket;
                }
            }
            
            ++it_bucket;
            neighborhood_infos = static_cast<neighborhood_bitmap>(neighborhood_infos >> 1);
        }
        
        return m_buckets.end();
    }
    
    size_type get_expand_size() const {
        return static_cast<size_type>(std::ceil(bucket_count() * REHASH_SIZE_MULTIPLICATION_FACTOR));
    }
    
    
    // TODO could be faster
    static std::size_t round_up_to_power_of_two(std::size_t value) {
        std::size_t power = 1;
        while(power < value) {
            power <<= 1;
        }
        
        return power;
    }
    
    static constexpr bool is_power_of_two(std::size_t value) {
        return value != 0 && (value & (value - 1)) == 0;
    }
    
public:    
    static const size_type DEFAULT_INIT_BUCKETS_SIZE = 16;
    static constexpr float DEFAULT_MAX_LOAD_FACTOR = 0.95f;
    static const std::size_t DEFAULT_MAX_PROBES_FOR_EMPTY_BUCKET = 10*NeighborhoodSize;
    
private:    
    static const size_type MIN_BUCKETS_SIZE = 2;
    static constexpr double REHASH_SIZE_MULTIPLICATION_FACTOR = 1.0*GrowthFactor::num/GrowthFactor::den;
    static const bool USE_POWER_OF_TWO_MOD = is_power_of_two(GrowthFactor::num) && is_power_of_two(GrowthFactor::den) &&
                                                static_cast<double>(static_cast<std::size_t>(REHASH_SIZE_MULTIPLICATION_FACTOR)) == 
                                                REHASH_SIZE_MULTIPLICATION_FACTOR;

    /*
     * bucket_count() should be a power of 2. We can then use "hash & (bucket_count() - 1)" 
     * to get the bucket for a hash if the GrowthFactor is right for that.
     */
    static_assert(is_power_of_two(DEFAULT_INIT_BUCKETS_SIZE), "DEFAULT_INIT_BUCKETS_SIZE should be a power of 2.");
    static_assert(is_power_of_two(MIN_BUCKETS_SIZE), "MIN_BUCKETS_SIZE should be a power of 2.");
    static_assert(REHASH_SIZE_MULTIPLICATION_FACTOR >= 1.1, "Grow factor should be >= 1.1.");
    
private:    
    std::vector<hopscotch_bucket, buckets_allocator> m_buckets;
    std::list<value_type, overflow_elements_allocator> m_overflow_elements;
    
    size_type m_nb_elements;
    
    
    float m_max_load_factor;
    size_type m_load_threshold;
    
    std::size_t m_max_probes_for_empty_bucket;
    
    hasher m_hash;
    key_equal m_key_equal;
};

}







/**
 * Implementation of a hash map using the hopscotch hashing algorithm.
 * 
 * The size of the neighborhood (NeighborhoodSize) must be > 0 and <= 62.
 * 
 * The Key and the value T must be either move-constructible, copy-constuctible or both.
 * 
 * By default the map grows by a factor of 2. This has the advantage to allow us to do fast modulo because
 * the number of buckets is kept to a power of two. The growth factor can be changed with std::ratio 
 * (ex: std::ratio<3, 2> will give a growth factor of 1.5), but if the resulting growth factor
 * is not a power of two, the map will be slower as it can't use the power of two modulo optimization.
 * 
 * If the destructors of Key or T throw an exception, behaviour of the class is undefined.
 * 
 * Iterators invalidation:
 *  - clear, operator=: always invalidate the iterators.
 *  - insert, operator[]: invalidate the iterators if there is a rehash, or if a displacement is needed to resolve a collision (which mean that most of the time, insert will invalidate the iterators).
 *  - erase: iterator on the erased element is the only one which become invalid.
 */
template<class Key, 
         class T, 
         class Hash = std::hash<Key>,
         class KeyEqual = std::equal_to<Key>,
         class Allocator = std::allocator<std::pair<Key, T>>,
         unsigned int NeighborhoodSize = 62,
         class GrowthFactor = std::ratio<2, 1>>
class hopscotch_map {
private:    
    class KeySelect {
    public:
        using key_type = Key;
        
        const key_type& operator()(const std::pair<Key, T>& key_value) const {
            return key_value.first;
        }
        
        key_type& operator()(std::pair<Key, T>& key_value) {
            return key_value.first;
        }
    };  
    
    class ValueSelect {
    public:
        using value_type = T;
        
        const value_type& operator()(const std::pair<Key, T>& key_value) const {
            return key_value.second;
        }
        
        value_type& operator()(std::pair<Key, T>& key_value) {
            return key_value.second;
        }
    };
    
    using ht = detail::hopscotch_hash<std::pair<Key, T>, KeySelect, ValueSelect,
                                      Hash, KeyEqual, 
                                      Allocator, NeighborhoodSize, GrowthFactor>;
    
public:
    using key_type = typename ht::key_type;
    using mapped_type = T;
    using value_type = typename ht::value_type;
    using size_type = typename ht::size_type;
    using difference_type = typename ht::difference_type;
    using hasher = typename ht::hasher;
    using key_equal = typename ht::key_equal;
    using allocator_type = typename ht::allocator_type;
    using reference = typename ht::reference;
    using const_reference = typename ht::const_reference;
    using pointer = typename ht::pointer;
    using const_pointer = typename ht::const_pointer;
    using iterator = typename ht::iterator;
    using const_iterator = typename ht::const_iterator;
    
    
    /*
     * Constructors
     */
    hopscotch_map() : hopscotch_map(ht::DEFAULT_INIT_BUCKETS_SIZE) {
    }
    
    explicit hopscotch_map(size_type bucket_count, 
                        const Hash& hash = Hash(),
                        const KeyEqual& equal = KeyEqual(),
                        const Allocator& alloc = Allocator()) : 
                        m_ht(bucket_count, hash, equal, alloc, ht::DEFAULT_MAX_LOAD_FACTOR)
    {
    }
    
    hopscotch_map(size_type bucket_count,
                  const Allocator& alloc) : hopscotch_map(bucket_count, Hash(), KeyEqual(), alloc)
    {
    }
    
    hopscotch_map(size_type bucket_count,
                  const Hash& hash,
                  const Allocator& alloc) : hopscotch_map(bucket_count, hash, KeyEqual(), alloc)
    {
    }
    
    explicit hopscotch_map(const Allocator& alloc) : hopscotch_map(ht::DEFAULT_INIT_BUCKETS_SIZE, alloc) {
    }
    
    template<class InputIt>
    hopscotch_map(InputIt first, InputIt last,
                size_type bucket_count = ht::DEFAULT_INIT_BUCKETS_SIZE,
                const Hash& hash = Hash(),
                const KeyEqual& equal = KeyEqual(),
                const Allocator& alloc = Allocator()) : hopscotch_map(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }
    
    template<class InputIt>
    hopscotch_map(InputIt first, InputIt last,
                size_type bucket_count,
                const Allocator& alloc) : hopscotch_map(first, last, bucket_count, Hash(), KeyEqual(), alloc)
    {
    }
    
    template<class InputIt>
    hopscotch_map(InputIt first, InputIt last,
                size_type bucket_count,
                const Hash& hash,
                const Allocator& alloc) : hopscotch_map(first, last, bucket_count, hash, KeyEqual(), alloc)
    {
    }

    hopscotch_map(std::initializer_list<value_type> init,
                    size_type bucket_count = ht::DEFAULT_INIT_BUCKETS_SIZE,
                    const Hash& hash = Hash(),
                    const KeyEqual& equal = KeyEqual(),
                    const Allocator& alloc = Allocator()) : 
                    hopscotch_map(init.begin(), init.end(), bucket_count, hash, equal, alloc)
    {
    }

    hopscotch_map(std::initializer_list<value_type> init,
                    size_type bucket_count,
                    const Allocator& alloc) : 
                    hopscotch_map(init.begin(), init.end(), bucket_count, Hash(), KeyEqual(), alloc)
    {
    }

    hopscotch_map(std::initializer_list<value_type> init,
                    size_type bucket_count,
                    const Hash& hash,
                    const Allocator& alloc) : 
                    hopscotch_map(init.begin(), init.end(), bucket_count, hash, KeyEqual(), alloc)
    {
    }

    
    hopscotch_map& operator=(std::initializer_list<value_type> ilist) {
        m_ht.clear();
        m_ht.insert(ilist.begin(), ilist.end());
        return *this;
    }
    
    allocator_type get_allocator() const { return m_ht.get_allocator(); }
    
    
    /*
     * Iterators
     */
    iterator begin() noexcept { return m_ht.begin(); }
    const_iterator begin() const noexcept { return m_ht.begin(); }
    const_iterator cbegin() const noexcept { return m_ht.cbegin(); }
    
    iterator end() noexcept { return m_ht.end(); }
    const_iterator end() const noexcept { return m_ht.end(); }
    const_iterator cend() const noexcept { return m_ht.cend(); }
    
    
    /*
     * Capacity
     */
    bool empty() const noexcept { return m_ht.empty(); }
    size_type size() const noexcept { return m_ht.size(); }
    size_type max_size() const noexcept { return m_ht.max_size(); }
    
    /*
     * Modifiers
     */
    void clear() noexcept { m_ht.clear(); }
    
    std::pair<iterator, bool> insert(const value_type& value) { return m_ht.insert(value); }
    std::pair<iterator, bool> insert(value_type&& value) { return m_ht.insert(std::move(value)); }
    
    template<class InputIt>
    void insert(InputIt first, InputIt last) { m_ht.insert(first, last); }
    void insert(std::initializer_list<value_type> ilist) { m_ht.insert(ilist.begin(), ilist.end()); }

    /**
     * Due to the Implementation, emplace will need to move or copy the std::pair<Key, Value> once.
     * The method is the equivalent of insert(value_type(std::forward<Args>(args)...));
     * 
     * Mainly here for compatibility with the std::unordered_map interface.
     */
    template<class... Args>
    std::pair<iterator,bool> emplace(Args&&... args) { return m_ht.emplace(std::forward<Args>(args)...); }
    
    template <class... Args>
    std::pair<iterator, bool> try_emplace(const key_type& k, Args&&... args) { 
        return m_ht.try_emplace(k, std::forward<Args>(args)...);
    }
    
    template <class... Args>
    std::pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
        return m_ht.try_emplace(std::move(k), std::forward<Args>(args)...);
    }

    iterator erase(const_iterator pos) { return m_ht.erase(pos); }
    iterator erase(const_iterator first, const_iterator last) { return m_ht.erase(first, last); }
    size_type erase(const key_type& key) { return m_ht.erase(key); }
    
    /*
     * Lookup
     */
    T& at(const Key& key) {
        return const_cast<T&>(static_cast<const hopscotch_map*>(this)->at(key));
    }
    
    const T& at(const Key& key) const {
        const T* value = m_ht.find_value(key);
        if(value == nullptr) {
            throw std::out_of_range("Couldn't find key.");
        }
        else {
            return *value;
        }
    }
    
    T& operator[](const Key& key) {
        T* value = const_cast<T*>(m_ht.find_value(key));
        if(value == nullptr) {
            return insert(std::make_pair(key, T())).first.value();
        }
        else {
            return *value;
        }
    }    
    
    size_type count(const Key& key) const { return m_ht.count(key); }
    
    iterator find(const Key& key) { return m_ht.find(key); }
    const_iterator find(const Key& key) const { return m_ht.find(key); }
    
    /*
     * Bucket interface 
     */
    size_type bucket_count() const { return m_ht.bucket_count(); }
    
    
    /*
     *  Hash policy 
     */
    float load_factor() const { return m_ht.load_factor(); }
    float max_load_factor() const { return m_ht.max_load_factor(); }
    void max_load_factor(float ml) { m_ht.max_load_factor(ml); }
    
    void rehash(size_type count) { m_ht.rehash(count); }
    void reserve(size_type count) { m_ht.reserve(count); }
    
    
    /*
     * Observers
     */
    hasher hash_function() const { return m_ht.hash_function(); }
    key_equal key_eq() const { return m_ht.key_eq(); }
    
    /*
     * Other
     */
    std::size_t max_probes_for_empty_bucket() const { return m_ht.max_probes_for_empty_bucket; }
    
    /**
     * On insert, when searching for an empty bucket, linear probing is used. 
     * The maximum number of bucket checked is defined by 'max_probes'. If no empty bucket is found before that, 
     * a rehash occurs.
     * 
     * A high value will result in a more aggressive rehash policy but may speed-up inserts.
     */
    void max_probes_for_empty_bucket(std::size_t max_probes) { m_ht.max_probes_for_empty_bucket(max_probes); }
    
private:
    ht m_ht;
};



template<class Key, class T, class Hash, class KeyEqual, class Allocator, unsigned int NeighborhoodSize, class GrowthFactor>
inline bool operator==(const hopscotch_map<Key, T, Hash, KeyEqual, Allocator, NeighborhoodSize, GrowthFactor>& lhs, 
                       const hopscotch_map<Key, T, Hash, KeyEqual, Allocator, NeighborhoodSize, GrowthFactor>& rhs)
{
    if(lhs.size() != rhs.size()) {
        return false;
    }
    
    for(const auto& element_lhs : lhs) {
        const auto it_element_rhs = rhs.find(element_lhs.first);
        if(it_element_rhs == rhs.cend() || element_lhs.second != it_element_rhs->second) {
            return false;
        }
    }
    
    return true;
}


template<class Key, class T, class Hash, class KeyEqual, class Allocator, unsigned int NeighborhoodSize, class GrowthFactor>
inline bool operator!=(const hopscotch_map<Key, T, Hash, KeyEqual, Allocator, NeighborhoodSize, GrowthFactor>& lhs, 
                       const hopscotch_map<Key, T, Hash, KeyEqual, Allocator, NeighborhoodSize, GrowthFactor>& rhs)
{
    return !operator==(lhs, rhs);
}










/**
 * Implementation of a hash set using the hopscotch hashing algorithm.
 * 
 * The size of the neighborhood (NeighborhoodSize) must be > 0 and <= 62.
 * 
 * The Key must be either move-constructible, copy-constuctible or both.
 * 
 * By default the map grows by a factor of 2. This has the advantage to allow us to do fast modulo because
 * the number of buckets is kept to a power of two. The growth factor can be changed with std::ratio 
 * (ex: std::ratio<3, 2> will give a growth factor of 1.5), but if the resulting growth factor
 * is not a power of two, the map will be slower as it can't use the power of two modulo optimization.
 * 
 * If the destructor of Key throws an exception, behaviour of the class is undefined.
 * 
 * Iterators invalidation:
 *  - clear, operator=: always invalidate the iterators.
 *  - insert: invalidate the iterators if there is a rehash, or if a displacement is needed to resolve a collision (which mean that most of the time, insert will invalidate the iterators).
 *  - erase: iterator on the erased element is the only one which become invalid.
 */
template<class Key, 
         class Hash = std::hash<Key>,
         class KeyEqual = std::equal_to<Key>,
         class Allocator = std::allocator<Key>,
         unsigned int NeighborhoodSize = 62,
         class GrowthFactor = std::ratio<2, 1>>
class hopscotch_set {
private:    
    class KeySelect {
    public:
        using key_type = Key;
        
        const key_type& operator()(const Key& key) const {
            return key;
        }
        
        key_type& operator()(Key& key) {
            return key;
        }
    };
    
    using ht = detail::hopscotch_hash<Key, KeySelect, void,
                                      Hash, KeyEqual, 
                                      Allocator, NeighborhoodSize, GrowthFactor>;
            
public:
    using key_type = typename ht::key_type;
    using value_type = typename ht::value_type;
    using size_type = typename ht::size_type;
    using difference_type = typename ht::difference_type;
    using hasher = typename ht::hasher;
    using key_equal = typename ht::key_equal;
    using allocator_type = typename ht::allocator_type;
    using reference = typename ht::reference;
    using const_reference = typename ht::const_reference;
    using pointer = typename ht::pointer;
    using const_pointer = typename ht::const_pointer;
    using iterator = typename ht::const_iterator;
    using const_iterator = typename ht::const_iterator;

    
    /*
     * Constructors
     */
    hopscotch_set() : hopscotch_set(ht::DEFAULT_INIT_BUCKETS_SIZE) {
    }
    
    explicit hopscotch_set(size_type bucket_count, 
                        const Hash& hash = Hash(),
                        const KeyEqual& equal = KeyEqual(),
                        const Allocator& alloc = Allocator()) : 
                        m_ht(bucket_count, hash, equal, alloc, ht::DEFAULT_MAX_LOAD_FACTOR)
    {
    }
    
    hopscotch_set(size_type bucket_count,
                  const Allocator& alloc) : hopscotch_set(bucket_count, Hash(), KeyEqual(), alloc)
    {
    }
    
    hopscotch_set(size_type bucket_count,
                  const Hash& hash,
                  const Allocator& alloc) : hopscotch_set(bucket_count, hash, KeyEqual(), alloc)
    {
    }
    
    explicit hopscotch_set(const Allocator& alloc) : hopscotch_set(ht::DEFAULT_INIT_BUCKETS_SIZE, alloc) {
    }
    
    template<class InputIt>
    hopscotch_set(InputIt first, InputIt last,
                size_type bucket_count = ht::DEFAULT_INIT_BUCKETS_SIZE,
                const Hash& hash = Hash(),
                const KeyEqual& equal = KeyEqual(),
                const Allocator& alloc = Allocator()) : hopscotch_set(bucket_count, hash, equal, alloc)
    {
        insert(first, last);
    }
    
    template<class InputIt>
    hopscotch_set(InputIt first, InputIt last,
                size_type bucket_count,
                const Allocator& alloc) : hopscotch_set(first, last, bucket_count, Hash(), KeyEqual(), alloc)
    {
    }
    
    template<class InputIt>
    hopscotch_set(InputIt first, InputIt last,
                size_type bucket_count,
                const Hash& hash,
                const Allocator& alloc) : hopscotch_set(first, last, bucket_count, hash, KeyEqual(), alloc)
    {
    }

    hopscotch_set(std::initializer_list<value_type> init,
                    size_type bucket_count = ht::DEFAULT_INIT_BUCKETS_SIZE,
                    const Hash& hash = Hash(),
                    const KeyEqual& equal = KeyEqual(),
                    const Allocator& alloc = Allocator()) : 
                    hopscotch_set(init.begin(), init.end(), bucket_count, hash, equal, alloc)
    {
    }

    hopscotch_set(std::initializer_list<value_type> init,
                    size_type bucket_count,
                    const Allocator& alloc) : 
                    hopscotch_set(init.begin(), init.end(), bucket_count, Hash(), KeyEqual(), alloc)
    {
    }

    hopscotch_set(std::initializer_list<value_type> init,
                    size_type bucket_count,
                    const Hash& hash,
                    const Allocator& alloc) : 
                    hopscotch_set(init.begin(), init.end(), bucket_count, hash, KeyEqual(), alloc)
    {
    }

    
    hopscotch_set& operator=(std::initializer_list<value_type> ilist) {
        m_ht.clear();
        m_ht.insert(ilist.begin(), ilist.end());
        return *this;
    }
    
    allocator_type get_allocator() const { return m_ht.get_allocator(); }
    
    
    /*
     * Iterators
     */
    iterator begin() noexcept { return m_ht.begin(); }
    const_iterator begin() const noexcept { return m_ht.begin(); }
    const_iterator cbegin() const noexcept { return m_ht.cbegin(); }
    
    iterator end() noexcept { return m_ht.end(); }
    const_iterator end() const noexcept { return m_ht.end(); }
    const_iterator cend() const noexcept { return m_ht.cend(); }
    
    
    /*
     * Capacity
     */
    bool empty() const noexcept { return m_ht.empty(); }
    size_type size() const noexcept { return m_ht.size(); }
    size_type max_size() const noexcept { return m_ht.max_size(); }
    
    /*
     * Modifiers
     */
    void clear() noexcept { m_ht.clear(); }
    
    std::pair<iterator, bool> insert(const value_type& value) { return m_ht.insert(value); }
    std::pair<iterator, bool> insert(value_type&& value) { return m_ht.insert(std::move(value)); }
    
    template<class InputIt>
    void insert(InputIt first, InputIt last) { m_ht.insert(first, last); }
    void insert(std::initializer_list<value_type> ilist) { m_ht.insert(ilist.begin(), ilist.end()); }

    /**
     * Due to the Implementation, emplace will need to move or copy the Key once.
     * The method is the equivalent of insert(value_type(std::forward<Args>(args)...));
     * 
     * Mainly here for compatibility with the std::unordered_map interface.
     */
    template<class... Args>
    std::pair<iterator,bool> emplace(Args&&... args) { return m_ht.emplace(std::forward<Args>(args)...); }

    iterator erase(const_iterator pos) { return m_ht.erase(pos); }
    iterator erase(const_iterator first, const_iterator last) { return m_ht.erase(first, last); }
    size_type erase(const key_type& key) { return m_ht.erase(key); }
    
    /*
     * Lookup
     */
    size_type count(const Key& key) const { return m_ht.count(key); }
    
    iterator find(const Key& key) { return m_ht.find(key); }
    const_iterator find(const Key& key) const { return m_ht.find(key); }
    
    /*
     * Bucket interface 
     */
    size_type bucket_count() const { return m_ht.bucket_count(); }
    
    
    /*
     *  Hash policy 
     */
    float load_factor() const { return m_ht.load_factor(); }
    float max_load_factor() const { return m_ht.max_load_factor(); }
    void max_load_factor(float ml) { m_ht.max_load_factor(ml); }
    
    void rehash(size_type count) { m_ht.rehash(count); }
    void reserve(size_type count) { m_ht.reserve(count); }
    
    
    /*
     * Observers
     */
    hasher hash_function() const { return m_ht.hash_function(); }
    key_equal key_eq() const { return m_ht.key_eq(); }
    
    
    /*
     * Other
     */
    std::size_t max_probes_for_empty_bucket() const { return m_ht.max_probes_for_empty_bucket; }
    
    /**
     * On insert, when searching for an empty bucket, linear probing is used. 
     * The maximum number of bucket checked is defined by 'max_probe'. If no empty bucket is found before that, 
     * a rehash occurs.
     * 
     * A high value will result in a more aggressive rehash policy but may speed-up inserts.
     */
    void max_probes_for_empty_bucket(std::size_t max_probes) { m_ht.max_probes_for_empty_bucket(max_probes); }
    
private:
    ht m_ht;    
};


template<class Key, class Hash, class KeyEqual, class Allocator, unsigned int NeighborhoodSize, class GrowthFactor>
inline bool operator==(const hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, GrowthFactor>& lhs, 
                       const hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, GrowthFactor>& rhs)
{
    if(lhs.size() != rhs.size()) {
        return false;
    }
    
    for(const auto& element_lhs : lhs) {
        const auto it_element_rhs = rhs.find(element_lhs);
        if(it_element_rhs == rhs.cend()) {
            return false;
        }
    }
    
    return true;
}


template<class Key, class Hash, class KeyEqual, class Allocator, unsigned int NeighborhoodSize, class GrowthFactor>
inline bool operator!=(const hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, GrowthFactor>& lhs, 
                       const hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, GrowthFactor>& rhs)
{
    return !operator==(lhs, rhs);
}


}

#endif
