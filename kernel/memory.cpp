#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <new>
#include <iostream> // For potential debugging

// Helper function for alignment
inline size_t align_up(size_t size, size_t align) noexcept {
    return (size + align - 1) & ~(align - 1);
}

template <typename T>
class MemoryManager {
    static constexpr size_t ALIGNMENT = alignof(std::max_align_t);

    // Header placed *before* the user data area in *every* block
    struct alignas(ALIGNMENT) BlockHeader {
        size_t size;      // Size of the *entire* block (header + user_data + footer)
        bool is_free;
        BlockHeader* next_free; // Pointer to next block in the free list (if free)
        BlockHeader* prev_free; // Pointer to previous block in the free list (if free)
        // No physical next/prev needed, calculable from size and footer
    };

    // Footer placed *after* the user data area in *every* block
    struct alignas(ALIGNMENT) BlockFooter {
         size_t size; // Size of the *entire* block (header + user_data + footer)
         // bool is_free; // Could duplicate status here for robustness/debugging
    };

    static constexpr size_t HEADER_SIZE = align_up(sizeof(BlockHeader), ALIGNMENT);
    static constexpr size_t FOOTER_SIZE = align_up(sizeof(BlockFooter), ALIGNMENT);
    // Minimum size for a block to hold header, footer, and potentially free list pointers
    static constexpr size_t MIN_BLOCK_SIZE = HEADER_SIZE + FOOTER_SIZE + ALIGNMENT; // Must be at least large enough for header/footer

    uint8_t* memory_pool;
    size_t pool_size;
    BlockHeader* free_list_head = nullptr; // Head of the explicit free list
    mutable std::mutex mutex;

    // --- Helper functions for block manipulation ---

    // Get pointer to the footer of a block given the header
    BlockFooter* get_footer(BlockHeader* header) const noexcept {
        return reinterpret_cast<BlockFooter*>(
            reinterpret_cast<uint8_t*>(header) + header->size - FOOTER_SIZE
        );
    }

    // Get pointer to the header of the physically *next* block
    BlockHeader* get_next_physical_block(BlockHeader* header) const noexcept {
        uint8_t* next_block_ptr = reinterpret_cast<uint8_t*>(header) + header->size;
        if (next_block_ptr >= memory_pool + pool_size) {
            return nullptr; // Reached end of pool
        }
        return reinterpret_cast<BlockHeader*>(next_block_ptr);
    }

    // Get pointer to the header of the physically *previous* block
    BlockHeader* get_prev_physical_block(BlockHeader* header) const noexcept {
        if (reinterpret_cast<uint8_t*>(header) == memory_pool) {
            return nullptr; // First block in pool
        }
        // Use the footer of the *current* block to find the size of the *previous* block
        BlockFooter* prev_footer = reinterpret_cast<BlockFooter*>(
            reinterpret_cast<uint8_t*>(header) - FOOTER_SIZE
        );
        // Now calculate the start of the previous block's header
        return reinterpret_cast<BlockHeader*>(
            reinterpret_cast<uint8_t*>(header) - prev_footer->size
        );
    }

    // Get pointer to the user data area within a block
    void* get_user_data(BlockHeader* header) const noexcept {
        return reinterpret_cast<void*>(
            reinterpret_cast<uint8_t*>(header) + HEADER_SIZE
        );
    }

    // Get pointer to the header from a user data pointer
    BlockHeader* get_header_from_user_data(void* ptr) const noexcept {
         return reinterpret_cast<BlockHeader*>(
            reinterpret_cast<uint8_t*>(ptr) - HEADER_SIZE
        );
    }

    // --- Free List Management ---

    // Remove a block from the free list
    void remove_from_free_list(BlockHeader* block) noexcept {
        if (block->prev_free) {
            block->prev_free->next_free = block->next_free;
        } else {
            // Block was the head
            free_list_head = block->next_free;
        }
        if (block->next_free) {
            block->next_free->prev_free = block->prev_free;
        }
        block->next_free = nullptr;
        block->prev_free = nullptr;
    }

    // Add a block to the front of the free list
    void add_to_free_list(BlockHeader* block) noexcept {
        block->next_free = free_list_head;
        block->prev_free = nullptr;
        if (free_list_head) {
            free_list_head->prev_free = block;
        }
        free_list_head = block;
        block->is_free = true; // Ensure it's marked free
    }

    // --- Coalescing ---
    BlockHeader* coalesce(BlockHeader* block) noexcept {
        BlockHeader* next_block = get_next_physical_block(block);
        BlockHeader* prev_block = get_prev_physical_block(block);

        bool merged_prev = false;
        bool merged_next = false;

        // Coalesce with previous block
        if (prev_block && prev_block->is_free) {
            remove_from_free_list(prev_block); // Remove neighbor from list
            prev_block->size += block->size;
            get_footer(prev_block)->size = prev_block->size; // Update footer of combined block
            block = prev_block; // The combined block starts at prev_block
            merged_prev = true;
        }

        // Coalesce with next block
        if (next_block && next_block->is_free) {
            remove_from_free_list(next_block); // Remove neighbor from list
            block->size += next_block->size;
            get_footer(block)->size = block->size; // Update footer of combined block
            merged_next = true;
        }

        // If we didn't merge with previous, the original block (or block+next) needs to be added
        // If we did merge with previous, prev_block (now representing the merged block) needs to be added
        // If we merged both, prev_block (representing all three) needs to be added
        // In all cases where merging happened or the original block is freed, 'block' points to the start
        // of the final coalesced free block. Add it back to the free list.
        // If the block was *already* in the free list (e.g., during initial setup or if coalesce is called redundantly),
        // we might add it twice if not careful. Coalesce should only be called *after* marking a block free
        // and *before* adding it to the list, or it should handle removing the block itself first if needed.
        // Let's assume coalesce is called *before* adding the newly freed block to the list.

        return block; // Return the potentially larger, coalesced block
    }


public:
    explicit MemoryManager(size_t totalSize)
        : pool_size(align_up(totalSize, ALIGNMENT)) {
        if (pool_size < MIN_BLOCK_SIZE) {
             throw std::invalid_argument("Total size too small for memory manager overhead.");
        }
        memory_pool = static_cast<uint8_t*>(::operator new(pool_size));
        if (!memory_pool) {
            throw std::bad_alloc();
        }

        // Initialize the entire pool as a single free block
        BlockHeader* initial_block = reinterpret_cast<BlockHeader*>(memory_pool);
        initial_block->size = pool_size;
        initial_block->is_free = true;
        initial_block->next_free = nullptr;
        initial_block->prev_free = nullptr;

        BlockFooter* initial_footer = get_footer(initial_block);
        initial_footer->size = initial_block->size;

        // Add the initial block to the free list
        add_to_free_list(initial_block);
    }

    ~MemoryManager() {
        // Optional: Could add checks here to see if all memory was deallocated
        ::operator delete(memory_pool);
    }

    T* allocate(size_t count) {
        if (count == 0) return nullptr;

        const size_t user_data_size = align_up(count * sizeof(T), ALIGNMENT);
        // Total size needed: header + aligned user data + footer
        const size_t required_block_size = HEADER_SIZE + user_data_size + FOOTER_SIZE;

        if (required_block_size < MIN_BLOCK_SIZE) {
             // Ensure even small allocations meet minimum block size if needed
             // This might not be strictly necessary if splitting handles it, but safer.
             // required_block_size = MIN_BLOCK_SIZE;
             // Re-calculate user_data_size based on MIN_BLOCK_SIZE if needed? No, user asked for 'count'.
             // Let's assume MIN_BLOCK_SIZE check in constructor is enough, and splitting handles small remainders.
        }


        std::lock_guard<std::mutex> lock(mutex);

        // --- Find a suitable block (First-Fit) ---
        BlockHeader* current = free_list_head;
        BlockHeader* found_block = nullptr;
        while (current) {
            if (current->size >= required_block_size) {
                found_block = current;
                break;
            }
            current = current->next_free;
        }

        if (!found_block) {
            // Optional: Could try to extend the pool here if designed for it
            throw std::bad_alloc();
        }

        // --- Allocate the block ---
        remove_from_free_list(found_block);

        const size_t original_block_size = found_block->size;
        const size_t remaining_size = original_block_size - required_block_size;

        // Check if we can split the block
        if (remaining_size >= MIN_BLOCK_SIZE) {
            // Split: Create a new free block from the remainder
            found_block->size = required_block_size; // Shrink the allocated block
            get_footer(found_block)->size = required_block_size; // Update its footer

            // Set up the new free block header
            BlockHeader* new_free_block = get_next_physical_block(found_block); // It starts right after the allocated one
            new_free_block->size = remaining_size;
            new_free_block->is_free = true;
            // Footer for the new free block
            get_footer(new_free_block)->size = remaining_size;

            // Add the new smaller block to the free list
            add_to_free_list(new_free_block);

        } else {
            // Cannot split, allocate the entire block (internal fragmentation)
            // No size changes needed for found_block or its footer
        }

        found_block->is_free = false;
        // No separate AllocHeader needed

        // Construct T objects if needed (placement new) - outside scope of basic allocator
        // for (size_t i = 0; i < count; ++i) {
        //     new (static_cast<T*>(get_user_data(found_block)) + i) T();
        // }

        return static_cast<T*>(get_user_data(found_block));
    }

    void deallocate(T* ptr) noexcept {
        if (!ptr) return;

        // Basic check: Is the pointer within the managed pool bounds?
        uint8_t* ptr_byte = reinterpret_cast<uint8_t*>(ptr);
        if (ptr_byte < memory_pool + HEADER_SIZE || ptr_byte >= memory_pool + pool_size) {
             // Pointer is outside the possible user data area of the pool
             // Optional: Log error, assert, or silently return
             // std::cerr << "Warning: Deallocating pointer outside managed pool." << std::endl;
             return;
        }

        BlockHeader* block = get_header_from_user_data(ptr);

        // More robust check: Could involve magic numbers in header/footer if implemented

        std::lock_guard<std::mutex> lock(mutex);

        // Check for double free
        if (block->is_free) {
            // Optional: Log error, assert, or silently return
            // std::cerr << "Warning: Double free detected." << std::endl;
            return;
        }

        // Call destructors if needed (placement delete) - outside scope of basic allocator
        // size_t count = (block->size - HEADER_SIZE - FOOTER_SIZE) / sizeof(T);
        // for (size_t i = 0; i < count; ++i) {
        //     (static_cast<T*>(ptr) + i)->~T();
        // }

        block->is_free = true; // Mark as free *before* coalescing/adding to list

        // Coalesce with neighbors
        BlockHeader* coalesced_block = coalesce(block);

        // Add the (potentially larger) coalesced block back to the free list
        add_to_free_list(coalesced_block);
    }

    // --- Rule of 5/0 ---
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    MemoryManager(MemoryManager&& other) noexcept
        : memory_pool(other.memory_pool),
          pool_size(other.pool_size),
          free_list_head(other.free_list_head),
          mutex() // Mutex is not moved, acquire new one
          {
        // Must carefully re-acquire lock or ensure other is not in use.
        // Simpler: just null out the other's state.
        other.memory_pool = nullptr;
        other.pool_size = 0;
        other.free_list_head = nullptr;
    }

    MemoryManager& operator=(MemoryManager&& other) noexcept {
        if (this != &other) {
            // Lock both mutexes to prevent deadlock (using std::scoped_lock if available, or std::lock)
            std::lock(mutex, other.mutex);
            std::lock_guard<std::mutex> lock_this(mutex, std::adopt_lock);
            std::lock_guard<std::mutex> lock_other(other.mutex, std::adopt_lock);

            // Release current resources
            ::operator delete(memory_pool);

            // Take other's resources
            memory_pool = other.memory_pool;
            pool_size = other.pool_size;
            free_list_head = other.free_list_head;

            // Leave other in a valid state
            other.memory_pool = nullptr;
            other.pool_size = 0;
            other.free_list_head = nullptr;
        }
        return *this;
    }

    // --- Debugging Helper (Optional) ---
    void print_free_list() const {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "Free List:" << std::endl;
        BlockHeader* current = free_list_head;
        int count = 0;
        while (current) {
            std::cout << "  Block at offset " << (reinterpret_cast<uint8_t*>(current) - memory_pool)
                      << ", Size: " << current->size << ", Free: " << current->is_free << std::endl;
            current = current->next_free;
            count++;
        }
         if (count == 0) {
             std::cout << "  (Empty)" << std::endl;
         }
        std::cout << "Total free blocks: " << count << std::endl;
    }

    void print_all_blocks() const {
         std::lock_guard<std::mutex> lock(mutex);
         std::cout << "All Blocks:" << std::endl;
         uint8_t* current_ptr = memory_pool;
         while (current_ptr < memory_pool + pool_size) {
             BlockHeader* header = reinterpret_cast<BlockHeader*>(current_ptr);
             std::cout << "  Block at offset " << (current_ptr - memory_pool)
                       << ", Size: " << header->size << ", Free: " << header->is_free;
             if (header->size == 0) {
                 std::cout << " !!! ZERO SIZE BLOCK - CORRUPTION? !!!" << std::endl;
                 break;
             }
             // Verify footer matches header size
             BlockFooter* footer = get_footer(header);
             if (footer->size != header->size) {
                  std::cout << " !!! FOOTER/HEADER SIZE MISMATCH !!! Footer size: " << footer->size;
             }
             std::cout << std::endl;
             current_ptr += header->size;
         }
          std::cout << "End of Pool at offset " << pool_size << std::endl;
    }

};
