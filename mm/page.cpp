#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <ranges>

using namespace std;

// Function to read input from user
void getInput(vector<double>& numbers) {
    cout << "Enter some numbers (enter nothing to quit): ";
    string line;
    while (getline(cin, line) && !line.empty()) {
        try {
            const char* str = line.c_str();
            double num = stod(str);
            numbers.push_back(num);
        } catch (invalid_argument& e) {
            // Ignore invalid inputs
        }
    }
}

// Function to compute sum of squares
double sumSquares(const vector<double>& numbers) {
    return accumulate(numbers.begin(), numbers.end(), 0.0, [](double acc, double x){return acc + x * x;});
}

// Function to compute product of positive numbers
double prodPositive(const vector<double>& numbers) {
    auto posNumbers = filter(numbers.begin(), numbers.end(), [](double x) {return x > 0;});
    return reduce(posNumbers.begin(), posNumbers.end(), 1.0, multiplies<>());
}

// Function to compute average of numbers
double avg(const vector<double>& numbers) {
    return accumulate(numbers.begin(), numbers.end(), 0.0) / numbers.size();
}

// Function to find maximum element in a range
template <typename T>
T maxElement(const vector<T>& vec) {
    return *max_element(vec.begin(), vec.end());
}

// Function to find minimum element in a range
template <typename T>
T minElement(const vector<T>& vec) {
    return *min_element(vec.begin(), vec.end());
}

// Function to sort elements in ascending order
template <typename T>
void sortElements(vector<T>& vec) {
    sort(vec.begin(), vec.end());
}

// Function to print elements in a container
template <typename Container>
void printContainer(const Container& c) {
    for (auto elem : c) {
        cout << elem << ' ';
    }
    cout << endl;
}

int main() {
    vector<double> numbers;
    getInput(numbers);
    
    cout << "Sum of squares: " << sumSquares(numbers) << '\n';
    cout << "Product of positive numbers: " << prodPositive(numbers) << '\n';
    cout << "Average of numbers: " << avg(numbers) << '\n';
    cout << "Maximum element: " << maxElement(numbers) << '\n';
    cout << "Minimum element: " << minElement(numbers) << '\n';
    sortElements(numbers);
    cout << "Sorted elements: \n";
    printContainer(numbers);
    
    return 0;
}
