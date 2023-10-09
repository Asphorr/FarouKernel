#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>

using namespace std;

struct MemoryBlock {
    uintptr_t startAddress;
    uintptr_t endAddress;
};

auto allocate(size_t size) -> uintptr_t {
    auto ptr = std::make_unique<MemoryBlock>();
    ptr->startAddress = reinterpret_cast<uintptr_t>(malloc(size));
    ptr->endAddress = ptr->startAddress + size;
    blockList.push_back(move(ptr));
    return ptr->startAddress;
}

void deallocate(uintptr_t addr) {
    auto iter = find_if(begin(blockList), end(blockList), [&](const auto &p){return p->startAddress == addr;});
    if (iter != end(blockList)) {
        free((*iter)->startAddress);
        (*iter).reset();
        blockList.erase(iter);
    }
}

void printFreeMemory() {
    cout << "Total Free Memory: ";
    auto totalFreeMem = accumulate(begin(blockList), end(blockList), 0, [](auto acc, const auto &p){return acc + (p->endAddress - p->startAddress);});
    cout << totalFreeMem << endl;
}

int main() {
    // Allocate some memory blocks
    auto memPtr1 = allocate(4 * sizeof(int));
    auto memPtr2 = allocate(8 * sizeof(double));
    auto memPtr3 = allocate(16 * sizeof(char));

    // Deallocate some memory blocks
    deallocate(memPtr1);
    deallocate(memPtr2);

    // Print free memory
    printFreeMemory();

    return 0;
}
