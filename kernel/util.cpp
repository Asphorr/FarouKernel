#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <iterator>

template <typename T>
struct UtilityFunctions {
    static void printVector(const std::vector<T>& v) {
        std::cout << '[';
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, ", "));
        std::cout << ']';
    }
    
    template <typename U>
    static std::vector<U> getUniqueElements(const std::vector<U>& v) {
        std::sort(v.begin(), v.end());
        std::vector<U> result;
        std::set_difference(v.begin(), v.end(), result.begin(), result.end());
        return result;
    }
    
    template <typename V>
    static bool hasDuplicates(const std::vector<V>& v) {
        return !std::is_permutation(v.begin(), v.end(), v.begin());
    }
};

int main() {
    constexpr size_t n = 10;
    std::vector<int> vec(n);
    std::iota(vec.begin(), vec.end(), 0);
    
    UtilityFunctions<int>::printVector(vec);
    auto uniqueVec = UtilityFunctions<int>::getUniqueElements(vec);
    bool hasDups = UtilityFunctions<int>::hasDuplicates(uniqueVec);
    
    std::cout << "Unique elements: ";
    UtilityFunctions<int>::printVector(uniqueVec);
    std::cout << "Has duplicates: " << hasDups << '\n';
    
    return 0;
}
