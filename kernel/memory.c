#include <memory>
#include <algorithm>
#include <span>

template <typename T>
struct MemoryBlock {
 std::unique_ptr<T[]> data;
 std::size_t size;
};

template <typename T>
using BlockList = std::vector<MemoryBlock<T>>;

template <typename T>
void allocate(BlockList<T>& list, std::size_t size) {
 list.emplace_back();
 auto& block = list.back();
 block.data = std::make_unique<T[]>(size);
 block.size = size;
}

template <typename T>
void deallocate(BlockList<T>& list, T* address) {
 auto iter = std::find_if(list.begin(), list.end(), [address](const MemoryBlock<T>& block) {
     return block.data.get() <= address && address < block.data.get() + block.size;
 });
 if (iter != list.end()) {
     list.erase(iter);
 }
}

template <typename T>
void printFreeMemory(const BlockList<T>& list) {
 std::printf("Total Free Memory: %zu\n", std::accumulate(list.begin(), list.end(), 0, [](auto acc, const MemoryBlock<T>& block) {
     return acc + block.size;
 }));
}

template <typename T>
void reallocate(BlockList<T>& list, T* address, std::size_t newSize) {
 auto iter = std::find_if(list.begin(), list.end(), [address](const MemoryBlock<T>& block) {
     return block.data.get() <= address && address < block.data.get() + block.size;
 });
 if (iter != list.end()) {
     iter->data = std::make_unique<T[]>(newSize);
     iter->size = newSize;
 }
}

template <typename T>
void resize(BlockList<T>& list, T* address, std::size_t newSize) {
 auto iter = std::find_if(list.begin(), list.end(), [address](const MemoryBlock<T>& block) {
     return block.data.get() <= address && address < block.data.get() + block.size;
 });
 if (iter != list.end()) {
     auto newData = std::make_unique<T[]>(newSize);
     std::copy(iter->data.get(), iter->data.get() + std::min(iter->size, newSize), newData.get());
     iter->data = std::move(newData);
     iter->size = newSize;
 }
}

template <typename T>
void copy(BlockList<T>& list, T* sourceAddress, T* destinationAddress, std::size_t size) {
 auto sourceIter = std::find_if(list.begin(), list.end(), [sourceAddress](const MemoryBlock<T>& block) {
     return block.data.get() <= sourceAddress && sourceAddress < block.data.get() + block.size;
 });
 auto destinationIter = std::find_if(list.begin(), list.end(), [destinationAddress](const MemoryBlock<T>& block) {
     return block.data.get() <= destinationAddress && destinationAddress < block.data.get() + block.size;
 });
 if (sourceIter != list.end() && destinationIter != list.end()) {
     std::copy(sourceIter->data.get(), sourceIter->data.get() + std::min(sourceIter->size, size), destinationIter->data.get());
 }
}

template <typename T>
constexpr std::span<T> getMemoryBlock(const BlockList<T>& list, T* address) {
 auto iter = std::find_if(list.begin(), list.end(), [address](const MemoryBlock<T>& block) {
     return block.data.get() <= address && address < block.data.get() + block.size;
 });
 if (iter != list.end()) {
     return std::span<T>(iter->data.get(), iter->size);
 } else {
     return std::span<T>();
 }
}
