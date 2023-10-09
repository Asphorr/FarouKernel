#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

// Function to read input from user
void getInput(std::vector<double>& numbers) {
    double num;
    while (true) {
        std::cin >> num;
        if (!num) break;
        numbers.push_back(num);
    }
}

// Function to compute sum of squares
double sumSquares(const std::vector<double>& numbers) {
    return std::accumulate(numbers.begin(), numbers.end(), 0.0, [](double acc, double x){return acc + x * x;});
}

// Function to compute product of positive numbers
double prodPositive(const std::vector<double>& numbers) {
    auto posNumbers = std::copy_if(numbers.begin(), numbers.end(), [] (double x) {return x > 0;});
    return std::reduce(posNumbers.begin(), posNumbers.end(), 1.0, std::multiplies<>());
}

// Main program
int main() {
    std::vector<double> numbers;
    getInput(numbers);
    
    std::cout << "Sum of squares: " << sumSquares(numbers) << '\n';
    std::cout << "Product of positive numbers: " << prodPositive(numbers) << '\n';
    
    return 0;
}
