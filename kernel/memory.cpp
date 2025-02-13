#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <new>
#include <algorithm>
#include <atomic>

template <typename T>
class MemoryManager {
    struct BlockHeader {
        size_t size;
        bool is_free;
        BlockHeader* next;
        BlockHeader* prev;
    };

    struct AllocHeader {
        size_t size;
        size_t offset;
    };

    static constexpr size_t ALIGNMENT = alignof(std::max_align_t);
    static constexpr size_t HEADER_SIZE = (sizeof(AllocHeader) + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

    uint8_t* memory_pool;
    size_t pool_size;
    BlockHeader* head;
    std::multiset<BlockHeader*, bool(*)(BlockHeader*, BlockHeader*)> free_blocks;
    mutable std::mutex mutex;

    static size_t align_up(size_t size, size_t align) {
        return (size + align - 1) & ~(align - 1);
    }

    void coalesce(BlockHeader* block) {
        if (block->prev && block->prev->is_free) {
            block->prev->size += block->size + sizeof(BlockHeader);
            block->prev->next = block->next;
            if (block->next) block->next->prev = block->prev;
            free_blocks.erase(block);
            block = block->prev;
        }

        if (block->next && block->next->is_free) {
            block->size += block->next->size + sizeof(BlockHeader);
            block->next = block->next->next;
            if (block->next) block->next->prev = block;
            free_blocks.erase(block->next);
        }

        free_blocks.insert(block);
    }

public:
    MemoryManager(size_t totalSize) 
        : pool_size(align_up(totalSize, ALIGNMENT)),
          free_blocks([](BlockHeader* a, BlockHeader* b) { return a->size < b->size; }) {
        memory_pool = static_cast<uint8_t*>(::operator new(pool_size));
        head = new (memory_pool) BlockHeader{pool_size - sizeof(BlockHeader), true, nullptr, nullptr};
        free_blocks.insert(head);
    }

    ~MemoryManager() {
        ::operator delete(memory_pool);
    }

    T* allocate(size_t count) {
        std::lock_guard<std::mutex> lock(mutex);
        if (count == 0) return nullptr;

        const size_t required = align_up(count * sizeof(T), ALIGNMENT) + HEADER_SIZE;
        auto it = std::find_if(free_blocks.begin(), free_blocks.end(),
            [required](BlockHeader* b) { return b->size >= required; });

        if (it == free_blocks.end()) throw std::bad_alloc();

        BlockHeader* block = *it;
        free_blocks.erase(it);

        if (block->size > required + sizeof(BlockHeader)) {
            BlockHeader* new_block = reinterpret_cast<BlockHeader*>(
                reinterpret_cast<uint8_t*>(block) + sizeof(BlockHeader) + required);
            new_block->size = block->size - required - sizeof(BlockHeader);
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

    void deallocate(T* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex);
        AllocHeader* header = reinterpret_cast<AllocHeader*>(ptr) - 1;
        
        if (header->offset >= pool_size) 
            throw std::invalid_argument("Invalid pointer");

        BlockHeader* block = reinterpret_cast<BlockHeader*>(memory_pool + header->offset);
        if (!block->is_free) {
            block->is_free = true;
            free_blocks.insert(block);
            coalesce(block);
        } else {
            throw std::runtime_error("Double free detected");
        }
    }

    // Запрещаем копирование и присваивание
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    // Поддержка move-семантики
    MemoryManager(MemoryManager&& other) noexcept 
        : memory_pool(other.memory_pool),
          pool_size(other.pool_size),
          head(other.head),
          free_blocks(std::move(other.free_blocks)),
          mutex() {
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
