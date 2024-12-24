#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>

// Define a template struct for vectors
template <typename T>
struct Vector {
    std::vector<T> data_;

    // Constructor
    explicit Vector(std::initializer_list<T> init) : data_(init) {}

    // Iterator-based constructor
    template <typename Iterator>
    Vector(Iterator begin, Iterator end) : data_(begin, end) {}

    // Destructor
    ~Vector() {}

    // Copy constructor
    Vector(const Vector& other) : data_(other.data_) {}

    // Move constructor
    Vector(Vector&& other) : data_(std::move(other.data_)) {}

    // Assignment operator
    Vector& operator=(const Vector& other) {
        if (this != &other) {
            data_ = other.data_;
        }
        return *this;
    }

    // Move assignment operator
    Vector& operator=(Vector&& other) {
        if (this != &other) {
            data_ = std::move(other.data_);
        }
        return *this;
    }

    // Element access operators
    const T& operator[](size_t index) const {
        if (index >= data_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    T& operator[](size_t index) {
        if (index >= data_.size()) {
            throw std::out_of_range("Index out of range");
        }
        return data_[index];
    }

    // Size getter
    size_t size() const {
        return data_.size();
    }

    // Iterators
    typename std::vector<T>::iterator begin() {
        return data_.begin();
    }

    typename std::vector<T>::iterator end() {
        return data_.end();
    }

    typename std::vector<T>::const_iterator cbegin() const {
        return data_.cbegin();
    }

    typename std::vector<T>::const_iterator cend() const {
        return data_.cend();
    }

    // Other methods
    void push_back(const T& value) {
        data_.push_back(value);
    }

    void pop_back() {
        data_.pop_back();
    }

    void clear() {
        data_.clear();
    }

    void sort(bool ascending = true) {
        if (ascending) {
            std::sort(data_.begin(), data_.end());
        } else {
            std::sort(data_.begin(), data_.end(), std::greater<T>());
        }
    }

    void reverse() {
        std::reverse(data_.begin(), data_.end());
    }

    void insert(size_t position, const T& value) {
        if (position > data_.size()) {
            throw std::out_of_range("Invalid position");
        }
        data_.insert(data_.begin() + position, value);
    }

    void erase(size_t position) {
        if (position >= data_.size()) {
            throw std::out_of_range("Invalid position");
        }
        data_.erase(data_.begin() + position);
    }

    typename std::vector<T>::iterator find(const T& value) {
        return std::find(data_.begin(), data_.end(), value);
    }

    size_t count(const T& value) {
        return std::count(data_.begin(), data_.end(), value);
    }

    Vector<T> subvector(size_t start, size_t end) {
        if (start > end || end > data_.size()) {
            throw std::out_of_range("Invalid range");
        }
        return Vector<T>(data_.begin() + start, data_.begin() + end);
    }

    friend std::ostream& operator<<(std::ostream& out, const Vector& v) {
        out << "[";
        bool first = true;
        for (const auto& element : v) {
            if (!first) {
                out << ", ";
            }
            out << element;
            first = false;
        }
        out << "]";
        return out;
    }
};

int main() {
    constexpr int MAX_SIZE = 100;

    // Create a vector of integers
    Vector<int> vec({1, 2, 3, 4, 5});
}
      // Print the vector contents
    std::cout << "Vector: " << vec << std::endl;

    // Check if the size of the vector is less than or equal to MAX_SIZE
    bool result = vec.size() <= MAX_SIZE;
    std::cout << "Size check result: " << result << std::endl;

    // Sum up the elements of the vector
    auto total = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "Total: " << total << std::endl;

    // Sort the vector in ascending order
    vec.sort();
    std::cout << "Sorted vector (ascending): " << vec << std::endl;

    // Reverse the vector
    vec.reverse();
    std::cout << "Reversed vector: " << vec << std::endl;

    // Insert an element at position 2
    vec.insert(2, 10);
    std::cout << "Vector after insertion: " << vec << std::endl;

    // Erase the element at position 3
    vec.erase(3);
    std::cout << "Vector after erasure: " << vec << std::endl;

    // Find the first occurrence of 4
    auto it = vec.find(4);
    if (it != vec.end()) {
        std::cout << "Found element 4 at position: " << std::distance(vec.begin(), it) << std::endl;
    } else {
        std::cout << "Element 4 not found" << std::endl;
    }

    // Count the number of occurrences of 2
    size_t count = vec.count(2);
    std::cout << "Number of occurrences of 2: " << count << std::endl;

    // Extract a subvector from position 1 to 3
    Vector<int> subvec = vec.subvector(1, 4);
    std::cout << "Subvector: " << subvec << std::endl;

    return 0;
}
