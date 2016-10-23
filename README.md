## A C++ implementation of a hash map using hopscotch hashing
The hopscotch-map library is a C++ implementation of a single-thread hash map and hash set using hopscotch hashing. It offers good performances if all the elements of the key used by the equal function are contiguous in memory (no pointers to other parts of the memory that may cause a cache-miss) thanks to its good cache locality. The number of memory allocations are also small, they only happen on rehash or if there is an overflow (see [implementation details](https://tessil.github.io/2016/08/29/hopscotch-hashing.html)), allowing hopscotch-map to do fast inserts. It may be a good alternative to `std::unordered_map` in some cases and is mainly a concurrent to `google::dense_hash_map`, it trades off some memory space (if the key is big, otherwise it has the advantage compare to `std::unordered_map`) to have a fast hash table.

The library provides two classes: `tsl::hopscotch_map` and `tsl::hopscotch_set`.

An overview of hopscotch hashing and some implementation details may be found [here](https://tessil.github.io/2016/08/29/hopscotch-hashing.html).

A benchmark of `tsl::hopscotch_map` against other hash maps may be found [there](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html).

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
- No support for some emplace methods (and some others like bucket_size, bucket, ...).

These differences also apply between `std::unordered_set` and `tsl::hopscotch_set`.

### Differences compare to `google::dense_hash_map`
`tsl::hopscotch_map` has comparable performances to `google::dense_hash_map` (see [benchmark](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html)), but come with some advantages:
- There is no need to reserve sentinel values for the key as it is done by google::dense_hash_map where you need to have a sentinel for empty and deleted keys.
- The type of the value in the map doesn't need a default constructor.
- It uses less memory for its speed as it can sustain a load factor of 0.9 (which is the default value in the library compare to the 0.5 of `google::dense_hash_map`) while keeping good performances.

### Installation
To use hopscotch-map, just include the header [src/hopscotch_map.h](src/hopscotch_map.h) to your project. It's a header-only library.

The code should work with any C++11 standard-compliant compiler and has been tested with GCC 6.1, Clang 3.6 and Visual Studio 2015.

To run the tests you will need the Boost library and CMake. 

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
#include <iostream>
#include "hopscotch_map.h"

int main() {
    tsl::hopscotch_map<std::string, std::int64_t> map = {{"a", 1}, {"b", 2}};
    map["c"] = 3;
    map["d"] = 4;
    
    map.insert({"e", 5});
    map.erase("b");
    
    for(auto it = map.begin(); it != map.end(); ++it) {
        //it->second += 2; // Not valid.
        it.value() += 2;
    }
    
    for(const auto& key_value : map) {
        std::cout << "map: " << key_value.first << " " << key_value.second << std::endl;
    }
    
    
    // Use a map with a different neighborhood size
    const size_t neighborhood_size = 30;
    tsl::hopscotch_map<std::string, std::int64_t, std::hash<std::string>, 
                       std::equal_to<std::string>,
                       std::allocator<std::pair<std::string, std::int64_t>>,
                       neighborhood_size> map2 = {{"a", 1}, {"b", 2}};
    
    for(const auto& key_value : map2) {
        std::cout << "map2: " << key_value.first << " " << key_value.second << std::endl;
    }
    
    
    tsl::hopscotch_set<std::int64_t> set;
    set.insert({1, 9, 0});
    set.insert({2, -1, 9});
    
    for(const auto& key : set) {
        std::cout << "set: " << key << std::endl;
    }
}
```

### License

The code is licensed under the MIT license, see the [LICENSE file](LICENSE) for details.
