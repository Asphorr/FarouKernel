#include <iostream>
#include <vector>
#include <numeric>

// Define a template struct for vectors
template <typename T>
struct Vector {
    std::vector<T> data_;

    // Constructor
    explicit Vector(std::initializer_list<T> init) : data_(init) {}

    // Destructor
    ~Vector() {}

    // Copy constructor
    Vector(const Vector& other) : data_(other.data_) {}

    // Move constructor
    Vector(Vector&& other) : data_(std::move(other.data_)) {}

    // Assignment operator
    Vector& operator=(const Vector& other) {
        data_ = other.data_;
        return *this;
    }

    // Move assignment operator
    Vector& operator=(Vector&& other) {
        data_ = std::move(other.data_);
        return *this;
    }

    // Element access operators
    const T& operator[](size_t index) const {
        return data_.at(index);
    }

    T& operator[](size_t index) {
        return data_.at(index);
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

    friend std::ostream& operator<<(std::ostream& out, const Vector& v) {
        out << "[";
        for (auto it = v.begin(); it != v.end(); ++it) {
            out << *it << ", ";
        }
        out << "]";
        return out;
    }
};

int main() {
    // Create a vector of integers
    Vector<int> vec({1, 2, 3, 4, 5});

    // Print the vector contents
    std::cout << vec << std::endl;

    // Check if the size of the vector is less than or equal to MAX_SIZE
    bool result = vec.size() <= MAX_SIZE;
    std::cout << "Size check result: " << result << std::endl;

    // Sum up the elements of the vector
    auto total = std::accumulate(vec.begin(), vec.end(), 0);
    std::cout << "Total: " << total << std::endl;

    return 0;
}
