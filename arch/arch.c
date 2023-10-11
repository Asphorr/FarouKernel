#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>

// Define a type alias for the accumulator function
using AccumulatorFunc = std::plus<>;

// Implement the SumFirstKElements concept
template <typename T>
concept bool SumFirstKElements = requires(T x, T y) {
    { x + y } -> T;
};

// Implement the apply method for the SumFirstKElements concept
template <SumFirstKElements T>
constexpr decltype(auto) apply(T begin, T end, size_t k) {
    return std::accumulate(std::next(begin), std::min(std::next(begin, k), end), *begin);
}

int main() {
    // Read input from stdin
    int n;
    std::cin >> n;
    
    // Create vector of integers
    std::vector<int> numbers(n);
    for (auto& num : numbers) {
        std::cin >> num;
    }
    
    // Sort the vector in descending order
    std::sort(numbers.rbegin(), numbers.rend());
    
    // Calculate sum of first k elements
    auto k = n / 2;
    long long sum = apply(numbers.begin(), numbers.end(), k);
    
    // Print result
    std::cout << sum << "\n";
    
    return 0;
}
