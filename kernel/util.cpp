#include "util.h"

using namespace std;

int main() {
    // Create sample data
    constexpr size_t n = 10;
    vector<int> vec(n);
    iota(vec.begin(), vec.end(), 0);

    // Call utility functions
    util::printVector(vec);
    auto uniqueVec = util::getUniqueElements(vec);
    bool hasDups = util::hasDuplicates(uniqueVec);

    // Output results
    cout << "Unique elements: ";
    util::printVector(uniqueVec);
    cout << "Has duplicates: " << hasDups << endl;

    return 0;
}
