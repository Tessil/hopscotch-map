#ifndef __T_UTILS_H__
#define __T_UTILS_H__

#include <boost/numeric/conversion/cast.hpp>

template<unsigned int MOD>
class mod_hash {
public:   
    template<typename T>
    size_t operator()(T value) const {
        return std::hash<T>()(value) % MOD;
    }
};


class utils {
public:
    template<typename T>
    static T get_key(size_t counter);
    
    template<typename T>
    static T get_value(size_t counter);
    
    template<typename HMap>
    static HMap get_filled_hash_map(size_t nb_elements);
};



template<>
int64_t utils::get_key<int64_t>(size_t counter) {
    return boost::numeric_cast<int64_t>(counter);
}

template<>
std::string utils::get_key<std::string>(size_t counter) {
    return "Key " + std::to_string(counter);
}

template<>
int64_t utils::get_value<int64_t>(size_t counter) {
    return boost::numeric_cast<int64_t>(counter*2);
}

template<>
std::string utils::get_value<std::string>(size_t counter) {
    return "Value " + std::to_string(counter);
}


template<typename HMap>
HMap utils::get_filled_hash_map(size_t nb_elements) {
    using key_t = typename HMap::key_type; using value_t = typename HMap:: mapped_type;
    
    HMap map;
    for(size_t i = 0; i < nb_elements; i++) {
        map.insert({utils::get_key<key_t>(i), utils::get_value<value_t>(i)});
    }
    
    return map;
}

#endif
