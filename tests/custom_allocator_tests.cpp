#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "utils.h"
#include "hopscotch_map.h"


static std::size_t nb_custom_allocs = 0;

template<typename T>
class custom_allocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;


    template<typename U> 
    struct rebind { 
        using other = custom_allocator<U>;
    };
    
    custom_allocator() = default;
    custom_allocator(const custom_allocator&) = default;
    
    template<typename U> 
    custom_allocator(const custom_allocator<U>&) {
    }

    pointer address(reference x) const noexcept {
        return &x;
    }
    
    const_pointer address(const_reference x) const noexcept {
        return &x;
    }

    pointer allocate(size_type n, const void* /*hint*/ = 0) {
        nb_custom_allocs++;
        
        pointer ptr = static_cast<pointer>(malloc(n * sizeof(T)));
        if(ptr == nullptr) {
            throw std::bad_alloc();
        }
        
        return ptr;
    }

    void deallocate(T* p, size_type /*n*/) {
        free(p);
    }
    
    size_type max_size() const noexcept {
        return std::numeric_limits<size_type>::max()/sizeof(value_type);
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        ::new(static_cast<void *>(p)) U(std::forward<Args>(args)...);
    }

    template<typename U>
    void destroy(U* p) {
        p->~U();
    }
};

template <class T, class U>
bool operator==(const custom_allocator<T>&, const custom_allocator<U>&) { 
    return true; 
}

template <class T, class U>
bool operator!=(const custom_allocator<T>&, const custom_allocator<U>&) { 
    return false; 
}


BOOST_AUTO_TEST_SUITE(test_custom_allocator)

BOOST_AUTO_TEST_CASE(test_custom_allocator_1) {
        nb_custom_allocs = 0;
        
        tsl::hopscotch_map<int, int, mod_hash<9>, std::equal_to<int>, 
                           custom_allocator<std::pair<int, int>>, 6> map;
        
        const int nb_elements = 10000;
        for(int i = 0; i < nb_elements; i++) {
            map.insert({i, i*2});
        }
        
        BOOST_CHECK_NE(map.overflow_size(), 0);
        BOOST_CHECK_NE(nb_custom_allocs, 0);
        
        //TODO check that number of global allocations is 0
}

BOOST_AUTO_TEST_SUITE_END()
