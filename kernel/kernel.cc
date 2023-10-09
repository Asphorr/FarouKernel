#include <iostream>
#include <vector>
#include <string_view>
#include <concepts>
#include <ranges>
#include <numbers>
#include <execution>

using namespace std;

constexpr int MAX_SIZE = 1000;

template <typename T>
requires requires(T t) {
    t + t;
}
struct VectorSum {
    static void print(const vector<T>& vec) {
        cout << "Vector contents: ";
        ranges::for_each(vec, [](const auto& elem){cout << elem << ' ';});
        cout << '\n';
    }

    static bool check_size(const vector<T>& vec) {
        return vec.size() <= MAX_SIZE;
    }

    static T sum(const vector<T>& vec) {
        return ranges::reduce(vec, 0, plus<>());
    }
};

int main() {
    // Create a vector of integers
    vector<int> vec{1, 2, 3, 4, 5};

    // Print the vector contents
    VectorSum::print(vec);

    // Check if the size of the vector is less than or equal to MAX_SIZE
    bool result = VectorSum::check_size(vec);
    cout << "Size check result: " << result << "\n";

    // Sum up the elements of the vector
    auto total = VectorSum::sum(vec);
    cout << "Total: " << total << "\n";

    return 0;
}
