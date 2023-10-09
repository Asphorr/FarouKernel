#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <algorithm>
#include <numeric>

class MemoryManager {
public:
    static void allocate(std::size_t size) {
        auto ptr = std::make_unique<MemoryBlock>();
        ptr->startAddress = reinterpret_cast<uintptr_t>(malloc(size));
        ptr->endAddress = ptr->startAddress + size;
        blockList.emplace_back(std::move(ptr));
    }

    static void deallocate(uintptr_t addr) {
        auto iter = std::find_if(blockList.begin(), blockList.end(), [&](const auto& p) { return p->startAddress == addr; });
        if (iter != blockList.end()) {
            free((*iter)->startAddress);
            (*iter).reset();
            blockList.erase(iter);
        }
    }

    static void printFreeMemory() {
        std::cout << "Total Free Memory: ";
        auto totalFreeMem = std::accumulate(blockList.begin(), blockList.end(), 0, [](auto acc, const auto& p) { return acc + (p->endAddress - p->startAddress); });
        std::cout << totalFreeMem << std::endl;
    }
private:
    struct MemoryBlock {
        uintptr_t startAddress;
        uintptr_t endAddress;
    };

    static std::vector<std::unique_ptr<MemoryBlock>> blockList;
};

int main() {
    // Allocate some memory blocks
    auto memPtr1 = MemoryManager::allocate(4 * sizeof(int));
    auto memPtr2 = MemoryManager::allocate(8 * sizeof(double));
    auto memPtr3 = MemoryManager::allocate(16 * sizeof(char));

    // Deallocate some memory blocks
    MemoryManager::deallocate(memPtr1);
    MemoryManager::deallocate(memPtr2);

    // Print free memory
    MemoryManager::printFreeMemory();

    return 0;
}
