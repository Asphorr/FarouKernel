#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <algorithm>
#include <mutex>
#include <stdexcept>

template <typename T>
class MemoryManager {
    struct Block {
        size_t offset;
        size_t size;
        Block* next;
    };

    struct FreeBlock : Block {
        FreeBlock* next_free;
        FreeBlock* prev_free;
    };

    struct AllocHeader {
        size_t size;
    };

    static constexpr size_t ALIGNMENT = alignof(T);
    static constexpr size_t HEADER_SIZE = (sizeof(AllocHeader) + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    uint8_t* memory_pool;
    size_t pool_size;
    FreeBlock* free_list;
    Block* block_list;
    std::mutex mutex;

    static size_t align_up(size_t size, size_t align) {
        return (size + align - 1) & ~(align - 1);
    }

    void coalesce(FreeBlock* block) {
        for (FreeBlock* curr = block;;) {
            FreeBlock* next = reinterpret_cast<FreeBlock*>(
                reinterpret_cast<uint8_t*>(curr) + curr->size);
            
            if (reinterpret_cast<uint8_t*>(next) >= memory_pool + pool_size) break;
            if (next->next_free == nullptr && !next->next) break;
            
            curr->size += next->size;
            curr->next = next->next;
            if (next->next) next->next->next = curr;
        }
    }

public:
    MemoryManager(size_t totalSize) : pool_size(align_up(totalSize, ALIGNMENT)) {
        memory_pool = new uint8_t[pool_size];
        block_list = new Block{0, pool_size, nullptr};
        free_list = static_cast<FreeBlock*>(block_list);
        free_list->next_free = nullptr;
        free_list->prev_free = nullptr;
    }

    ~MemoryManager() {
        delete[] memory_pool;
        while (block_list) {
            Block* next = block_list->next;
            delete block_list;
            block_list = next;
        }
    }

    T* allocate(size_t count) {
        std::lock_guard<std::mutex> lock(mutex);
        if (count == 0) return nullptr;

        const size_t required = align_up(count * sizeof(T) + HEADER_SIZE, ALIGNMENT);
        FreeBlock* best = nullptr;
        FreeBlock* curr = free_list;

        while (curr) {
            if (curr->size >= required && (!best || curr->size < best->size)) {
                best = curr;
                if (curr->size == required) break;
            }
            curr = curr->next_free;
        }

        if (!best) return nullptr;

        if (best->size > required) {
            FreeBlock* remainder = new FreeBlock{
                {best->offset + required, best->size - required, best->next},
                nullptr, nullptr};
            
            best->size = required;
            best->next = remainder;
            
            if (free_list == best) free_list = remainder;
            remainder->next_free = best->next_free;
            remainder->prev_free = best->prev_free;
        }

        if (best->prev_free) best->prev_free->next_free = best->next_free;
        if (best->next_free) best->next_free->prev_free = best->prev_free;
        if (free_list == best) free_list = best->next_free;

        auto header = reinterpret_cast<AllocHeader*>(memory_pool + best->offset);
        header->size = best->size;
        return reinterpret_cast<T*>(memory_pool + best->offset + HEADER_SIZE);
    }

    void deallocate(T* ptr) {
        if (!ptr) return;
        std::lock_guard<std::mutex> lock(mutex);

        AllocHeader* header = reinterpret_cast<AllocHeader*>(
            reinterpret_cast<uint8_t*>(ptr) - HEADER_SIZE);
        
        FreeBlock* new_block = new FreeBlock{
            {static_cast<size_t>(reinterpret_cast<uint8_t*>(header) - memory_pool),
             header->size, block_list},
            nullptr, nullptr};
        
        block_list = new_block;
        new_block->next_free = free_list;
        if (free_list) free_list->prev_free = new_block;
        free_list = new_block;
        
        coalesce(new_block);
    }
};
