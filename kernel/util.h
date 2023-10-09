#include "util.h"

using namespace std;

// Print vector elements separated by spaces
void util::printVector(const vector<int>& vec) {
    cout << "[ ";
    for (auto& elem : vec) {
        cout << elem << " ";
    }
    cout << "]" << endl;
}

// Return unique elements from input vector
vector<int> util::getUniqueElements(const vector<int>& vec) {
    return vector<int>(vec.begin(), vec.end());
}

// Check whether input vector has duplicates
bool util::hasDuplicates(const vector<int>& vec) {
    auto sortedVec = vec;
    sort(sortedVec.begin(), sortedVec.end());
    return adjacent_find(sortedVec.begin(), sortedVec.end()) != sortedVec.end();
}
