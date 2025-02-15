#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <new>

template <typename T>
class MemoryManager {
    struct alignas(alignof(std::max_align_t)) BlockHeader {
        size_t size;
        bool is_free;
        BlockHeader* next;
        BlockHeader* prev;
    };

    struct alignas(alignof(std::max_align_t)) AllocHeader {
        size_t size;
        size_t offset;
    };

    static constexpr size_t ALIGNMENT = alignof(std::max_align_t);
    static constexpr size_t HEADER_SIZE = sizeof(AllocHeader);

    struct BlockCompare {
        using is_transparent = void;
        bool operator()(const BlockHeader* a, const BlockHeader* b) const noexcept {
            return a->size < b->size;
        }
        bool operator()(const BlockHeader* a, size_t b_size) const noexcept {
            return a->size < b_size;
        }
        bool operator()(size_t a_size, const BlockHeader* b) const noexcept {
            return a_size < b->size;
        }
    };

    uint8_t* memory_pool;
    size_t pool_size;
    BlockHeader* head;
    std::multiset<BlockHeader*, BlockCompare> free_blocks;
    mutable std::mutex mutex;

    static size_t align_up(size_t size, size_t align) noexcept {
        return (size + align - 1) & ~(align - 1);
    }

    void coalesce(BlockHeader* block) noexcept {
        // Merge with previous block if free
        if (block->prev && block->prev->is_free) {
            auto prev_it = free_blocks.find(block->prev);
            if (prev_it != free_blocks.end()) {
                free_blocks.erase(prev_it);
                free_blocks.erase(free_blocks.find(block));
                
                block->prev->size += block->size + sizeof(BlockHeader);
                block->prev->next = block->next;
                if (block->next) {
                    block->next->prev = block->prev;
                }
                block = block->prev;
                
                free_blocks.insert(block);
            }
        }

        // Merge with next block if free
        if (block->next && block->next->is_free) {
            auto next_it = free_blocks.find(block->next);
            if (next_it != free_blocks.end()) {
                free_blocks.erase(next_it);
                free_blocks.erase(free_blocks.find(block));
                
                block->size += block->next->size + sizeof(BlockHeader);
                block->next = block->next->next;
                if (block->next) {
                    block->next->prev = block;
                }
                
                free_blocks.insert(block);
            }
        }
    }

public:
    MemoryManager(size_t totalSize) 
        : pool_size(align_up(totalSize, ALIGNMENT)),
          memory_pool(static_cast<uint8_t*>(::operator new(pool_size))),
          head(new (memory_pool) BlockHeader{pool_size - sizeof(BlockHeader), true, nullptr, nullptr}) {
        free_blocks.insert(head);
    }

    ~MemoryManager() {
        ::operator delete(memory_pool);
    }

    T* allocate(size_t count) {
        if (count == 0) return nullptr;

        std::lock_guard<std::mutex> lock(mutex);
        const size_t user_data_size = align_up(count * sizeof(T), ALIGNMENT);
        const size_t required = user_data_size + HEADER_SIZE;

        auto it = free_blocks.lower_bound(required);
        if (it == free_blocks.end()) throw std::bad_alloc();

        BlockHeader* block = *it;
        free_blocks.erase(it);

        const size_t remaining = block->size - required;
        if (remaining > sizeof(BlockHeader) + ALIGNMENT) {
            BlockHeader* new_block = reinterpret_cast<BlockHeader*>(
                reinterpret_cast<uint8_t*>(block) + sizeof(BlockHeader) + required);
            new_block->size = remaining - sizeof(BlockHeader);
            new_block->is_free = true;
            new_block->prev = block;
            new_block->next = block->next;
            
            if (block->next) block->next->prev = new_block;
            block->next = new_block;
            block->size = required;
            free_blocks.insert(new_block);
        }

        block->is_free = false;
        AllocHeader* header = reinterpret_cast<AllocHeader*>(block + 1);
        header->size = block->size;
        header->offset = reinterpret_cast<uint8_t*>(block) - memory_pool;

        return reinterpret_cast<T*>(header + 1);
    }

    void deallocate(T* ptr) noexcept {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex);
        AllocHeader* header = reinterpret_cast<AllocHeader*>(ptr) - 1;
        if (header->offset >= pool_size) return;

        BlockHeader* block = reinterpret_cast<BlockHeader*>(memory_pool + header->offset);
        if (!block->is_free) {
            block->is_free = true;
            coalesce(block);
            free_blocks.insert(block);
        }
    }

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    MemoryManager(MemoryManager&& other) noexcept 
        : memory_pool(other.memory_pool),
          pool_size(other.pool_size),
          head(other.head),
          free_blocks(std::move(other.free_blocks)) {
        other.memory_pool = nullptr;
        other.head = nullptr;
    }

    MemoryManager& operator=(MemoryManager&& other) noexcept {
        if (this != &other) {
            ::operator delete(memory_pool);
            memory_pool = other.memory_pool;
            pool_size = other.pool_size;
            head = other.head;
            free_blocks = std::move(other.free_blocks);
            other.memory_pool = nullptr;
            other.head = nullptr;
        }
        return *this;
    }
};
