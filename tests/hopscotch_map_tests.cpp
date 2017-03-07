#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "utils.h"
#include "hopscotch_map.h"
#include "hopscotch_sc_map.h"


using test_types = boost::mpl::list<
                        tsl::hopscotch_map<int64_t, int64_t>, 
                        tsl::hopscotch_map<int64_t, int64_t, std::hash<int64_t>, std::equal_to<int64_t>, 
                            std::allocator<std::pair<int64_t, int64_t>>, 6>, 
                        // Test with hash having a lot of collisions
                        tsl::hopscotch_map<int64_t, int64_t, mod_hash<9>>,
                        tsl::hopscotch_map<int64_t, int64_t, mod_hash<9>, std::equal_to<int64_t>, 
                            std::allocator<std::pair<int64_t, int64_t>>, 6>, 
                        tsl::hopscotch_map<std::string, std::string>,
                        tsl::hopscotch_map<std::string, std::string, mod_hash<9>>,
                        tsl::hopscotch_map<std::string, std::string, mod_hash<9>, std::equal_to<std::string>, 
                            std::allocator<std::pair<std::string, std::string>>, 6>,
                        tsl::hopscotch_map<int64_t, move_only_test>,
                        tsl::hopscotch_map<move_only_test, int64_t>,
                        tsl::hopscotch_map<move_only_test, move_only_test, mod_hash<9>, std::equal_to<move_only_test>, 
                            std::allocator<std::pair<move_only_test, move_only_test>>, 6>,
                        tsl::hopscotch_map<self_reference_member_test, self_reference_member_test>,
                        tsl::hopscotch_map<self_reference_member_test, self_reference_member_test, 
                            mod_hash<9>, std::equal_to<self_reference_member_test>, 
                            std::allocator<std::pair<self_reference_member_test, self_reference_member_test>>, 6>,
                        // Store hash
                        tsl::hopscotch_map<std::string, std::string, mod_hash<9>, std::equal_to<std::string>, 
                            std::allocator<std::pair<std::string, std::string>>, 6, true>,
                        tsl::hopscotch_map<self_reference_member_test, self_reference_member_test, 
                            mod_hash<9>, std::equal_to<self_reference_member_test>, 
                            std::allocator<std::pair<self_reference_member_test, self_reference_member_test>>, 6, true>,
                        // hopscotch_sc_map
                        tsl::hopscotch_sc_map<int64_t, int64_t, mod_hash<9>>>;
                                    
                              
                                        
/**
 * insert
 */                                      
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, HMap, test_types) {
    // insert x values, insert them again, check values
    using key_t = typename HMap::key_type; using value_t = typename HMap:: mapped_type;
    
    const size_t nb_values = 1000;
    HMap map;
    typename HMap::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert({utils::get_key<key_t>(i), utils::get_value<value_t>(i)});
        
        BOOST_CHECK_EQUAL(it->first, utils::get_key<key_t>(i));
        BOOST_CHECK_EQUAL(it->second, utils::get_value<value_t>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert({utils::get_key<key_t>(i), utils::get_value<value_t>(i)});
        
        BOOST_CHECK_EQUAL(it->first, utils::get_key<key_t>(i));
        BOOST_CHECK_EQUAL(it->second, utils::get_value<value_t>(i));
        BOOST_CHECK(!inserted);
    }
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(utils::get_key<key_t>(i));
        
        BOOST_CHECK_EQUAL(it->first, utils::get_key<key_t>(i));
        BOOST_CHECK_EQUAL(it->second, utils::get_value<value_t>(i));
    }
}


// Get nothrow_move_construbtible elements into the overflow list before rehash.
static const unsigned int overflow_mod = 50;
using test_overflow_rehash_types = boost::mpl::list<
                    tsl::hopscotch_map<int64_t, move_only_test, mod_hash<overflow_mod>, std::equal_to<int64_t>, 
                            std::allocator<std::pair<int64_t, move_only_test>>, 6>,
                    tsl::hopscotch_sc_map<int64_t, move_only_test, mod_hash<overflow_mod>, std::equal_to<int64_t>, 
                            std::less<int64_t>, std::allocator<std::pair<int64_t, move_only_test>>, 6>>;                   
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_overflow_rehash_nothrow_move_construbtible, HMap, test_overflow_rehash_types) {    
    // insert x/mod values, insert x values, check values
    static_assert(std::is_nothrow_move_constructible<typename HMap::value_type>::value, "");
    
    HMap map;
    typename HMap::iterator it;
    bool inserted;
    
    
    const size_t nb_values = 5000;
    for(size_t i = 1; i < nb_values; i+= overflow_mod) {
        std::tie(it, inserted) = map.insert({i, move_only_test(i+1)});
        
        BOOST_CHECK_EQUAL(it->first, i);
        BOOST_CHECK_EQUAL(it->second, move_only_test(i+1));
        BOOST_CHECK(inserted);
        
    }
    
    BOOST_CHECK(map.overflow_size() > 0);
    BOOST_CHECK_EQUAL(map.size(), nb_values/overflow_mod);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert({i, move_only_test(i+1)});
        
        BOOST_CHECK_EQUAL(it->first, i);
        BOOST_CHECK_EQUAL(it->second, move_only_test(i+1));
        BOOST_CHECK((i%overflow_mod==1)?!inserted:inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values);
    
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(i);
        
        BOOST_CHECK_EQUAL(it->first, i);
        BOOST_CHECK_EQUAL(it->second, move_only_test(i+1));
    }
}


// Check that the virtual table stays intact. 
BOOST_AUTO_TEST_CASE(test_insert_virtual_table) {
    // insert 2x values with same base class, check values
    const size_t nb_values = 5000;
    
    std::vector<std::unique_ptr<virtual_table_test_base_class>> class_1_elements;
    std::vector<std::unique_ptr<virtual_table_test_base_class>> class_2_elements;
    
    for(size_t i = 0; i < nb_values; i++) {
        class_1_elements.emplace_back(new virtual_table_test_class_1(i));
        class_2_elements.emplace_back(new virtual_table_test_class_2(i));
    }
    
    
    using HMap = tsl::hopscotch_map<virtual_table_test_base_class*, int64_t, mod_hash<9>>;
    HMap::iterator it;
    bool inserted;
    
    
    HMap map(19);
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert({class_1_elements[i].get(), 1});
        BOOST_CHECK(inserted);
        BOOST_CHECK_EQUAL(it->first->value(), class_1_elements[i]->value());
        BOOST_CHECK_NE(it->first->value(), class_2_elements[i]->value());
        BOOST_CHECK_EQUAL(it->second, 1);
        
        std::tie(it, inserted) = map.insert({class_2_elements[i].get(), 2});
        BOOST_CHECK(inserted);
        BOOST_CHECK_EQUAL(it->first->value(), class_2_elements[i]->value());
        BOOST_CHECK_NE(it->first->value(), class_1_elements[i]->value());
        BOOST_CHECK_EQUAL(it->second, 2);
    }
    
    BOOST_CHECK(map.overflow_size() > 0);
    BOOST_CHECK_EQUAL(map.size(), nb_values*2);
    
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(class_1_elements[i].get());
        BOOST_CHECK_EQUAL(it->first->value(), class_1_elements[i]->value());
        BOOST_CHECK_NE(it->first->value(), class_2_elements[i]->value());
        BOOST_CHECK_EQUAL(it->second, 1);
        
        it = map.find(class_2_elements[i].get());
        BOOST_CHECK_EQUAL(it->first->value(), class_2_elements[i]->value());
        BOOST_CHECK_NE(it->first->value(), class_1_elements[i]->value());
        BOOST_CHECK_EQUAL(it->second, 2);
    }
}

BOOST_AUTO_TEST_CASE(test_range_insert) {
    const int nb_values = 1000;
    std::vector<std::pair<int, int>> values;
    for(int i = 0; i < nb_values; i++) {
        values.push_back(std::make_pair(i, i+1));
    }
    
    
    tsl::hopscotch_map<int, int> map = {{-1, 0}, {-2, 0}};
    map.insert(std::next(values.begin(), 10), values.end());
    
    BOOST_CHECK_EQUAL(map.size(), 992);
}


BOOST_AUTO_TEST_CASE(test_insert_with_hint) {
    tsl::hopscotch_map<int, int> map{{1, 0}, {2, 1}, {3, 2}};
    BOOST_CHECK(map.insert(map.find(2), std::make_pair(3, 4)) == map.find(3));
    BOOST_CHECK(map.insert(map.find(2), std::make_pair(2, 4)) == map.find(2));
    BOOST_CHECK(map.insert(map.find(10), std::make_pair(2, 4)) == map.find(2));
    
    BOOST_CHECK_EQUAL(map.size(), 3);
    BOOST_CHECK_EQUAL(map.insert(map.find(10), std::make_pair(4, 3))->first, 4);
    BOOST_CHECK_EQUAL(map.insert(map.find(2), std::make_pair(5, 4))->first, 5);
}


/**
 * try_emplace
 */
BOOST_AUTO_TEST_CASE(test_try_emplace) {
    tsl::hopscotch_map<int64_t, move_only_test> map;
    tsl::hopscotch_map<int64_t, move_only_test>::iterator it;
    bool inserted;
    
    
    std::tie(it, inserted) = map.try_emplace(10, 1);
    BOOST_CHECK_EQUAL(it->first, 10);
    BOOST_CHECK_EQUAL(it->second, move_only_test(1));
    BOOST_CHECK(inserted);
    
    
    std::tie(it, inserted) = map.try_emplace(10, 3);
    BOOST_CHECK_EQUAL(it->first, 10);
    BOOST_CHECK_EQUAL(it->second, move_only_test(1));
    BOOST_CHECK(!inserted);
}


/**
 * insert_or_assign
 */
BOOST_AUTO_TEST_CASE(test_insert_or_assign) {
    tsl::hopscotch_map<int64_t, move_only_test> map;
    tsl::hopscotch_map<int64_t, move_only_test>::iterator it;    
    bool inserted;
    
    
    std::tie(it, inserted) = map.insert_or_assign(10, move_only_test(1));
    BOOST_CHECK_EQUAL(it->first, 10);
    BOOST_CHECK_EQUAL(it->second, move_only_test(1));
    BOOST_CHECK(inserted);
    
    
    std::tie(it, inserted) = map.insert_or_assign(10, move_only_test(3));
    BOOST_CHECK_EQUAL(it->first, 10);
    BOOST_CHECK_EQUAL(it->second, move_only_test(3));
    BOOST_CHECK(!inserted);
}


BOOST_AUTO_TEST_CASE(test_insert_or_assign_hint) {
    tsl::hopscotch_map<int64_t, move_only_test> map(0);
    
    auto it = map.insert_or_assign(map.find(10), 10, move_only_test(1));
    BOOST_CHECK_EQUAL(it->first, 10);
    BOOST_CHECK_EQUAL(it->second, move_only_test(1));
    
    
    it = map.insert_or_assign(map.find(10), 10, move_only_test(3));
    BOOST_CHECK_EQUAL(it->first, 10);
    BOOST_CHECK_EQUAL(it->second, move_only_test(3));
}



/**
 * erase
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_all, HMap, test_types) {
    // insert x values, delete all
    const size_t nb_values = 1000;
    HMap map = utils::get_filled_hash_map<HMap>(nb_values);
    
    auto it = map.erase(map.begin(), map.end());
    BOOST_CHECK(it == map.end());
    BOOST_CHECK(map.empty());
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_loop, HMap, test_types) {
    // insert x values, delete all one by one
    size_t nb_values = 1000;
    HMap map = utils::get_filled_hash_map<HMap>(nb_values);
    HMap map2 = utils::get_filled_hash_map<HMap>(nb_values);
    
    auto it = map.begin();
    // Use second map to check for key after delete as we may not copy the key with move-only types.
    auto it2 = map2.begin();
    while(it != map.end()) {
        it = map.erase(it);
        --nb_values;
        
        BOOST_CHECK_EQUAL(map.count(it2->first), 0);
        BOOST_CHECK_EQUAL(map.size(), nb_values);
        ++it2;
    }
    
    BOOST_CHECK(map.empty());
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert_erase_insert, HMap, test_types) {
    // insert x/2 values, delete x/4 values, insert x/2 values, find each value
    using key_t = typename HMap::key_type; using value_t = typename HMap:: mapped_type;
    
    const size_t nb_values = 2000;
    HMap map;
    typename HMap::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values/2; i++) {
        std::tie(it, inserted) = map.insert({utils::get_key<key_t>(i), utils::get_value<value_t>(i)});
        
        BOOST_CHECK_EQUAL(it->first, utils::get_key<key_t>(i));
        BOOST_CHECK_EQUAL(it->second, utils::get_value<value_t>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/2);
    
    
    // Delete half
    for(size_t i = 0; i < nb_values/2; i++) {
        if(i%2 == 0) {
            BOOST_CHECK_EQUAL(map.erase(utils::get_key<key_t>(i)), 1);
        }
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/4);
    
    
    for(size_t i = nb_values/2; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert({utils::get_key<key_t>(i), utils::get_value<value_t>(i)});
        
        BOOST_CHECK_EQUAL(it->first, utils::get_key<key_t>(i));
        BOOST_CHECK_EQUAL(it->second, utils::get_value<value_t>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values-nb_values/4);
    
    for(size_t i = 0; i < nb_values; i++) {
        if(i%2 == 0 && i < nb_values/2) {
            it = map.find(utils::get_key<key_t>(i));
            
            BOOST_CHECK(it == map.cend());
        }
        else {
            it = map.find(utils::get_key<key_t>(i));
            
            BOOST_CHECK_EQUAL(it->first, utils::get_key<key_t>(i));
            BOOST_CHECK_EQUAL(it->second, utils::get_value<value_t>(i));
        }
    }
}

BOOST_AUTO_TEST_CASE(test_range_erase_same_iterators) {
    const size_t nb_values = 100;
    auto map = utils::get_filled_hash_map<tsl::hopscotch_map<int64_t, int64_t>>(nb_values);
    
    tsl::hopscotch_map<int64_t, int64_t>::const_iterator it_const = map.cbegin();
    std::advance(it_const, 10);
    
    tsl::hopscotch_map<int64_t, int64_t>::iterator it_mutable = map.erase(it_const, it_const);
    BOOST_CHECK(it_const == it_mutable);
    BOOST_CHECK_EQUAL(map.size(), 100);
    
    it_mutable.value() = -100;
    BOOST_CHECK_EQUAL(it_const.value(), -100);
}


/**
 * operator== and operator!=
 */
BOOST_AUTO_TEST_CASE_TEMPLATE(test_compare, HMap, test_types) {
    // create 3 maps, 2 are the same, compare maps
    using key_t = typename HMap::key_type; using value_t = typename HMap:: mapped_type;
    
    const size_t nb_values = 1000;
    HMap map_1_1;
    HMap map_1_2;
    HMap map_2_1;
    
    for(size_t i = 0; i < nb_values; i++) {
        map_1_1.insert({utils::get_key<key_t>(i), utils::get_value<value_t>(i)});
        if(i != 0) {
            map_2_1.insert({utils::get_key<key_t>(i), utils::get_value<value_t>(i)});
        }
    }
    
    // Same as map_1_1 but insertion order inverted
    for(size_t i = nb_values; i != 0; i--) {
        map_1_2.insert({utils::get_key<key_t>(i-1), utils::get_value<value_t>(i-1)});
    }
    
    
    BOOST_CHECK_EQUAL(map_1_1.size(), nb_values);
    BOOST_CHECK_EQUAL(map_1_2.size(), nb_values);
    BOOST_CHECK_EQUAL(map_2_1.size(), nb_values-1);
    
    BOOST_CHECK(map_1_1 == map_1_2);
    BOOST_CHECK(map_1_2 == map_1_1);
    
    BOOST_CHECK(map_1_1 != map_2_1);
    BOOST_CHECK(map_2_1 != map_1_1);
    
    BOOST_CHECK(map_1_2 != map_2_1);
    BOOST_CHECK(map_2_1 != map_1_2);
}




/**
 * clear
 */
BOOST_AUTO_TEST_CASE(test_clear) {
    // insert x values, clear map
    const size_t nb_values = 1000;
    auto map = utils::get_filled_hash_map<tsl::hopscotch_map<int64_t, int64_t>>(nb_values);
    
    map.clear();
    BOOST_CHECK_EQUAL(map.size(), 0);
    BOOST_CHECK_EQUAL(std::distance(map.begin(), map.end()), 0);
}




/**
 * iterator.value()
 */
BOOST_AUTO_TEST_CASE(test_modify_value) {
    // insert x values, modify value of even keys, check values
    const size_t nb_values = 100;
    auto map = utils::get_filled_hash_map<tsl::hopscotch_map<int64_t, int64_t>>(nb_values);
    
    for(auto it = map.begin(); it != map.end(); ++it) {
        if(it->first % 2 == 0) {
            it.value() = -1;
        }
    }
    
    for(auto& val : map) {
        if(val.first % 2 == 0) {
            BOOST_CHECK_EQUAL(val.second, -1);
        }
        else {
            BOOST_CHECK_NE(val.second, -1);
        }
    }
}




/**
 * operator=
 */
BOOST_AUTO_TEST_CASE(test_assign_operator) {
    tsl::hopscotch_map<int64_t, int64_t> map = {{0, 10}, {-2, 20}};
    BOOST_CHECK_EQUAL(map.size(), 2);
    
    map = {{1, 3}};
    BOOST_CHECK_EQUAL(map.size(), 1);
    BOOST_CHECK_EQUAL(map.at(1), 3);
}


BOOST_AUTO_TEST_CASE(test_reassign_moved_object_move_constructor) {
    tsl::hopscotch_map<int64_t, int64_t> map = {{1, 1}, {2, 1}, {3, 1}};
    BOOST_CHECK_EQUAL(map.size(), 3);
    
    tsl::hopscotch_map<int64_t, int64_t> map_move(std::move(map));
    BOOST_CHECK_EQUAL(map_move.size(), 3);
    BOOST_CHECK_EQUAL(map.size(), 0);
    
    BOOST_CHECK(map_move == (tsl::hopscotch_map<int64_t, int64_t>({{1, 1}, {2, 1}, {3, 1}})));
    BOOST_CHECK(map == (tsl::hopscotch_map<int64_t, int64_t>()));
    
    map = {{4, 1}, {5, 1}};
    BOOST_CHECK_EQUAL(map.size(), 2);
    BOOST_CHECK(map == (tsl::hopscotch_map<int64_t, int64_t>({{4, 1}, {5, 1}})));
}

BOOST_AUTO_TEST_CASE(test_reassign_moved_object_move_operator) {
    tsl::hopscotch_map<int64_t, int64_t> map = {{1, 1}, {2, 1}, {3, 1}};
    BOOST_CHECK_EQUAL(map.size(), 3);
    
    tsl::hopscotch_map<int64_t, int64_t> map_move = std::move(map);
    BOOST_CHECK_EQUAL(map_move.size(), 3);
    BOOST_CHECK_EQUAL(map.size(), 0);
    
    BOOST_CHECK(map_move == (tsl::hopscotch_map<int64_t, int64_t>({{1, 1}, {2, 1}, {3, 1}})));
    BOOST_CHECK(map == (tsl::hopscotch_map<int64_t, int64_t>()));
    
    map = {{4, 1}, {5, 1}};
    BOOST_CHECK_EQUAL(map.size(), 2);
    BOOST_CHECK(map == (tsl::hopscotch_map<int64_t, int64_t>({{4, 1}, {5, 1}})));
}




/**
 * at
 */
BOOST_AUTO_TEST_CASE(test_at) {
    // insert x values, use at for known and unknown values.
    tsl::hopscotch_map<int64_t, int64_t> map = {{0, 10}, {-2, 20}};
    
    BOOST_CHECK_EQUAL(map.at(0), 10);
    BOOST_CHECK_EQUAL(map.at(-2), 20);
    BOOST_CHECK_THROW(map.at(1), std::out_of_range);
}




/**
 * operator[]
 */
BOOST_AUTO_TEST_CASE(test_access_operator) {
    // insert x values, use at for known and unknown values.
    tsl::hopscotch_map<int64_t, int64_t> map = {{0, 10}, {-2, 20}};
    
    BOOST_CHECK_EQUAL(map[0], 10);
    BOOST_CHECK_EQUAL(map[-2], 20);
    BOOST_CHECK_EQUAL(map[2], int64_t());
    
    BOOST_CHECK_EQUAL(map.size(), 3);
}



/**
 * swap
 */
BOOST_AUTO_TEST_CASE(test_swap) {
    tsl::hopscotch_map<int64_t, int64_t> map = {{1, 10}, {8, 80}, {3, 30}};
    tsl::hopscotch_map<int64_t, int64_t> map2 = {{4, 40}, {5, 50}};
    
    using std::swap;
    swap(map, map2);
    
    BOOST_CHECK(map == (tsl::hopscotch_map<int64_t, int64_t>{{4, 40}, {5, 50}}));
    BOOST_CHECK(map2 == (tsl::hopscotch_map<int64_t, int64_t>{{1, 10}, {8, 80}, {3, 30}}));
}






/**
 * other
 */
BOOST_AUTO_TEST_CASE(test_heterogeneous_lookups) {
    struct hash_ptr {
        size_t operator()(const std::unique_ptr<int>& p) const {
            return std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(p.get()));
        }
        
        size_t operator()(uintptr_t p) const {
            return std::hash<uintptr_t>()(p);
        }
        
        size_t operator()(const int* const& p) const {
            return std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(p));
        }
    };
    
    struct equal_to_ptr {
        using is_transparent = std::true_type;
        
        bool operator()(const std::unique_ptr<int>& p1, const std::unique_ptr<int>& p2) const {
            return p1 == p2;
        }
        
        bool operator()(const std::unique_ptr<int>& p1, uintptr_t p2) const {
            return reinterpret_cast<uintptr_t>(p1.get()) == p2;
        }
        
        bool operator()(uintptr_t p1, const std::unique_ptr<int>& p2) const {
            return p1 == reinterpret_cast<uintptr_t>(p2.get());
        }
        
        bool operator()(const std::unique_ptr<int>& p1, const int* const& p2) const {
            return p1.get() == p2;
        }
        
        bool operator()(const int* const& p1, const std::unique_ptr<int>& p2) const {
            return p1 == p2.get();
        }
    };
    
    std::unique_ptr<int> ptr1(new int(1));
    std::unique_ptr<int> ptr2(new int(2));
    std::unique_ptr<int> ptr3(new int(3));
    int other;
    
    const uintptr_t addr1 = reinterpret_cast<uintptr_t>(ptr1.get());
    const int* const addr2 = ptr2.get();
    const int* const addr_unknown = &other;
     
    tsl::hopscotch_map<std::unique_ptr<int>, int, hash_ptr, equal_to_ptr> map;
    map.insert({std::move(ptr1), 4});
    map.insert({std::move(ptr2), 5});
    map.insert({std::move(ptr3), 6});
    
    BOOST_CHECK_EQUAL(map.size(), 3);
    
    
    BOOST_CHECK_EQUAL(map.at(addr1), 4);
    BOOST_CHECK_EQUAL(map.at(addr2), 5);
    BOOST_CHECK_THROW(map.at(addr_unknown), std::out_of_range);
    
    
    BOOST_CHECK_EQUAL(*map.find(addr1)->first, 1);
    BOOST_CHECK_EQUAL(*map.find(addr2)->first, 2);
    BOOST_CHECK(map.find(addr_unknown) == map.end());
    
    
    BOOST_CHECK_EQUAL(map.count(addr1), 1);
    BOOST_CHECK_EQUAL(map.count(addr2), 1);
    BOOST_CHECK_EQUAL(map.count(addr_unknown), 0);
    
    
    BOOST_CHECK_EQUAL(map.erase(addr1), 1);
    BOOST_CHECK_EQUAL(map.erase(addr2), 1);
    BOOST_CHECK_EQUAL(map.erase(addr_unknown), 0);
    
    
    BOOST_CHECK_EQUAL(map.size(), 1);
}
