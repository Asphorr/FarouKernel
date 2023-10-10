#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T>
struct MemoryBlock {
    T* startAddress;
    T* endAddress;
};

template <typename T>
using BlockList = std::vector<MemoryBlock<T>>;

template <typename T>
void allocate(BlockList<T>& list, std::size_t size) {
    auto ptr = new MemoryBlock<T>;
    ptr->startAddress = reinterpret_cast<T*>(malloc(size));
    ptr->endAddress = ptr->startAddress + size / sizeof(T);
    list.push_back(*ptr);
}

template <typename T>
void deallocate(BlockList<T>& list, T* address) {
    auto iter = std::find_if(list.begin(), list.end(), [address](const MemoryBlock<T>& block) {
        return block.startAddress <= address && address < block.endAddress;
    });
    if (iter != list.end()) {
        free(iter->startAddress);
        list.erase(iter);
    }
}

template <typename T>
void printFreeMemory(const BlockList<T>& list) {
    std::printf("Total Free Memory: %zu\n", std::accumulate(list.begin(), list.end(), 0, [](auto acc, const MemoryBlock<T>& block) {
        return acc + (block.endAddress - block.startAddress);
    }));
}

int main() {
    BlockList<int> intBlocks;
    BlockList<double> doubleBlocks;
    BlockList<char> charBlocks;

    // Allocate some memory blocks
    allocate(intBlocks, 4 * sizeof(int));
    allocate(doubleBlocks, 8 * sizeof(double));
    allocate(charBlocks, 16 * sizeof(char));

    // Deallocate some memory blocks
    deallocate(intBlocks, intBlocks[0].startAddress);
    deallocate(doubleBlocks, doubleBlocks[0].startAddress);

    // Print free memory
    printFreeMemory(intBlocks);
    printFreeMemory(doubleBlocks);
    printFreeMemory(charBlocks);

    return 0;
}
