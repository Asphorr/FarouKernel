#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <execution>
#include <type_traits>

// A function template that sorts and filters a vector of strings
template <typename T>
requires requires(T t) {
    // Make sure T has begin(), end(), size(), and operator[] methods
    { t.begin() };
    { t.end() };
    { t.size() };
    { t[0] };
} && std::is_convertible_v<decltype(*t.begin()), std::string>
void sortAndFilterVectorOfStrings(T& vec) {
    // Sort the vector in descending order by string length
    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b) {
        return a.length() > b.length();
    }, std::greater<>{});
    
    // Remove any empty strings from the vector
    vec.erase(std::remove_if(vec.begin(), vec.end(), [](const auto& s) {
        return s.empty();
    }));
}

int main() {
    // Create a vector of strings
    std::vector<std::string> vec = {"hello", "", "world", "abcdefghijklmnopqrstuvwxyz", ""};
    
    // Call the function template to sort and filter the vector
    sortAndFilterVectorOfStrings(vec);
    
    // Print out each element of the sorted and filtered vector
    for (const auto& elem : vec) {
        std::cout << elem << "\n";
    }
    
    return 0;
}
