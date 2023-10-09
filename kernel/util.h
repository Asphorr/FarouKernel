#include <iostream>
#include <string>
#include <algorithm>
#include <numeric>
#include <ranges>

template <typename T>
struct VectorPrinter {
    void print() const {
        std::cout << "[";
        for (const auto& item : items_) {
            std::cout << ' ';
            std::visit([](const auto& x) { std::cout << x; }, item);
        }
        std::cout << " ]\n";
    }
private:
    std::vector<T> items_;
};

template <>
struct VectorPrinter<char> {
    void print() const {
        std::cout << "[";
        for (const auto& c : chars_) {
            std::cout << ' ';
            std::cout << c;
        }
        std::cout << " ]\n";
    }
private:
    std::vector<char> chars_;
};

template <typename T>
void printVector(const std::vector<T>& v) {
    VectorPrinter<T>{v}.print();
}

template <typename T>
std::vector<T> getUniqueElements(const std::vector<T>& v) {
    return std::set<T>(v.begin(), v.end()).to_vector();
}

template <typename T>
bool hasDuplicates(const std::vector<T>& v) {
    return std::adjacent_find(v.begin(), v.end()) != v.end();
}

int main() {
    // Example usage
    std::vector<int> myInts{1, 2, 3, 4, 5};
    printVector(myInts);
    
    std::vector<double> myDoubles{1.1, 2.2, 3.3, 4.4, 5.5};
    printVector(myDoubles);
    
    std::vector<char> myChars{'a', 'b', 'c', 'd', 'e'};
    printVector(myChars);
    
    std::vector<std::string> myStrings{"hello", "world", "this", "is", "a", "test"};
    printVector(myStrings);
    
    std::cout << "\nUnique elements:\n";
    std::vector<int> uniqueInts = getUniqueElements(myInts);
    printVector(uniqueInts);
    
    std::vector<double> uniqueDoubles = getUniqueElements(myDoubles);
    printVector(uniqueDoubles);
    
    std::vector<char> uniqueChars = getUniqueElements(myChars);
    printVector(uniqueChars);
    
    std::vector<std::string> uniqueStrings = getUniqueElements(myStrings);
    printVector(uniqueStrings);
    
    std::cout << "\nHas duplicates?\n";
    bool intsHaveDups = hasDuplicates(myInts);
    std::cout << "intsHaveDups: " << intsHaveDups << '\n';
    
    bool doublesHaveDups = hasDuplicates(myDoubles);
    std::cout << "doublesHaveDups: " << doublesHaveDups << '\n';
    
    bool charsHaveDups = hasDuplicates(myChars);
    std::cout << "charsHaveDups: " << charsHaveDups << '\n';
    
    bool stringsHaveDups = hasDuplicates(myStrings);
    std::cout << "stringsHaveDups: " << stringsHaveDups << '\n';
    
    return 0;
}
