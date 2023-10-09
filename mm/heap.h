#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>

// Define a custom comparator function for sorting elements
auto compare = [] (const auto& lhs, const auto& rhs) { return lhs > rhs; };

class Heap {
public:
    // Constructor
    Heap(std::initializer_list<int> list) : _data{list} {}
    
    // Destructor
    ~Heap() {}
    
    // Insert element into the heap
    void insert(int value) {
        _data.push_back(value);
        std::make_heap(_data.begin(), _data.end());
    }
    
    // Remove top element from the heap
    int removeTop() {
        std::pop_heap(_data.begin(), _data.end());
        return _data.front();
    }
    
private:
    std::vector<int> _data;
};

int main() {
    Heap h({1, 2, 3, 4, 5});
    h.insert(6);
    h.removeTop();
    std::cout << "The top element is: " << h._data[0] << "\n";
    return 0;
}
