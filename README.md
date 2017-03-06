[![Build Status](https://travis-ci.org/Tessil/hopscotch-map.svg?branch=master)](https://travis-ci.org/Tessil/hopscotch-map) [![Build status](https://ci.appveyor.com/api/projects/status/e97rjkcn3qwrhpvf/branch/master?svg=true)](https://ci.appveyor.com/project/Tessil/hopscotch-map/branch/master)

## A C++ implementation of a fast hash map using hopscotch hashing
The hopscotch-map library is a C++ implementation of a single-thread hash map and hash set using hopscotch hashing. It offers good performances if all the elements of the key used by the equal function are contiguous in memory (no pointers to other parts of the memory that may cause a cache-miss) thanks to its good cache locality. The number of memory allocations are also small, they only happen on rehash or if there is an overflow (see [implementation details](https://tessil.github.io/2016/08/29/hopscotch-hashing.html)), allowing hopscotch-map to do fast inserts. It may be a good alternative to `std::unordered_map` in some cases and is mainly a concurrent to `google::dense_hash_map`, it trades off some memory space (if the key-value pair is big, otherwise it has the advantage compare to `std::unordered_map`) to have a fast hash table.

The library provides two classes: `tsl::hopscotch_map` and `tsl::hopscotch_set`.

An overview of hopscotch hashing and some implementation details may be found [here](https://tessil.github.io/2016/08/29/hopscotch-hashing.html).

A **benchmark** of `tsl::hopscotch_map` against other hash maps may be found [there](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html).

### Key features
- Header-only library, just include [src/](src/) to your include path and you're ready to go.
- Fast, see [benchmark](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html) for some numbers.
- Support for move-only and non-default constructible key/value.
- Support for heterogeneous lookups (e.g. if you have a map that uses `std::unique_ptr<int>` as key, you could use an `int*` or a `std::uintptr_t` for example as key parameter for `find`, see [example](https://github.com/Tessil/hopscotch-map#heterogeneous-lookup)).
- No need to reserve any sentinel value from key.
- API closely similar to `std::unordered_map` and `std::unordered_set`.

### Differences compare to `std::unordered_map`
`tsl::hopscotch_map` tries to have an interface similar to `std::unordered_map`, but some differences exist:
- Iterator invalidation on insert doesn't behave in the same way (see [API](https://tessil.github.io/hopscotch-map/doc/html/classhopscotch__map.html#details) for details).
- References and pointers to keys or values in the map are invalidated in the same way as iterators to these keys-values.
- The size of the bucket array in the map grows by a factor of 2, the size will always be a power of 2, which may be a too steep growth rate for some purposes. The growth factor is modifiable (see the GrowthFactor template parameter) but it may reduce the speed of the hash map if it is not a power of two.
- For iterators, `operator*()` and `operator->()` return a reference and a pointer to `const std::pair<Key, T>` instead of `std::pair<const Key, T>` making the value `T` not modifiable. To modify the value you have to call the `value()` method of the iterator to get a mutable reference. Example:
```c++
tsl::hopscotch_map<int, int> map = {{1, 1}, {2, 1}, {3, 1}};
for(auto it = map.begin(); it != map.end(); ++it) {
    //it->second = 2; // Illegal
    it.value() = 2; // Ok
}
```
- No support for some bucket related methods (like bucket_size, bucket, ...).
- No support for move-only types with a move constructor that may throw an exception (with open adressing, it's not possible to keep the strong exception guarantee on rehash if the move constructor may throw).

These differences also apply between `std::unordered_set` and `tsl::hopscotch_set`.

### Differences compare to `google::dense_hash_map`
`tsl::hopscotch_map` has comparable performances to `google::dense_hash_map` (see [benchmark](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html)), but come with some advantages:
- There is no need to reserve sentinel values for the key as it is done by `google::dense_hash_map` where you need to have a sentinel for empty and deleted keys.
- The type of the value in the map doesn't need a default constructor.
- The key and the value of the map don't need a copy constructor/operator, move-only types are supported.
- It uses less memory for its speed as it can sustain a load factor of 0.95 (which is the default value in the library compare to the 0.5 of `google::dense_hash_map`) while keeping good performances.

### Installation
To use hopscotch-map, just add the [src/](src/) directory to your include path. It's a **header-only** library.

The code should work with any C++11 standard-compliant compiler and has been tested with GCC 4.8.4, Clang 3.5.0 and Visual Studio 2015.

To run the tests you will need the Boost Test library and CMake. 

```bash
git clone https://github.com/Tessil/hopscotch-map.git
cd hopscotch-map
mkdir build
cd build
cmake ..
make
./test_hopscotch_map 
```


### Usage
The API can be found [here](https://tessil.github.io/hopscotch-map/doc/html/). 

All methods are not documented yet, but they replicate the behaviour of the ones in `std::unordered_map` and `std::unordered_set`, except if specified otherwise.

### Example
```c++
#include <cstdint>
#include <iostream>
#include <string>
#include "hopscotch_map.h"
#include "hopscotch_set.h"

int main() {
    tsl::hopscotch_map<std::string, int> map = {{"a", 1}, {"b", 2}};
    map["c"] = 3;
    map["d"] = 4;
    
    map.insert({"e", 5});
    map.erase("b");
    
    for(auto it = map.begin(); it != map.end(); ++it) {
        //it->second += 2; // Not valid.
        it.value() += 2;
    }
    
    // {d, 6} {a, 3} {e, 7} {c, 5}
    for(const auto& key_value : map) {
        std::cout << "{" << key_value.first << ", " << key_value.second << "}" << std::endl;
    }
    
    
    // Use a map with a different neighborhood size
    const std::size_t neighborhood_size = 30;
    tsl::hopscotch_map<std::string, int, std::hash<std::string>, 
                       std::equal_to<std::string>,
                       std::allocator<std::pair<std::string, int>>,
                       neighborhood_size> map2 = {{"a", 1}, {"b", 2}};
    
    // {a, 1} {b, 2}
    for(const auto& key_value : map2) {
        std::cout << "{" << key_value.first << ", " << key_value.second << "}" << std::endl;
    }
    
    
    tsl::hopscotch_set<int> set;
    set.insert({1, 9, 0});
    set.insert({2, -1, 9});
    
    // {0} {1} {2} {9} {-1}
    for(const auto& key : set) {
        std::cout << "{" << key << "}" << std::endl;
    }
} 
```

#### Heterogeneous lookup

Heterogeneous overloads allow the usage of other types than `Key` for lookup and erase operations as long as the used types are hashable and comparable to `Key`.

To activate the heterogeneous overloads in `tsl::hopscotch_map/set`, the qualified-id `KeyEqual::is_transparent` must be valid. It works the same way as for [`std::map::find`](http://en.cppreference.com/w/cpp/container/map/find). You can either use [`std::equal_to<>`](http://en.cppreference.com/w/cpp/utility/functional/equal_to_void) or define your own function object.

Both `KeyEqual` and `Hash` will need to be able to deal with the different types.

```c++
#include <functional>
#include <iostream>
#include <string>
#include "hopscotch_map.h"


struct employee {
    employee(int id, std::string name) : m_id(id), m_name(std::move(name)) {
    }
    
    friend bool operator==(const employee& empl, int empl_id) {
        return empl.m_id == empl_id;
    }
    
    friend bool operator==(int empl_id, const employee& empl) {
        return empl_id == empl.m_id;
    }
    
    friend bool operator==(const employee& empl1, const employee& empl2) {
        return empl1.m_id == empl2.m_id;
    }
    
    
    int m_id;
    std::string m_name;
};

struct hash_employee {
    std::size_t operator()(const employee& empl) const {
        return std::hash<int>()(empl.m_id);
    }
    
    std::size_t operator()(int id) const {
        return std::hash<int>()(id);
    }
};

struct equal_employee {
    using is_transparent = void;
    
    bool operator()(const employee& empl, int empl_id) const {
        return empl.m_id == empl_id;
    }
    
    bool operator()(int empl_id, const employee& empl) const {
        return empl_id == empl.m_id;
    }
    
    bool operator()(const employee& empl1, const employee& empl2) const {
        return empl1.m_id == empl2.m_id;
    }
};


int main() {
    // Use std::equal_to<> which will automatically deduce and forward the parameters
    tsl::hopscotch_map<employee, int, hash_employee, std::equal_to<>> map; 
    map.insert({employee(1, "John Doe"), 2001});
    map.insert({employee(2, "Jane Doe"), 2002});
    map.insert({employee(3, "John Smith"), 2003});

    // John Smith 2003
    auto it = map.find(3);
    if(it != map.end()) {
        std::cout << it->first.m_name << " " << it->second << std::endl;
    }

    map.erase(1);



    // Use a custom KeyEqual which has an is_transparent member type
    tsl::hopscotch_map<employee, int, hash_employee, equal_employee> map2;
    map2.insert({employee(4, "Johnny Doe"), 2004});

    // 2004
    std::cout << map2.at(4) << std::endl;
} 
```

### License

The code is licensed under the MIT license, see the [LICENSE file](LICENSE) for details.
