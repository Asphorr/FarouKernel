#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <type_traits>

// A helper function to print vectors in a readable format
template <typename T>
void printVector(const std::vector<T>& vec) {
    std::cout << "{ ";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        std::cout << *it << ", ";
    }
    std::cout << "}\n";
}

// A template class to store a vector of any type
template <typename T>
class Vector {
public:
    // Constructor takes an initializer list as input
    explicit Vector(std::initializer_list<T> initList) : data_(initList) {}
    
    // Returns true if there are no duplicate elements in the vector
    bool hasNoDuplicates() const {
        return std::all_of(data_.begin(), data_.end(), [](const T& elem){
            return std::count(data_.begin(), data_.end(), elem) == 1;
        });
    }
    
    // Returns a new vector containing only the unique elements from the original vector
    std::vector<T> getUniqueElements() const {
        std::vector<T> result;
        for (const auto& elem : data_) {
            if (!result.contains(elem)) {
                result.push_back(elem);
            }
        }
        return result;
    }
    
    // Prints the contents of the vector using the provided printer function
    template <typename U>
    friend void printVector(const Vector<U>& vec);
    
private:
    std::vector<T> data_;
};

// Specialization for printing vectors of integers
template <>
void printVector(const Vector<int>& vec) {
    std::cout << "{ ";
    for (auto it = vec.data_.begin(); it != vec.data_.end(); ++it) {
        std::cout << *it << ", ";
    }
    std::cout << "}\n";
}

// Specialization for printing vectors of floating-point numbers
template <>
void printVector(const Vector<float>& vec) {
    std::cout << "{ ";
    for (auto it = vec.data_.begin(); it != vec.data_.end(); ++it) {
        std::cout << *it << ", ";
    }
    std::cout << "}\n";
}

// Specialization for printing vectors of characters
template <>
void printVector(const Vector<char>& vec) {
    std::cout << "{ ";
    for (auto it = vec.data_.begin(); it != vec.data_.end(); ++it) {
        std::cout << *it << ", ";
    }
    std::cout << "}\n";
}

// Specialization for printing vectors of strings
template <>
void printVector(const Vector<std::string>& vec) {
    std::cout << "{ ";
    for (auto it = vec.data_.begin(); it != vec.data_.end(); ++it) {
        std::cout << '"' << *it << "\", ";
    }
    std::cout << "}\n";
}

int main() {
    // Create some test vectors
    Vector<int> myInts({1, 2, 3, 4, 5});
    Vector<float> myFloats({1.1f, 2.2f, 3.3f, 4.4f, 5.5f});
    Vector<char> myChars({'a', 'b', 'c', 'd', 'e'});
    Vector<std::string> myStrings({"Hello", "World", "This", "Is", "A", "Test"});
    
    // Print the vectors
    printVector(myInts);
    printVector(myFloats);
    printVector(myChars);
    printVector(myStrings);
    
    // Check if they have duplicates
    std::cout << "MyInts has duplicates: " << myInts.hasNoDuplicates() << "\n";
    std::cout << "MyFloats has duplicates: " << myFloats.hasNoDuplicates() << "\n";
std::cout << "MyChars has duplicates: " << myChars.hasNoDuplicates() << "\n";
std::cout << "MyStrings has duplicates: " << myStrings.hasNoDuplicates() << "\n\n";

// Get the unique elements and print them
std::vector<int> intsWithoutDups = myInts.getUniqueElements();
std::vector<float> floatsWithoutDups = myFloats.getUniqueElements();
std::vector<char> charsWithoutDups = myChars.getUniqueElements();
std::vector<std::string> stringsWithoutDups = myStrings.getUniqueElements();

printVector(intsWithoutDups);
printVector(floatsWithoutDups);
printVector(charsWithoutDups);
printVector(stringsWithoutDups);

return 0;
}
