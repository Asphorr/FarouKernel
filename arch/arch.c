#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>

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
    std::sort(numbers.begin(), numbers.end(), std::greater<>());
    
    // Calculate sum of first k elements
    auto k = n / 2;
    long long sum = std::accumulate(numbers.begin(), numbers.begin() + k, 0ll);
    
    // Print result
    std::cout << sum << "\n";
    
    return 0;
}
