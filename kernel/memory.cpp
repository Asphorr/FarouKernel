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
#include <iterator>

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
    static_assert(std::is_trivially_copyable_v<T>, 
                 "MemoryManager requires trivially copyable types");

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
    std::vector<Block> blocks;            // List of memory blocks (sorted by offset)

    mutable std::shared_mutex manager_mutex;
    std::unordered_set<std::size_t> allocated_blocks;

    static constexpr std::size_t alignment = alignof(T);

    static std::size_t align_up(std::size_t size, std::size_t align) {
        return (size + align - 1) & ~(align - 1);
    }

public:
    explicit MemoryManager(std::size_t totalSize)
        : pool_size(align_up(totalSize, alignment)),
          memory_pool(std::make_unique<char[]>(pool_size)) {
        if (pool_size < alignment) {
            throw std::invalid_argument("Pool size too small for alignment.");
        }
        blocks.emplace_back(0, pool_size, true);
        logMessage(LogLevel::INFO, "MemoryManager initialized with pool size: " + std::to_string(pool_size));
    }

    ~MemoryManager() {
        std::shared_lock lock(manager_mutex);
        if (!allocated_blocks.empty()) {
            logMessage(LogLevel::ERROR, "Memory leaks detected:");
            for (const auto& offset : allocated_blocks) {
                logMessage(LogLevel::ERROR, " - Leaked block at offset " + std::to_string(offset));
            }
        }
    }

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;

    T* allocate(std::size_t size) {
        std::unique_lock lock(manager_mutex);
        if (size == 0 || size > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
            logMessage(LogLevel::WARNING, "Invalid allocation size");
            return nullptr;
        }

        const std::size_t bytes_needed = align_up(size * sizeof(T), alignment);
        auto best_it = blocks.end();
        std::size_t best_size = std::numeric_limits<std::size_t>::max();

        // Binary search for best fit
        auto it = std::lower_bound(blocks.begin(), blocks.end(), bytes_needed,
            [](const Block& b, std::size_t sz) { return b.size < sz; });

        while (it != blocks.end()) {
            if (it->is_free && it->size >= bytes_needed) {
                if (it->size < best_size) {
                    best_size = it->size;
                    best_it = it;
                }
            }
            ++it;
        }

        if (best_it == blocks.end()) {
            logMessage(LogLevel::ERROR, "Allocation failed: Not enough memory");
            return nullptr;
        }

        const std::size_t allocated_offset = best_it->offset;
        T* ptr = reinterpret_cast<T*>(memory_pool.get() + allocated_offset);

        if (best_it->size > bytes_needed) {
            Block remaining(allocated_offset + bytes_needed, 
                          best_it->size - bytes_needed, true);
            best_it = blocks.erase(best_it);
            best_it = blocks.emplace(best_it, Block{allocated_offset, bytes_needed, false});
            blocks.emplace(best_it + 1, remaining);
        } else {
            best_it->is_free = false;
        }

        allocated_blocks.insert(allocated_offset);
        return ptr;
    }

    bool deallocate(T* address) {
        if (!address) {
            logMessage(LogLevel::WARNING, "Attempted to deallocate nullptr");
            return false;
        }

        std::unique_lock lock(manager_mutex);
        const std::size_t offset = reinterpret_cast<char*>(address) - memory_pool.get();
        
        auto it = std::lower_bound(blocks.begin(), blocks.end(), offset,
            [](const Block& b, std::size_t o) { return b.offset < o; });

        if (it == blocks.end() || it->offset != offset || it->is_free) {
            logMessage(LogLevel::ERROR, "Deallocation failed: Invalid address");
            return false;
        }

        it->is_free = true;
        allocated_blocks.erase(offset);
        merge_adjacent_free_blocks(it);
        return true;
    }

    T* reallocate(T* address, std::size_t newSize) {
        if (!address) return allocate(newSize);
        if (newSize == 0) {
            deallocate(address);
            return nullptr;
        }

        std::unique_lock lock(manager_mutex);
        const std::size_t current_offset = reinterpret_cast<char*>(address) - memory_pool.get();
        
        auto it = std::lower_bound(blocks.begin(), blocks.end(), current_offset,
            [](const Block& b, std::size_t o) { return b.offset < o; });

        if (it == blocks.end() || it->offset != current_offset || it->is_free) {
            logMessage(LogLevel::ERROR, "Reallocation failed: Invalid address");
            return nullptr;
        }

        const std::size_t current_size = it->size;
        const std::size_t new_bytes_needed = align_up(newSize * sizeof(T), alignment);

        if (new_bytes_needed == current_size) return address;

        // Try to expand in place
        if (new_bytes_needed < current_size) {
            it->size = new_bytes_needed;
            Block remaining(it->offset + new_bytes_needed, 
                          current_size - new_bytes_needed, true);
            blocks.insert(it + 1, remaining);
            return address;
        }

        // Try to merge with next block
        auto next_it = it + 1;
        if (next_it != blocks.end() && next_it->is_free) {
            const std::size_t combined_size = it->size + next_it->size;
            if (combined_size >= new_bytes_needed) {
                it->size = new_bytes_needed;
                if (combined_size > new_bytes_needed) {
                    Block remaining(it->offset + new_bytes_needed,
                                  combined_size - new_bytes_needed, true);
                    *next_it = remaining;
                } else {
                    blocks.erase(next_it);
                }
                return address;
            }
        }

        // Manual allocation to avoid deadlock
        const std::size_t required = new_bytes_needed - it->size;
        auto best_it = blocks.end();
        std::size_t best_size = std::numeric_limits<std::size_t>::max();

        for (auto blk = blocks.begin(); blk != blocks.end(); ++blk) {
            if (blk->is_free && blk->size >= required) {
                if (blk->size < best_size) {
                    best_size = blk->size;
                    best_it = blk;
                }
            }
        }

        if (best_it == blocks.end()) {
            logMessage(LogLevel::ERROR, "Reallocation failed: No contiguous space");
            return nullptr;
        }

        // Expand current block
        it->size += required;
        if (best_it->size > required) {
            Block remaining(best_it->offset + required,
                          best_it->size - required, true);
            *best_it = remaining;
        } else {
            blocks.erase(best_it);
        }

        return address;
    }

        bool copy(T* src, T* dest, std::size_t count) {
        std::shared_lock lock(manager_mutex);
        if (!src || !dest || count == 0) {
            logMessage(LogLevel::WARNING, "Invalid copy parameters");
            return false;
        }

        const std::size_t src_offset = reinterpret_cast<char*>(src) - memory_pool.get();
        const std::size_t dest_offset = reinterpret_cast<char*>(dest) - memory_pool.get();

        // Validate both blocks exist and are allocated
        auto src_block = find_block(src_offset);
        auto dest_block = find_block(dest_offset);
        
        if (!src_block || !dest_block || src_block->is_free || dest_block->is_free) {
            logMessage(LogLevel::ERROR, "Copy failed: Invalid memory blocks");
            return false;
        }

        // Calculate safe copy limits
        const std::size_t src_available = src_block->size - (src_offset - src_block->offset);
        const std::size_t dest_available = dest_block->size - (dest_offset - dest_block->offset);
        const std::size_t bytes_to_copy = std::min({
            count * sizeof(T), 
            src_available, 
            dest_available
        });

        if (bytes_to_copy == 0) {
            logMessage(LogLevel::ERROR, "Copy failed: No space in target block");
            return false;
        }

        std::memcpy(dest, src, bytes_to_copy);
        logMessage(LogLevel::INFO, "Copied " + std::to_string(bytes_to_copy) + " bytes");
        return true;
    }

private:
    // Unified block finding with binary search
    typename std::vector<Block>::iterator find_block(std::size_t offset) {
        auto it = std::lower_bound(blocks.begin(), blocks.end(), offset,
            [](const Block& b, std::size_t o) { return b.offset < o; });
        
        if (it != blocks.begin() && (it == blocks.end() || it->offset > offset)) {
            --it;
        }

        if (it != blocks.end() && offset >= it->offset && offset < it->offset + it->size) {
            return it;
        }
        return blocks.end();
    }

    // Enhanced merging logic with range validation
    void merge_adjacent_free_blocks(typename std::vector<Block>::iterator it) {
        if (it == blocks.end() || !it->is_free) return;

        // Merge with next blocks
        while (it + 1 != blocks.end() && (it + 1)->is_free) {
            it->size += (it + 1)->size;
            blocks.erase(it + 1);
        }

        // Merge with previous blocks
        while (it != blocks.begin() && (it - 1)->is_free) {
            (it - 1)->size += it->size;
            it = blocks.erase(it);
            --it;
        }
    }
};
