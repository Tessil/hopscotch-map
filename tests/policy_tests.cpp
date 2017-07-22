#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp> 
#include <cstddef>
#include <limits>
#include <stdexcept>

#include "hopscotch_hash.h"


BOOST_AUTO_TEST_SUITE(test_policy)

BOOST_AUTO_TEST_CASE(test_power_of_two_policy) {
    // Call next_bucket_count() on polcy until we reach its max_bucket_count()
    bool exception_thrown = false;
    
    std::size_t bucket_count = 0;
    tsl::power_of_two_growth_policy policy(bucket_count);
    
    try {
        while(true) {
            bucket_count = policy.next_bucket_count();
            policy = tsl::power_of_two_growth_policy(bucket_count);
        }
    }
    catch(const std::length_error& ) {
        exception_thrown = true;
        BOOST_CHECK_EQUAL(bucket_count, policy.max_bucket_count());
    }
    
    BOOST_CHECK(exception_thrown);
}

BOOST_AUTO_TEST_CASE(test_power_of_two_policy_min_size) {
    std::size_t size = 0;
    tsl::power_of_two_growth_policy policy(size);
    
    BOOST_CHECK_EQUAL(policy.bucket_for_hash(0), 0);
}

BOOST_AUTO_TEST_CASE(test_power_of_two_policy_max_size) {
    std::size_t size = 0;
    tsl::power_of_two_growth_policy policy(size);
    
    
    size = policy.max_bucket_count();
    tsl::power_of_two_growth_policy policy2(size);
    
    
    size = std::numeric_limits<std::size_t>::max();
    BOOST_CHECK_THROW((tsl::power_of_two_growth_policy(size)), std::length_error);
    
    
    size = policy.max_bucket_count() + 1;
    BOOST_CHECK_THROW((tsl::power_of_two_growth_policy(size)), std::length_error);
}




BOOST_AUTO_TEST_CASE(test_prime_policy) {
    // Call next_bucket_count() on polcy until we reach its max_bucket_count()
    bool exception_thrown = false;
    
    std::size_t bucket_count = 0;
    tsl::prime_growth_policy policy(bucket_count);
    
    try {
        while(true) {
            bucket_count = policy.next_bucket_count();
            policy = tsl::prime_growth_policy(bucket_count);
        }
    }
    catch(const std::length_error& ) {
        exception_thrown = true;
        BOOST_CHECK_EQUAL(bucket_count, policy.max_bucket_count());
    }
    
    BOOST_CHECK(exception_thrown);
}

BOOST_AUTO_TEST_CASE(test_prime_policy_min_size) {
    std::size_t size = 0;
    tsl::prime_growth_policy policy(size);
    
    BOOST_CHECK_EQUAL(policy.bucket_for_hash(0), 0);
}

BOOST_AUTO_TEST_CASE(test_prime_policy_max_size) {
    std::size_t size = 0;
    tsl::prime_growth_policy policy(size);
    
    
    size = policy.max_bucket_count();
    tsl::prime_growth_policy policy2(size);
    
    
    size = std::numeric_limits<std::size_t>::max();
    BOOST_CHECK_THROW((tsl::prime_growth_policy(size)), std::length_error);
    
    
    size = policy.max_bucket_count() + 1;
    BOOST_CHECK_THROW((tsl::prime_growth_policy(size)), std::length_error);
}




BOOST_AUTO_TEST_CASE(test_mod_policy) {
    // Call next_bucket_count() on polcy until we reach its max_bucket_count()
    bool exception_thrown = false;
    
    std::size_t bucket_count = 0;
    tsl::mod_growth_policy<> policy(bucket_count);
    
    try {
        while(true) {
            bucket_count = policy.next_bucket_count();
            policy = tsl::mod_growth_policy<>(bucket_count);
        }
    }
    catch(const std::length_error& ) {
        exception_thrown = true;
        BOOST_CHECK_EQUAL(bucket_count, policy.max_bucket_count());
    }
    
    BOOST_CHECK(exception_thrown);
}

BOOST_AUTO_TEST_CASE(test_mod_policy_min_size) {
    std::size_t size = 0;
    tsl::mod_growth_policy<> policy(size);
    
    BOOST_CHECK_EQUAL(policy.bucket_for_hash(0), 0);
}

BOOST_AUTO_TEST_CASE(test_mod_policy_max_size) {
    std::size_t size = 0;
    tsl::mod_growth_policy<> policy(size);
    
    size = policy.max_bucket_count();
    tsl::mod_growth_policy<> policy2(size);
    
    
    size = std::numeric_limits<std::size_t>::max();
    BOOST_CHECK_THROW((tsl::mod_growth_policy<>(size)), std::length_error);
    
    
    size = policy.max_bucket_count() + 1;
    BOOST_CHECK_THROW((tsl::mod_growth_policy<>(size)), std::length_error);
}


BOOST_AUTO_TEST_SUITE_END()
