#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>

#include "utils.h"
#include "hopscotch_set.h"
#include "hopscotch_sc_set.h"


using test_types = boost::mpl::list<tsl::hopscotch_set<int64_t>,
                                    tsl::hopscotch_set<self_reference_member_test>,
                                    tsl::hopscotch_set<move_only_test>,
                                    tsl::hopscotch_sc_set<int64_t, mod_hash<9>>,
                                    tsl::hopscotch_sc_set<self_reference_member_test, mod_hash<9>>,
                                    tsl::hopscotch_sc_set<move_only_test, mod_hash<9>>>;
                                    
                              
                                    
                           
                                             
BOOST_AUTO_TEST_CASE_TEMPLATE(test_insert, HSet, test_types) {
    // insert x values, insert them again, check values
    using key_t = typename HSet::key_type;
    
    const size_t nb_values = 1000;
    HSet set;
    typename HSet::iterator it;
    bool inserted;
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = set.insert(utils::get_key<key_t>(i));
        
        BOOST_CHECK_EQUAL(*it, utils::get_key<key_t>(i));
        BOOST_CHECK(inserted);
    }
    BOOST_CHECK_EQUAL(set.size(), nb_values);
    
    for(size_t i = 0; i < nb_values; i++) {
        std::tie(it, inserted) = set.insert(utils::get_key<key_t>(i));
        
        BOOST_CHECK_EQUAL(*it, utils::get_key<key_t>(i));
        BOOST_CHECK(!inserted);
    }
    
    for(size_t i = 0; i < nb_values; i++) {
        it = set.find(utils::get_key<key_t>(i));
        
        BOOST_CHECK_EQUAL(*it, utils::get_key<key_t>(i));
    }
}
