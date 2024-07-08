#include <memory>
#include <vector>
#include <algorithm>
#include <span>
#include <numeric>
#include <cstdio>
#include <cassert>

template <typename T>
class MemoryManager {
private:
    struct Block {
        std::unique_ptr<T[]> data;
        std::size_t size;
        bool is_free;

        Block(std::size_t s) : data(std::make_unique<T[]>(s)), size(s), is_free(true) {}
    };
    
    std::vector<Block> blocks;
    std::size_t total_memory{0};

public:
    T* allocate(std::size_t size) {
        auto it = std::find_if(blocks.begin(), blocks.end(), 
            [size](const Block& b) { return b.is_free && b.size >= size; });

        if (it != blocks.end()) {
            it->is_free = false;
            if (it->size > size) {
                split_block(it, size);
            }
            return it->data.get();
        }

        blocks.emplace_back(size);
        total_memory += size;
        blocks.back().is_free = false;
        return blocks.back().data.get();
    }

    void deallocate(T* address) {
        auto it = find_block(address);
        if (it != blocks.end()) {
            it->is_free = true;
            merge_adjacent_free_blocks(it);
        }
    }

    void printMemoryState() const {
        std::size_t free_memory = std::accumulate(blocks.begin(), blocks.end(), 0ULL,
            [](std::size_t sum, const Block& block) { return sum + (block.is_free ? block.size : 0); });
        std::printf("Total Memory: %zu, Free Memory: %zu, Used Memory: %zu\n", 
                    total_memory, free_memory, total_memory - free_memory);
    }

    T* reallocate(T* address, std::size_t newSize) {
        auto it = find_block(address);
        if (it == blocks.end()) return nullptr;

        if (newSize <= it->size) {
            if (it->size > newSize) {
                split_block(it, newSize);
            }
            return it->data.get();
        }

        T* new_address = allocate(newSize);
        if (new_address) {
            std::copy_n(it->data.get(), it->size, new_address);
            deallocate(address);
        }
        return new_address;
    }

    bool copy(T* sourceAddress, T* destinationAddress, std::size_t size) {
        auto srcIt = find_block(sourceAddress);
        auto destIt = find_block(destinationAddress);
        if (srcIt == blocks.end() || destIt == blocks.end()) return false;

        std::size_t copySize = std::min({srcIt->size, destIt->size, size});
        std::copy_n(srcIt->data.get(), copySize, destIt->data.get());
        return true;
    }

    std::span<T> getMemoryBlock(T* address) const {
        auto it = find_block(address);
        return (it != blocks.end()) ? std::span<T>(it->data.get(), it->size) : std::span<T>();
    }

private:
    auto find_block(T* address) const {
        return std::find_if(blocks.begin(), blocks.end(), [address](const Block& block) {
            return block.data.get() <= address && address < block.data.get() + block.size;
        });
    }

    void split_block(typename std::vector<Block>::iterator it, std::size_t size) {
        assert(it->size > size);
        std::size_t remaining_size = it->size - size;
        it->size = size;
        blocks.emplace(std::next(it), remaining_size);
    }

    void merge_adjacent_free_blocks(typename std::vector<Block>::iterator it) {
        while (std::next(it) != blocks.end() && std::next(it)->is_free) {
            it->size += std::next(it)->size;
            blocks.erase(std::next(it));
        }
        if (it != blocks.begin()) {
            auto prev = std::prev(it);
            if (prev->is_free) {
                prev->size += it->size;
                blocks.erase(it);
            }
        }
    }
};
