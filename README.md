# A C++ implementation of a hash map using hopscotch hashing
The hopscotch_map is a C++ implementation of a single-thread hash map using hopscotch hashing. It offers good performances if all the elements of the key used by the equal function are contiguous in memory (no pointers to other parts of the memory for example) thanks to its good cache locality. The number of memory allocations are also small, they only happen on rehash or if there is an overflow (see [implementation details](https://tessil.github.io/2016/08/29/hopscotch-hashing.html)), allowing hopscotch_map to do fast inserts. It may be a good alternative to std::unordered_map in some cases and is mainly a concurrent to Google dense_hash_map, it trades off some memory space (if the key is more than 8 bytes, else it has the advantage compare to std::unordered_map) for speed.

An overview of hopscotch hashing and some implementation details may be found [here](https://tessil.github.io/2016/08/29/hopscotch-hashing.html).

A benchmark of hopscotch_map against other hash maps may be found [there](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html).

Note: The implementation of hopscotch_map is single-thread only. A multi-thread implementation may arrive later.

## Differences compare to std::unordered_map
hopscotch_map tries to have an interface similar to std::unordered_map, but some differences exist:
- Iterator invalidation on insert doesn't behave in the same way (see [API](https://tessil.github.io/hopscotch-map/doc/html/classhopscotch__map.html#details) for details).
- References and pointers to keys or values in the map are invalidated in the same way as iterators to these keys-values.
- The size of the bucket array in the map grows by a factor of 2, the size will always be a power of 2, which may be a too steep growth rate for some purposes.
- For iterators, `operator*()` and `operator->()` return a reference and a pointer to `const std::pair<Key, T>` instead of `std::pair<const Key, T>` making the value `T` not modifiable. To modify the value you have to call the `value()` method of the iterator to get a mutable reference. Example:
```c++
hopscotch_map<int, int> map = {{1, 1}, {2, 1}, {3, 1}};
for(auto it = map.begin(); it != map.end(); ++it) {
    //it->second = 2; // Illegal
    it.value() = 2; // Ok
}
```
- No support for custom allocators yet.
- No support for some emplace methods (and some other like reserve, ...).

## Differences compare to google::dense_hash_map
hopscotch_map has comparable performances to google::dense_hash_map (see [benchmark](https://tessil.github.io/2016/08/29/benchmark-hopscotch-map.html)), but come with some advantages:
- There is no need to reserve sentinel values for the key as it is done by google::dense_hash_map where you need to have a sentinel for empty and deleted keys.
- The type of the value in the map doesn't need a default constructor.
- It uses less memory.

## Installation
To use hopscotch_map, just include the header src/hopscotch_map.h to your project. It's a header-only library.

The code should work with any C++11 standard compliant compiler and has been tested with GCC 6.1, Clang 3.6 and Visual Studio 2015.

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


## Usage
The API can be found [here](https://tessil.github.io/hopscotch-map/doc/html/). 

All methods are not documented yet, but they replicate the behaviour of the ones in std::unordered_map, except if specified otherwise.

## Example
```c++
#include <iostream>
#include "hopscotch_map.h"

int main() {
    hopscotch_map<std::string, int64_t> map = {{"a", 1}, {"b", 2}};
    
    map["c"] = 3;
    map["d"] = 4;
    
    map.insert({"e", 5});
    map.erase("b");
    
    for(const auto & key_value : map) {
        std::cout << "map: " << key_value.first << " " << key_value.second << std::endl;
    }
    
    // Use a map with a different neighborhood size
    const size_t neighborhood_size = 30;
    hopscotch_map<std::string, int64_t, std::hash<std::string>, 
                  std::equal_to<std::string>, neighborhood_size> map2 = {{"a", 1}, {"b", 2}};
    
    for(const auto & key_value : map2) {
        std::cout << "map2: " << key_value.first << " " << key_value.second << std::endl;
    }
}
```

## License

The code is licensed under the MIT license, see the [LICENSE file](LICENSE) for details.
