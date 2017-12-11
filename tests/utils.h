#ifndef TSL_UTILS_H
#define TSL_UTILS_H


#include <boost/numeric/conversion/cast.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <utility>


template<unsigned int MOD>
class mod_hash {
public:   
    template<typename T>
    size_t operator()(const T& value) const {
        return std::hash<T>()(value) % MOD;
    }
};

class self_reference_member_test {
public:
    self_reference_member_test() : m_value(-1), m_value_ptr(&m_value) {
    }
    
    explicit self_reference_member_test(int64_t value) : m_value(value), m_value_ptr(&m_value) {
    }
    
    self_reference_member_test(const self_reference_member_test& other) : m_value(*other.m_value_ptr), m_value_ptr(&m_value) {
    }
    
    self_reference_member_test(self_reference_member_test&& other) : m_value(*other.m_value_ptr), m_value_ptr(&m_value) {
    }
    
    self_reference_member_test& operator=(const self_reference_member_test& other) {
        m_value = *other.m_value_ptr;
        m_value_ptr = &m_value;
        
        return *this;
    }
    
    self_reference_member_test& operator=(self_reference_member_test&& other) {
        m_value = *other.m_value_ptr;
        m_value_ptr = &m_value;
        
        return *this;
    }
    
    int64_t value() const {
        return *m_value_ptr;
    }

    friend std::ostream& operator<<(std::ostream& stream, const self_reference_member_test& value) {
        stream << *value.m_value_ptr;
        
        return stream;
    }
    
    friend bool operator==(const self_reference_member_test& lhs, const self_reference_member_test& rhs) { 
        return *lhs.m_value_ptr == *rhs.m_value_ptr;
    }
    
    friend bool operator!=(const self_reference_member_test& lhs, const self_reference_member_test& rhs) { 
        return !(lhs == rhs); 
    }
    
    friend bool operator<(const self_reference_member_test& lhs, const self_reference_member_test& rhs) {
        return *lhs.m_value_ptr < *rhs.m_value_ptr;
    }
private:    
    int64_t  m_value;
    int64_t* m_value_ptr;
};



class move_only_test {
public:
    explicit move_only_test(int64_t value) : m_value(new int64_t(value)) {
    }
    
    move_only_test(const move_only_test&) = delete;
    move_only_test(move_only_test&&) = default;
    move_only_test& operator=(const move_only_test&) = delete;
    move_only_test& operator=(move_only_test&&) = default;
    
    friend std::ostream& operator<<(std::ostream& stream, const move_only_test& value) {
        if(value.m_value == nullptr) {
            stream << "null";
        }
        else {
            stream << *value.m_value;
        }
        
        return stream;
    }
    
    friend bool operator==(const move_only_test& lhs, const move_only_test& rhs) { 
        if(lhs.m_value == nullptr || rhs.m_value == nullptr) {
            return lhs.m_value == nullptr && rhs.m_value == nullptr;
        }
        else {
            return *lhs.m_value == *rhs.m_value; 
        }
    }
    
    friend bool operator!=(const move_only_test& lhs, const move_only_test& rhs) { 
        return !(lhs == rhs); 
    }
    
    friend bool operator<(const move_only_test& lhs, const move_only_test& rhs) {
        if(lhs.m_value == nullptr && rhs.m_value == nullptr) {
            return false;
        }
        else if(lhs.m_value == nullptr) {
            return true;
        }
        else if(rhs.m_value == nullptr) {
            return false;
        }
        else {
            return *lhs.m_value < *rhs.m_value; 
        }
    }
    
    int64_t value() const {
        return *m_value;
    }
private:    
    std::unique_ptr<int64_t> m_value;
};


namespace std {
    template<>
    struct hash<self_reference_member_test> {
        size_t operator()(const self_reference_member_test& val) const {
            return std::hash<int64_t>()(val.value());
        }
    };
    
    template<>
    struct hash<move_only_test> {
        size_t operator()(const move_only_test& val) const {
            return std::hash<int64_t>()(val.value());
        }
    };
}


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
inline int64_t utils::get_key<int64_t>(size_t counter) {
    return boost::numeric_cast<int64_t>(counter);
}

template<>
inline self_reference_member_test utils::get_key<self_reference_member_test>(size_t counter) {
    return self_reference_member_test(boost::numeric_cast<int64_t>(counter));
}

template<>
inline std::string utils::get_key<std::string>(size_t counter) {
    return "Key " + std::to_string(counter);
}

template<>
inline move_only_test utils::get_key<move_only_test>(size_t counter) {
    return move_only_test(boost::numeric_cast<int64_t>(counter));
}




template<>
inline int64_t utils::get_value<int64_t>(size_t counter) {
    return boost::numeric_cast<int64_t>(counter*2);
}

template<>
inline self_reference_member_test utils::get_value<self_reference_member_test>(size_t counter) {
    return self_reference_member_test(boost::numeric_cast<int64_t>(counter*2));
}

template<>
inline std::string utils::get_value<std::string>(size_t counter) {
    return "Value " + std::to_string(counter);
}

template<>
inline move_only_test utils::get_value<move_only_test>(size_t counter) {
    return move_only_test(boost::numeric_cast<int64_t>(counter*2));
}



template<typename HMap>
inline HMap utils::get_filled_hash_map(size_t nb_elements) {
    using key_t = typename HMap::key_type; using value_t = typename HMap:: mapped_type;
    
    HMap map;
    map.reserve(nb_elements);
    
    for(size_t i = 0; i < nb_elements; i++) {
        map.insert({utils::get_key<key_t>(i), utils::get_value<value_t>(i)});
    }
    
    return map;
}

#endif
