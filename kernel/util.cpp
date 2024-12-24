#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

namespace util {

// Print a vector to stdout
template <typename T>
void printVector(const std::vector<T>& v) {
    std::cout << "[ ";
    std::for_each(v.begin(), v.end(), [](const auto& elem){ std::cout << elem << ' '; });
    std::cout << " ]\n";
}

// Get unique elements from a vector
template <typename T>
std::vector<T> getUniqueElements(const std::vector<T>& v) {
    if (v.empty()) {
        return {};
    }

    std::vector<T> result;
    result.reserve(v.size());

    std::set<T> seen;
    for (const auto& elem : v) {
        if (seen.insert(elem).second) {
            result.push_back(elem);
        }
    }

    return result;
}

// Check if a vector contains any duplicates
template <typename T>
bool hasDuplicates(const std::vector<T>& v) {
    if (v.empty()) {
        return false;
    }

    std::set<T> seen;
    for (const auto& elem : v) {
        if (!seen.insert(elem).second) {
            return true;
        }
    }

    return false;
}

} // namespace util
