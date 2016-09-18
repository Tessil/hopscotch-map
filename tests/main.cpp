#define BOOST_TEST_MODULE test_hopscotch_map
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "utils.h"
#include "hopscotch_map.h"


using test_types = boost::mpl::list<hopscotch_map<int64_t, int64_t>, 
                                    hopscotch_map<int64_t, int64_t, std::hash<int64_t>, std::equal_to<int64_t>, 6>, 
                                    // Test with hash having a lot of collisions
                                    hopscotch_map<int64_t, int64_t, mod_hash<9>>,
                                    hopscotch_map<int64_t, int64_t, mod_hash<9>, std::equal_to<int64_t>, 6>, 
                                    hopscotch_map<std::string, std::string>,
                                    hopscotch_map<std::string, std::string, mod_hash<9>>,
                                    hopscotch_map<std::string, std::string, mod_hash<9>, std::equal_to<std::string>, 6>,
                                    hopscotch_map<int64_t, std::string>,
                                    hopscotch_map<int64_t, move_only_test>,
                                    hopscotch_map<int64_t, move_only_test, mod_hash<9>, std::equal_to<int64_t>, 6>,
                                    hopscotch_map<self_reference_member_test, self_reference_member_test>,
                                    hopscotch_map<self_reference_member_test, self_reference_member_test, 
                                                  mod_hash<9>, std::equal_to<self_reference_member_test>, 6>>;
                                    
                              
                                    
                                    
                                             
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, HMap, test_types) {
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

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_all, HMap, test_types) {
    const size_t nb_values = 1000;
    HMap map = utils::get_filled_hash_map<HMap>(nb_values);
    
    auto it = map.erase(map.begin(), map.end());
    BOOST_CHECK(it == map.end());
    BOOST_CHECK(map.empty());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_erase_loop, HMap, test_types) {
    size_t nb_values = 1000;
    HMap map = utils::get_filled_hash_map<HMap>(nb_values);
    
    auto it = map.begin();
    while(it != map.end()) {
        auto key = it->first;
        
        it = map.erase(it);
        --nb_values;
        BOOST_CHECK_EQUAL(map.count(key), 0);
        BOOST_CHECK_EQUAL(map.size(), nb_values);
    }
    
    BOOST_CHECK(map.empty());
}


BOOST_AUTO_TEST_CASE_TEMPLATE(test_iterator, HMap, test_types) {
    using key_t = typename HMap::key_type; using value_t = typename HMap:: mapped_type;
    
    const size_t nb_values = 1000;
    const HMap map = utils::get_filled_hash_map<HMap>(nb_values);
    BOOST_CHECK_EQUAL(std::distance(map.begin(), map.end()), nb_values);
    BOOST_CHECK_EQUAL(std::distance(map.cbegin(), map.cend()), nb_values);
    
    
    HMap map_tmp1 = utils::get_filled_hash_map<HMap>(nb_values);
    const std::map<key_t, value_t> sorted(std::make_move_iterator(map_tmp1.begin()), std::make_move_iterator(map_tmp1.end()));
    
    HMap map_tmp2 = utils::get_filled_hash_map<HMap>(nb_values);
    const std::map<key_t, value_t> sorted2(std::make_move_iterator(map_tmp2.begin()), std::make_move_iterator(map_tmp2.end()));
    
    
    BOOST_CHECK(sorted == sorted2);
    BOOST_CHECK_EQUAL(sorted.size(), map.size());
    
    for(const auto & key_value : sorted) {
        auto it_find = map.find(key_value.first);
        BOOST_CHECK_EQUAL(key_value.first, it_find->first);
        BOOST_CHECK_EQUAL(key_value.second, it_find->second);
    }
}


BOOST_AUTO_TEST_CASE(test_clear) {
    const size_t nb_values = 1000;
    auto map = utils::get_filled_hash_map<hopscotch_map<int64_t, int64_t>>(nb_values);
    
    map.clear();
    BOOST_CHECK_EQUAL(map.size(), 0);
    BOOST_CHECK_EQUAL(std::distance(map.begin(), map.end()), 0);
}

                                      
BOOST_AUTO_TEST_CASE_TEMPLATE(test_compare, HMap, test_types) {
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

/*
 * Get nothrow_move_construbtible elements into the overflow list before rehash.
 */
BOOST_AUTO_TEST_CASE(test_insert_overflow_rehash_nothrow_move_construbtible) {
    static const size_t mod = 100;
    using HMap = hopscotch_map<int64_t, move_only_test, mod_hash<mod>, std::equal_to<int64_t>, 6>;
    static_assert(std::is_nothrow_move_constructible<HMap::value_type>::value, "");
    
    HMap map;
    HMap::iterator it;
    bool inserted;
    
    
    const size_t nb_values = 5000;
    for(size_t i = 1; i < nb_values; i+= mod) {
        std::tie(it, inserted) = map.insert({i, i+1});
        
        BOOST_CHECK_EQUAL(it->first, i);
        BOOST_CHECK_EQUAL(it->second, i+1);
        BOOST_CHECK(inserted);
        
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values/mod);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = map.insert({i, i+1});
        
        BOOST_CHECK_EQUAL(it->first, i);
        BOOST_CHECK_EQUAL(it->second, i+1);
        BOOST_CHECK((i%mod==1)?!inserted:inserted);
    }
    BOOST_CHECK_EQUAL(map.size(), nb_values);
    
    
    for(size_t i = 0; i < nb_values; i++) {
        it = map.find(i);
        
        BOOST_CHECK_EQUAL(it->first, i);
        BOOST_CHECK_EQUAL(it->second, i+1);
    }
}


/*
 * Check that the virtual table stays intact. 
 */
BOOST_AUTO_TEST_CASE(test_virtual_table) {
    const size_t nb_values = 5000;
    
    std::vector<std::unique_ptr<virtual_table_test_base_class>> class_1_elements;
    std::vector<std::unique_ptr<virtual_table_test_base_class>> class_2_elements;
    
    for(size_t i = 0; i < nb_values; i++) {
        class_1_elements.emplace_back(new virtual_table_test_class_1(i));
        class_2_elements.emplace_back(new virtual_table_test_class_2(i));
    }
    
    
    using HMap = hopscotch_map<virtual_table_test_base_class*, int64_t, mod_hash<9>>;
    HMap::iterator it;
    bool inserted;
    
    
    HMap map;
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
