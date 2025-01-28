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
#include <mutex>
#include <shared_mutex>
#include <unordered_set>
#include <iomanip>
#include <limits>

// Logging utility for debugging and error reporting
enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

inline void logMessage(LogLevel level, const std::string& message) {
    static std::mutex log_mutex;
    std::lock_guard<std::mutex> lock(log_mutex);
    switch (level) {
        case LogLevel::INFO: std::cout << "[INFO] "; break;
        case LogLevel::WARNING: std::cout << "[WARNING] "; break;
        case LogLevel::ERROR: std::cout << "[ERROR] "; break;
    }
    std::cout << message << std::endl;
}

template <typename T>
class MemoryManager {
private:
    struct Block {
        std::size_t offset;        // Offset in the memory pool
        std::size_t size;          // Size of the block
        bool is_free;              // Flag indicating if the block is free

        Block(std::size_t off, std::size_t sz, bool free = true)
            : offset(off), size(sz), is_free(free) {}
    };

    std::unique_ptr<char[]> memory_pool;  // Raw memory pool
    std::size_t pool_size;                // Total size of the memory pool
    std::vector<Block> blocks;            // List of memory blocks

    mutable std::shared_mutex manager_mutex; // Mutex for thread safety

    // Set to track allocated block offsets for leak detection
    std::unordered_set<std::size_t> allocated_blocks;

    // Alignment requirements
    static constexpr std::size_t alignment = alignof(T);

    // Helper function to align a given size
    static std::size_t align_up(std::size_t size, std::size_t align) {
        return (size + align - 1) & ~(align - 1);
    }

public:
    // Constructor: Initialize memory manager with a total memory size
    explicit MemoryManager(std::size_t totalSize)
        : pool_size(align_up(totalSize, alignment)), // Ensure pool size is aligned
          memory_pool(std::make_unique<char[]>(pool_size)) {
        if (pool_size < alignment) {
            throw std::invalid_argument("Pool size too small for alignment.");
        }
        blocks.emplace_back(0, pool_size, true); // Start with one large free block
        logMessage(LogLevel::INFO, "MemoryManager initialized with pool size: " + std::to_string(pool_size));
    }

    // Destructor: Detect memory leaks
    ~MemoryManager() {
        std::shared_lock<std::shared_mutex> lock(manager_mutex);
        if (!allocated_blocks.empty()) {
            logMessage(LogLevel::ERROR, "Memory leaks detected:");
            for (const auto& offset : allocated_blocks) {
                logMessage(LogLevel::ERROR, " - Leaked block at offset " + std::to_string(offset));
            }
        } else {
            logMessage(LogLevel::INFO, "No memory leaks detected.");
        }
    }

    // Disable copy and move semantics
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    // Allocate memory of given size. Returns pointer to allocated memory or nullptr if failed.
    T* allocate(std::size_t size) {
        std::unique_lock<std::shared_mutex> lock(manager_mutex);
        if (size == 0) {
            logMessage(LogLevel::WARNING, "Attempted to allocate 0 size.");
            return nullptr; // Invalid size
        }

        // Calculate the size in bytes, considering the size of T
        std::size_t bytes_needed = align_up(size * sizeof(T), alignment);
        logMessage(LogLevel::INFO, "Allocating " + std::to_string(size) + " objects (" + std::to_string(bytes_needed) + " bytes).");

        // Best-Fit: Find the smallest free block that fits the requested size
        auto it = blocks.end();
        std::size_t best_size = std::numeric_limits<std::size_t>::max();
        for (auto current = blocks.begin(); current != blocks.end(); ++current) {
            if (current->is_free && current->size >= bytes_needed) {
                if (current->size < best_size) {
                    best_size = current->size;
                    it = current;
                }
            }
        }

        if (it == blocks.end()) {
            logMessage(LogLevel::ERROR, "Allocation failed: Not enough memory.");
            return nullptr;
        }

        // Allocate memory from the selected block
        std::size_t allocated_offset = it->offset;
        T* ptr = reinterpret_cast<T*>(memory_pool.get() + allocated_offset);

        if (it->size > bytes_needed) {
            // Split the block into allocated and remaining free parts
            Block allocated_block(allocated_offset, bytes_needed, false);
            Block remaining_block(allocated_offset + bytes_needed, it->size - bytes_needed, true);
            // Replace the current block with allocated and remaining blocks
            it = blocks.erase(it);
            it = blocks.emplace(it, allocated_block);
            blocks.emplace(it + 1, remaining_block);
        } else {
            // Exact fit; mark block as used
            it->is_free = false;
        }

        allocated_blocks.insert(allocated_offset);
        logMessage(LogLevel::INFO, "Allocated at offset: " + std::to_string(allocated_offset));
        return ptr;
    }

    // Deallocate memory at given address. Returns true if successful, false otherwise.
    bool deallocate(T* address) {
        if (address == nullptr) {
            logMessage(LogLevel::WARNING, "Attempted to deallocate a null pointer.");
            return false; // Null pointer cannot be deallocated
        }

        std::unique_lock<std::shared_mutex> lock(manager_mutex);
        if (!is_address_valid(address)) {
            logMessage(LogLevel::ERROR, "Deallocation failed: Invalid address.");
            return false;
        }

        std::size_t offset = reinterpret_cast<char*>(address) - memory_pool.get();
        auto it = std::find_if(blocks.begin(), blocks.end(),
            [offset](const Block& b) { return b.offset == offset; });

        if (it == blocks.end() || it->is_free) {
            logMessage(LogLevel::ERROR, "Deallocation failed: Block not found or already free.");
            return false;
        }

        it->is_free = true;
        allocated_blocks.erase(it->offset);
        logMessage(LogLevel::INFO, "Deallocated block at offset: " + std::to_string(it->offset));

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

        std::unique_lock<std::shared_mutex> lock(manager_mutex);
        if (!is_address_valid(address)) {
            logMessage(LogLevel::ERROR, "Reallocation failed: Invalid address.");
            return nullptr;
        }

        std::size_t current_offset = reinterpret_cast<char*>(address) - memory_pool.get();
        auto it = std::find_if(blocks.begin(), blocks.end(),
            [current_offset](const Block& b) { return b.offset == current_offset; });

        if (it == blocks.end() || it->is_free) {
            logMessage(LogLevel::ERROR, "Reallocation failed: Block not found or is free.");
            return nullptr;
        }

        std::size_t current_size = it->size;
        std::size_t new_bytes_needed = align_up(newSize * sizeof(T), alignment);

        if (new_bytes_needed == current_size) {
            logMessage(LogLevel::INFO, "Reallocation not required: Size unchanged.");
            return address;
        }

        if (new_bytes_needed < current_size) {
            // Shrink the block
            std::size_t remaining = current_size - new_bytes_needed;
            it->size = new_bytes_needed;
            Block new_free(it->offset + new_bytes_needed, remaining, true);
            auto inserted = blocks.emplace(it + 1, new_free);
            merge_adjacent_free_blocks(inserted);
            logMessage(LogLevel::INFO, "Shrunk block at offset " + std::to_string(it->offset) +
                                       " from " + std::to_string(current_size) +
                                       " to " + std::to_string(new_bytes_needed));
            return address;
        }

        // Attempt to expand the block in place by checking the next block
        auto next_it = std::next(it);
        if (next_it != blocks.end() && next_it->is_free) {
            std::size_t combined_size = it->size + next_it->size;
            if (combined_size >= new_bytes_needed) {
                std::size_t additional = new_bytes_needed - it->size;
                if (additional <= next_it->size) {
                    // Take 'additional' bytes from the next block
                    it->size += additional;
                    if (next_it->size > additional) {
                        next_it->offset += additional;
                        next_it->size -= additional;
                        merge_adjacent_free_blocks(next_it);
                    } else {
                        blocks.erase(next_it);
                    }
                    logMessage(LogLevel::INFO, "Expanded block at offset " + std::to_string(it->offset) +
                                               " to size " + std::to_string(it->size));
                    return address;
                }
            }
        }

        // Allocate a new block and copy data
        T* new_address = allocate(newSize);
        if (new_address) {
            std::memcpy(new_address, address, current_size);
            deallocate(address);
            logMessage(LogLevel::INFO, "Reallocated block from offset " + std::to_string(it->offset) +
                                       " to new offset " + std::to_string(reinterpret_cast<char*>(new_address) - memory_pool.get()));
        } else {
            logMessage(LogLevel::ERROR, "Reallocation failed: Unable to allocate new block.");
        }
        return new_address;
    }

    // Copy data from source to destination. Returns true on success.
    bool copy(T* sourceAddress, T* destinationAddress, std::size_t count) {
        static_assert(std::is_trivially_copyable_v<T>, "copy requires T to be trivially copyable.");

        if (sourceAddress == nullptr || destinationAddress == nullptr || count == 0) {
            logMessage(LogLevel::WARNING, "Invalid parameters for copy operation.");
            return false;
        }

        std::shared_lock<std::shared_mutex> lock(manager_mutex);
        if (!is_address_valid(sourceAddress) || !is_address_valid(destinationAddress)) {
            logMessage(LogLevel::ERROR, "Copy failed: Invalid source or destination address.");
            return false;
        }

        std::size_t src_size = get_block_size(sourceAddress);
        std::size_t dest_size = get_block_size(destinationAddress);
        std::size_t bytes_to_copy = std::min({count * sizeof(T), src_size, dest_size});

        std::memcpy(destinationAddress, sourceAddress, bytes_to_copy);
        logMessage(LogLevel::INFO, "Copied " + std::to_string(bytes_to_copy) + " bytes from source to destination.");
        return true;
    }

    // Get a span representing the memory block containing the given address.
    std::optional<std::span<T>> getMemoryBlock(T* address) const {
        if (address < reinterpret_cast<T*>(memory_pool.get()) ||
            address >= reinterpret_cast<T*>(memory_pool.get() + pool_size)) {
            return std::nullopt;
        }

        std::shared_lock<std::shared_mutex> lock(manager_mutex);
        std::size_t offset = reinterpret_cast<char*>(address) - memory_pool.get();
        for (const auto& block : blocks) {
            if (offset >= block.offset && offset < block.offset + block.size) {
                if (!block.is_free) {
                    std::size_t count = block.size / sizeof(T);
                    return std::span<T>(reinterpret_cast<T*>(memory_pool.get() + block.offset), count);
                }
            }
        }
        return std::nullopt;
    }

    // Print the current state of memory
    void printMemoryState() const {
        std::shared_lock<std::shared_mutex> lock(manager_mutex);
        std::size_t free_memory = std::accumulate(blocks.begin(), blocks.end(), 0ULL,
            [](std::size_t sum, const Block& block) { return sum + (block.is_free ? block.size : 0); });
        std::size_t used_memory = pool_size - free_memory;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Memory State:" << std::endl;
        std::cout << "Total Memory: " << pool_size << " bytes" << std::endl;
        std::cout << "Used Memory: " << used_memory << " bytes" << std::endl;
        std::cout << "Free Memory: " << free_memory << " bytes" << std::endl;
        std::cout << "Blocks:" << std::endl;
        std::cout << std::left << std::setw(10) << "Offset"
                  << std::setw(10) << "Size"
                  << std::setw(10) << "Status" << std::endl;
        for (const auto& block : blocks) {
            std::cout << std::left << std::setw(10) << block.offset
                      << std::setw(10) << block.size
                      << std::setw(10) << (block.is_free ? "Free" : "Used") << std::endl;
        }
        std::cout << "----------------------------------------" << std::endl;
    }

private:
    // Check if an address is valid and allocated
    bool is_address_valid(T* address) const {
        if (address < reinterpret_cast<T*>(memory_pool.get()) ||
            address >= reinterpret_cast<T*>(memory_pool.get() + pool_size)) {
            return false;
        }
        std::size_t offset = reinterpret_cast<char*>(address) - memory_pool.get();
        return allocated_blocks.find(offset) != allocated_blocks.end();
    }

    // Get the size of the block containing the given address
    std::size_t get_block_size(T* address) const {
        std::size_t offset = reinterpret_cast<char*>(address) - memory_pool.get();
        for (const auto& block : blocks) {
            if (block.offset <= offset && offset < block.offset + block.size) {
                return block.size;
            }
        }
        return 0;
    }

    // Merge adjacent free blocks around the given iterator
    void merge_adjacent_free_blocks(typename std::vector<Block>::iterator it) {
        if (it == blocks.end() || !it->is_free) return;

        // Merge with next blocks
        while (std::next(it) != blocks.end() && std::next(it)->is_free) {
            it->size += std::next(it)->size;
            blocks.erase(std::next(it));
            logMessage(LogLevel::INFO, "Merged with next free block. New size: " + std::to_string(it->size));
        }

        // Merge with previous block
        if (it != blocks.begin()) {
            auto prev = std::prev(it);
            if (prev->is_free) {
                prev->size += it->size;
                blocks.erase(it);
                logMessage(LogLevel::INFO, "Merged with previous free block. New size: " + std::to_string(prev->size));
            }
        }
    }
};
