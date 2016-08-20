#define BOOST_TEST_MODULE test_hopscotch_map
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "utils.h"
#include "hopscotch_map.h"




using test_types = boost::mpl::list<hopscotch_map<int64_t, int64_t>, 
                                    hopscotch_map<int64_t, int64_t, std::hash<int64_t>, std::equal_to<int64_t>, uint8_t>, 
                                    // Test with hash having lot of collision
                                    hopscotch_map<int64_t, int64_t, mod_hash<9>>,
                                    hopscotch_map<int64_t, int64_t, mod_hash<9>, std::equal_to<int64_t>, uint8_t>, 
                                    hopscotch_map<std::string, std::string>,
                                    hopscotch_map<std::string, std::string, std::hash<std::string>, std::equal_to<std::string>, uint64_t>,
                                    hopscotch_map<std::string, std::string, mod_hash<9>>,
                                    hopscotch_map<std::string, std::string, mod_hash<9>, std::equal_to<std::string>, uint64_t>,
                                    hopscotch_map<int64_t, std::string>>;
                              
                                    
                                    
                                             
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
    
    const std::map<key_t, value_t> sorted(map.cbegin(), map.cend());
    const std::map<key_t, value_t> sorted2(map.begin(), map.end());
    
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

