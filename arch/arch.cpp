#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <functional>

// Define a type alias for the accumulator function
using AccumulatorFunc = std::plus<>();

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

// Implement the MaxElement concept
template <typename T>
concept bool MaxElement = requires(T x, T y) {
    { x > y } -> bool;
};

// Implement the max_element method for the MaxElement concept
template <MaxElement T>
constexpr auto max_element(T begin, T end) {
    return std::max_element(begin, end);
}

// Implement the MinElement concept
template <typename T>
concept bool MinElement = requires(T x, T y) {
    { x < y } -> bool;
};

// Implement the min_element method for the MinElement concept
template <MinElement T>
constexpr auto min_element(T begin, T end) {
    return std::min_element(begin, end);
}

// Implement the MeanValue concept
template <typename T>
concept bool MeanValue = requires(T x, T y) {
    { x - y } -> T;
};

// Implement the mean_value method for the MeanValue concept
template <MeanValue T>
constexpr auto mean_value(T begin, T end) {
    return std::mean_value(begin, end);
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
    constexpr size_t k = n / 2;
    long long sum = apply(numbers.begin(), numbers.end(), k);
    
    // Find maximum element
    auto max_it = max_element(numbers.begin(), numbers.end());
    
    // Find minimum element
    auto min_it = min_element(numbers.begin(), numbers.end());
    
    // Calculate mean value
    double mean = mean_value(numbers.begin(), numbers.end());
    
    // Print results
    std::cout << "sum: " << sum << ", max: " << *max_it << ", min: " << *min_it << ", mean: " << mean << "\n";
    
    return 0;
}
