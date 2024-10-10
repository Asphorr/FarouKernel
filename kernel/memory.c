#include <memory>
#include <vector>
#include <algorithm>
#include <span>
#include <numeric>
#include <cstdio>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <iostream>

template <typename T>
class MemoryManager {
private:
    struct Block {
        std::size_t offset; // Offset in the memory pool
        std::size_t size;
        bool is_free;

        Block(std::size_t off, std::size_t sz, bool free = true)
            : offset(off), size(sz), is_free(free) {}
    };
    
    std::unique_ptr<T[]> memory_pool;
    std::size_t pool_size;
    std::vector<Block> blocks;
    
public:
    // Constructor: Initialize memory manager with a total memory size
    explicit MemoryManager(std::size_t totalSize)
        : memory_pool(std::make_unique<T[]>(totalSize)), pool_size(totalSize) {
        blocks.emplace_back(0, totalSize, true); // Start with one large free block
    }

    // Disable copy semantics
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // Enable move semantics
    MemoryManager(MemoryManager&&) = default;
    MemoryManager& operator=(MemoryManager&&) = default;

    // Allocate memory of given size. Returns pointer to allocated memory or nullptr if failed.
    T* allocate(std::size_t size) {
        if (size == 0 || size > pool_size) {
            return nullptr; // Invalid size
        }

        // Find first-fit free block
        auto it = std::find_if(blocks.begin(), blocks.end(), 
                [size](const Block& b) { return b.is_free && b.size >= size; });

        if (it == blocks.end()) {
            // No suitable block found
            return nullptr;
        }

        std::size_t allocated_offset = it->offset;
        T* ptr = memory_pool.get() + allocated_offset;

        if (it->size > size) {
            // Split the block into allocated and remaining free parts
            std::size_t remaining = it->size - size;
            it->offset += size;
            it->size = remaining;
            blocks.emplace(it, allocated_offset, size, false); // Insert allocated block before the free block
        } else {
            // Exact fit; mark block as used
            it->is_free = false;
        }

        return ptr;
    }

    // Deallocate memory at given address. Returns true if successful, false otherwise.
    bool deallocate(T* address) {
        if (address == nullptr) {
            return false; // Null pointer cannot be deallocated
        }

        if (address < memory_pool.get() || address >= memory_pool.get() + pool_size) {
            // Address out of range
            return false;
        }

        std::size_t offset = static_cast<std::size_t>(address - memory_pool.get());
        auto it = std::find_if(blocks.begin(), blocks.end(), 
                [offset](const Block& b) { return b.offset == offset; });

        if (it == blocks.end() || it->is_free) {
            // Block not found or already free
            return false;
        }

        it->is_free = true;
        merge_adjacent_free_blocks(it);
        return true;
    }

    // Reallocate memory to a new size. Returns new address or nullptr on failure.
    T* reallocate(T* address, std::size_t newSize) {
        if (newSize == 0) {
            deallocate(address);
            return nullptr;
        }

        if (address == nullptr) {
            return allocate(newSize);
        }

        if (address < memory_pool.get() || address >= memory_pool.get() + pool_size) {
            // Address out of range
            return nullptr;
        }

        std::size_t current_offset = static_cast<std::size_t>(address - memory_pool.get());
        auto it = std::find_if(blocks.begin(), blocks.end(), 
                [current_offset](const Block& b) { return b.offset == current_offset; });

        if (it == blocks.end() || it->is_free) {
            // Block not found or already free
            return nullptr;
        }

        if (newSize == it->size) {
            // Size unchanged
            return address;
        }

        if (newSize < it->size) {
            // Shrink the block
            std::size_t remaining = it->size - newSize;
            it->size = newSize;
            blocks.emplace(it + 1, it->offset + newSize, remaining, true);
            merge_adjacent_free_blocks(it + 1);
            return address;
        }

        // Attempt to expand into the next free block if possible
        auto next_it = std::next(it);
        if (next_it != blocks.end() && next_it->is_free && (it->size + next_it->size) >= newSize) {
            std::size_t additional = newSize - it->size;
            if (next_it->size > additional) {
                // Split the next block
                next_it->offset += additional;
                next_it->size -= additional;
            } else {
                // Consume the entire next block
                additional = next_it->size;
                blocks.erase(next_it);
            }
            it->size += additional;
            return address;
        }

        // Allocate a new block and copy data
        T* new_address = allocate(newSize);
        if (new_address) {
            std::memcpy(new_address, address, it->size * sizeof(T));
            deallocate(address);
        }
        return new_address;
    }

    // Copy data from source to destination. Returns true on success.
    bool copy(T* sourceAddress, T* destinationAddress, std::size_t size) {
        if (!sourceAddress || !destinationAddress || size == 0) {
            return false;
        }

        if (!is_address_valid(sourceAddress) || !is_address_valid(destinationAddress)) {
            return false;
        }

        // Ensure source and destination have enough size
        std::size_t src_size = get_block_size(sourceAddress);
        std::size_t dest_size = get_block_size(destinationAddress);
        if (size > src_size || size > dest_size) {
            return false;
        }

        std::memcpy(destinationAddress, sourceAddress, size * sizeof(T));
        return true;
    }

    // Get a span representing the memory block containing the given address.
    std::optional<std::span<T>> getMemoryBlock(T* address) const {
        if (address < memory_pool.get() || address >= memory_pool.get() + pool_size) {
            return std::nullopt;
        }

        std::size_t offset = static_cast<std::size_t>(address - memory_pool.get());
        for (const auto& block : blocks) {
            if (offset >= block.offset && offset < block.offset + block.size) {
                return std::span<T>(memory_pool.get() + block.offset, block.size);
            }
        }
        return std::nullopt;
    }

    // Print the current state of memory
    void printMemoryState() const {
        std::size_t free_memory = std::accumulate(blocks.begin(), blocks.end(), 0ULL,
            [](std::size_t sum, const Block& block) { return sum + (block.is_free ? block.size : 0); });
        std::size_t used_memory = pool_size - free_memory;
        std::printf("Total Memory: %zu, Free Memory: %zu, Used Memory: %zu\n", 
                    pool_size, free_memory, used_memory);
        
        for (const auto& block : blocks) {
            std::printf("Block Offset: %zu, Size: %zu, %s\n",
                        block.offset, block.size,
                        block.is_free ? "Free" : "Used");
        }
    }

private:
    // Check if an address is valid and allocated
    bool is_address_valid(T* address) const {
        if (address < memory_pool.get() || address >= memory_pool.get() + pool_size) {
            return false;
        }
        std::size_t offset = static_cast<std::size_t>(address - memory_pool.get());
        for (const auto& block : blocks) {
            if (block.offset <= offset && offset < block.offset + block.size && !block.is_free) {
                return true;
            }
        }
        return false;
    }

    // Get the size of the block containing the given address
    std::size_t get_block_size(T* address) const {
        std::size_t offset = static_cast<std::size_t>(address - memory_pool.get());
        for (const auto& block : blocks) {
            if (block.offset <= offset && offset < block.offset + block.size) {
                return block.size - (offset - block.offset);
            }
        }
        return 0;
    }

    // Merge adjacent free blocks around the given iterator
    void merge_adjacent_free_blocks(typename std::vector<Block>::iterator it) {
        if (it == blocks.end()) return;

        // Merge with next blocks
        while (std::next(it) != blocks.end() && std::next(it)->is_free) {
            it->size += std::next(it)->size;
            blocks.erase(std::next(it));
        }

        // Merge with previous block
        if (it != blocks.begin()) {
            auto prev = std::prev(it);
            if (prev->is_free) {
                prev->size += it->size;
                blocks.erase(it);
            }
        }
    }
};
