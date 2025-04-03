#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <new>
#include <iostream> // For debugging and stats
#include <vector>
#include <thread>
#include <chrono>
#include <array>
#include <numeric> // for std::gcd, std::lcm if needed, maybe just bit_width
#include <bit>     // For std::bit_width (C++20)

// Platform-specific includes for memory mapping and advising
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h> // For _BitScanReverse
#include <sysinfoapi.h> // For GlobalMemoryStatusEx
#else
#include <sys/mman.h>
#include <unistd.h> // For sysconf, sbrk (though mmap is better)
#include <string.h> // For memset
#include <sys/sysinfo.h> // For sysinfo
#endif

// --- Configuration ---
#define DEBUG_MEMORY_MANAGER // Define to enable safety checks (canaries)

// Helper function for alignment (remains useful)
inline size_t align_up(size_t size, size_t align) noexcept {
    return (size + align - 1) & ~(align - 1);
}

// Forward declaration
template <typename T, size_t Alignment> class MemoryManager;

// --- Core Data Structures ---

// Represents a block of memory, managed by the allocator
template <size_t Alignment>
struct alignas(Alignment) BlockHeader {
    // --- Frequently Accessed ---
    size_t size;        // Size of the *entire* block (header + user_data + potential padding + footer)
    bool is_free;       // Status of the block
    std::thread::id owner_thread_id; // ID of the thread that allocated this block (for TLC dealloc)
#ifdef DEBUG_MEMORY_MANAGER
    uint32_t canary_begin; // Canary value for overrun detection
#endif
    // Add cache-line padding if needed to avoid false sharing between hot fields and pointers
    // Assuming a 64-byte cache line. Adjust size calculation if fields change.
    static constexpr size_t hot_field_size = sizeof(size_t) + sizeof(bool) + sizeof(std::thread::id)
#ifdef DEBUG_MEMORY_MANAGER
                                             + sizeof(uint32_t)
#endif
        ;
    static constexpr size_t padding_size = (hot_field_size > 64) ? 0 : (64 - hot_field_size);
    char padding[padding_size];

    // --- Less Frequently Accessed (Pointers for free list) ---
    BlockHeader* next_free; // Pointer to next block in the *specific* free list (size class, thread local or global)
    BlockHeader* prev_free; // Pointer to previous block in the free list

};

// Optional Footer for coalescing and overrun detection
template <size_t Alignment>
struct alignas(Alignment) BlockFooter {
    size_t size; // Size of the *entire* block (header + user_data + footer)
#ifdef DEBUG_MEMORY_MANAGER
    uint32_t canary_end; // Canary value for overrun detection
#endif
};


// --- Statistics ---
struct MemoryManagerStats {
    std::atomic<size_t> total_allocated_bytes{0}; // Actual bytes used by blocks currently allocated (incl overhead)
    std::atomic<size_t> total_requested_bytes{0}; // User requested bytes (before alignment/overhead)
    std::atomic<size_t> allocation_count{0};
    std::atomic<size_t> deallocation_count{0};
    std::atomic<size_t> failed_allocations{0};
    std::atomic<size_t> os_memory_requested{0}; // Bytes requested from OS via mmap/VirtualAlloc
    std::atomic<size_t> os_memory_returned{0}; // Bytes returned to OS via madvise/VirtualFree
    std::atomic<size_t> global_list_alloc_attempts{0}; // Allocations falling back to global list
    std::atomic<size_t> cross_thread_deallocs{0}; // Deallocations handled via lock-free path
    std::atomic<size_t> mutex_contention_count{0}; // Estimate contention (e.g., try_lock fails) - basic version

    // Per size class statistics could be added here if needed
    // std::array<SizeClassStats, SIZE_CLASS_COUNT> size_class_stats;
};


// --- Memory Manager Class ---

template <typename T = std::byte, size_t Alignment = alignof(std::max_align_t)>
class MemoryManager {
public:
    // Make sure alignment is a power of 2 and sufficient
    static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be a power of 2");
    static_assert(Alignment >= alignof(void*), "Alignment must be at least alignof(void*)"); // Min practical alignment

    // Type aliases for allocator traits compatibility
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;

    // C++17 polymorphic allocator support
    template <typename U>
    struct rebind {
        using other = MemoryManager<U, Alignment>;
    };

private:
    // Use BlockHeader/Footer templated on Alignment
    using Header = BlockHeader<Alignment>;
    using Footer = BlockFooter<Alignment>;

    static constexpr size_t HEADER_SIZE = align_up(sizeof(Header), Alignment);
    static constexpr size_t FOOTER_SIZE = align_up(sizeof(Footer), Alignment);
    static constexpr size_t MIN_BLOCK_OVERHEAD = HEADER_SIZE + FOOTER_SIZE;
    // Minimum user data size to avoid fragmentation issues? Let's require at least pointer size.
    static constexpr size_t MIN_USER_DATA_SIZE = align_up(sizeof(void*), Alignment);
    // Minimum total block size needed to store header, footer, and minimal data/free pointers
    static constexpr size_t MIN_BLOCK_SIZE = align_up(MIN_BLOCK_OVERHEAD + MIN_USER_DATA_SIZE, Alignment);

#ifdef DEBUG_MEMORY_MANAGER
    static constexpr uint32_t CANARY_VALUE = 0xDEADBEEF;
#endif

    // --- Size Classes ---
    // Example: Powers of 2 starting from 16 or MIN_BLOCK_SIZE up to a certain limit
    // Needs careful tuning. Let's use powers of 2 from MIN_BLOCK_SIZE.
    static constexpr size_t MIN_SIZE_CLASS_USER_SIZE = MIN_USER_DATA_SIZE; // Smallest allocatable user size
    static constexpr size_t MIN_SIZE_CLASS_BLOCK_SIZE = MIN_BLOCK_SIZE; // Smallest block size
    static constexpr int SIZE_CLASS_SHIFT = 4; // Example: Base 2 logarithm of 16 (if MIN_SIZE_CLASS was 16) - adjust based on MIN_BLOCK_SIZE
    static constexpr size_t MAX_SIZE_CLASS_INDEX = 31; // Corresponds to 2^(31+SHIFT) bytes, ~2GB or ~8GB etc.
    static constexpr size_t SIZE_CLASS_COUNT = MAX_SIZE_CLASS_INDEX + 1;

    // Get size class index for a *required total block size*
    static size_t getSizeClassIndex(size_t total_block_size) noexcept {
        if (total_block_size <= MIN_SIZE_CLASS_BLOCK_SIZE) {
            return 0; // Smallest class
        }
        // Find index based on next power of 2 or similar scheme
        // Using C++20 std::bit_width for simplicity (log2 ceiling)
        // size_t adjusted_size = total_block_size - 1;
        // size_t log2_ceil = std::bit_width(adjusted_size); // C++20

        // Manual MSB calculation (portable)
        size_t adjusted_size = total_block_size - 1;
        size_t log2_ceil;
#ifdef _WIN32
        unsigned long msb_index;
        if (_BitScanReverse64(&msb_index, adjusted_size)) { // Use 64 for size_t
             log2_ceil = msb_index + 1;
        } else {
             log2_ceil = 0; // Handle size=1 case leading to adjusted_size=0
        }
#else // Assume GCC/Clang compatible
        if (adjusted_size == 0) {
            log2_ceil = 0;
        } else {
            // __builtin_clzl returns number of leading zeros for unsigned long (or long long)
            // Use unsigned long long for portability with 64-bit size_t
            log2_ceil = (sizeof(unsigned long long) * 8) - __builtin_clzll(adjusted_size);
        }
#endif
        if (log2_ceil < SIZE_CLASS_SHIFT) return 0; // Clamp to min class
        size_t index = log2_ceil - SIZE_CLASS_SHIFT;
        return std::min(index, MAX_SIZE_CLASS_INDEX); // Clamp to max class
    }

    // Round up total block size to the size represented by the class index
    static size_t roundToSizeClass(size_t total_block_size) noexcept {
        size_t index = getSizeClassIndex(total_block_size);
        if (index == 0 && total_block_size <= MIN_SIZE_CLASS_BLOCK_SIZE) {
             return MIN_SIZE_CLASS_BLOCK_SIZE;
        }
        // Calculate the size for this class (e.g., 2^(index + SHIFT))
        return static_cast<size_t>(1) << (index + SIZE_CLASS_SHIFT);
    }


    // --- Thread Local Cache (TLC) ---
    struct ThreadCache {
        Header* free_lists[SIZE_CLASS_COUNT] = {nullptr};
        size_t allocation_count = 0; // Thread-local stats if needed
        size_t deallocation_count = 0;
        // Could add limits to cache size per class to return memory to global pool
    };

    static thread_local ThreadCache thread_cache;

    // --- Global State ---
    // Use atomic<Header*> for the heads of the global free lists for lock-free dealloc push
    std::atomic<Header*> global_free_heads[SIZE_CLASS_COUNT];
    std::mutex global_mutexes[SIZE_CLASS_COUNT]; // Finer-grained locking for global list *allocations*
    void* memory_pool_start = nullptr; // Base address from OS
    size_t total_pool_size = 0;       // Total size obtained from OS
    std::mutex pool_management_mutex; // Protects expanding/managing the pool itself (rarely needed)

    MemoryManagerStats stats; // Global statistics object

    // OS Page size - cache it
    static size_t getPageSize() {
        static const size_t page_size = []() {
#ifdef _WIN32
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            return sysInfo.dwPageSize;
#else
            return sysconf(_SC_PAGESIZE);
#endif
        }();
        return page_size;
    }

    // --- Helper functions for block manipulation (adapted) ---

    Footer* get_footer(Header* header) const noexcept {
        return reinterpret_cast<Footer*>(reinterpret_cast<uint8_t*>(header) + header->size - FOOTER_SIZE);
    }

    // Need careful boundary checks against memory_pool_start and total_pool_size
    Header* get_next_physical_block(Header* header) const noexcept {
        uint8_t* next_block_ptr = reinterpret_cast<uint8_t*>(header) + header->size;
        if (next_block_ptr >= reinterpret_cast<uint8_t*>(memory_pool_start) + total_pool_size) {
            return nullptr; // Reached end of pool
        }
        // TODO: Add check if next_block_ptr crosses a span boundary if multiple spans are managed
        return reinterpret_cast<Header*>(next_block_ptr);
    }

    Header* get_prev_physical_block(Header* header) const noexcept {
        if (reinterpret_cast<uint8_t*>(header) == reinterpret_cast<uint8_t*>(memory_pool_start)) {
            return nullptr; // First block in pool
        }
        // Use the footer of the *previous* block to find its header (more robust than using current's footer)
        Footer* prev_footer = reinterpret_cast<Footer*>(reinterpret_cast<uint8_t*>(header) - FOOTER_SIZE);
        // Now calculate the start of the previous block's header
        return reinterpret_cast<Header*>(reinterpret_cast<uint8_t*>(header) - prev_footer->size);
    }

    void* get_user_data(Header* header) const noexcept {
        return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(header) + HEADER_SIZE);
    }

    Header* get_header_from_user_data(void* ptr) const noexcept {
        return reinterpret_cast<Header*>(reinterpret_cast<uint8_t*>(ptr) - HEADER_SIZE);
    }

    // --- Free List Management (Generic for TLC or Global) ---

    // Remove a block from a doubly-linked free list (given list head)
    void remove_from_list(Header* block, Header*& list_head) noexcept {
        if (block->prev_free) {
            block->prev_free->next_free = block->next_free;
        } else {
            list_head = block->next_free; // Update list head
        }
        if (block->next_free) {
            block->next_free->prev_free = block->prev_free;
        }
        block->next_free = nullptr;
        block->prev_free = nullptr;
    }

    // Add a block to the front of a doubly-linked free list
    void add_to_list(Header* block, Header*& list_head) noexcept {
        block->next_free = list_head;
        block->prev_free = nullptr;
        if (list_head) {
            list_head->prev_free = block;
        }
        list_head = block;
        block->is_free = true; // Ensure marked free
    }

     // Add a block to the front of a singly-linked list (atomic push for global dealloc)
    void add_to_list_atomic(Header* block, std::atomic<Header*>& list_head) noexcept {
        block->is_free = true; // Mark free first
        Header* expected = list_head.load(std::memory_order_relaxed);
        do {
            block->next_free = expected; // prev_free is not used in this atomic list
            // Attempt to atomically set the list head to the new block
        } while (!list_head.compare_exchange_weak(
            expected, block,
            std::memory_order_release, // Ensure writes (like setting is_free) are visible before push
            std::memory_order_relaxed // Okay if CAS fails spuriously
        ));
    }


    // --- Coalescing (remains similar, but needs list removal logic separated) ---
    Header* coalesce(Header* block) noexcept {
        // Coalescing assumes the block is *already marked free* but *not yet* in any list.
        // It finds neighbours, removes them from *their* lists if they are free,
        // merges sizes, and returns the final combined block header.
        // The caller is responsible for adding the final block to the appropriate list.

        Header* final_block = block;
        size_t final_size = block->size;

        // Check previous block
        Header* prev_block = get_prev_physical_block(block);
        if (prev_block && prev_block->is_free) {
            // Which list is prev_block in? Global or another thread's TLC?
            // This is complex. A simpler approach for coalescing might be:
            // Only coalesce if neighbours are in the *global* free list.
            // This avoids needing locks for other threads' TLCs.
            // Or, defer coalescing until blocks are returned to the global list.
            // Let's stick to immediate coalescing but remove from the GLOBAL list only for now.
            size_t prev_index = getSizeClassIndex(prev_block->size);
            { // Lock the specific global list for the previous block's size class
                std::lock_guard<std::mutex> lock(global_mutexes[prev_index]);
                // Need to traverse the global list to find and remove prev_block
                // This is inefficient! Alternative: Deferred coalescing or require blocks
                // being coalesced to be removed from lists *before* calling coalesce.

                // Let's assume a simpler model for now: Coalesce only happens when returning to global pool,
                // or modify deallocate to handle list removal carefully.
                // *** Revisiting Deallocate Logic Later ***
                // For now, let's assume neighbours are magically removed from lists if free...
                // This part needs a robust implementation strategy.

                // --- Simplified Coalescing Logic (assuming neighbours are handled elsewhere) ---
                 final_size += prev_block->size;
                 final_block = prev_block; // Merged block starts at prev_block
            } // Release lock
        }

        // Check next block
        Header* next_block = get_next_physical_block(final_block); // Use final_block in case we merged prev
         if (next_block && next_block->is_free) {
             size_t next_index = getSizeClassIndex(next_block->size);
             { // Lock the specific global list for the next block's size class
                  std::lock_guard<std::mutex> lock(global_mutexes[next_index]);
                 // Remove next_block from its list (again, simplifying here)
                 final_size += next_block->size;
             } // Release lock
         }

        // Update the header and footer of the final combined block
        if (final_block != block || final_size != block->size) {
            final_block->size = final_size;
            get_footer(final_block)->size = final_size;
        }

        return final_block; // Return the start of the potentially merged block
    }


    // --- Allocate more memory from OS ---
    Header* allocateNewSpan(size_t min_size_needed) noexcept {
        std::lock_guard<std::mutex> lock(pool_management_mutex); // Protect pool extension

        // How much to request? At least min_size, but typically more (e.g., 1MB chunks)
        size_t page_size = getPageSize();
        size_t size_to_request = align_up(std::max(min_size_needed, (size_t)1 * 1024 * 1024), page_size); // e.g., 1MB aligned

        void* new_mem = nullptr;
#ifdef _WIN32
        new_mem = VirtualAlloc(nullptr, size_to_request, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!new_mem) {
             stats.failed_allocations++;
             return nullptr;
        }
#else
        new_mem = mmap(nullptr, size_to_request, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (new_mem == MAP_FAILED) {
            stats.failed_allocations++;
            return nullptr;
        }
#endif
        stats.os_memory_requested += size_to_request;

        // TODO: Handle multiple discontiguous spans. This simple version assumes one pool.
        // For this example, let's *replace* the pool if it's null, otherwise fail (simplistic).
        if (memory_pool_start != nullptr) {
            // Cannot extend easily in this simple model. Production allocators manage lists of spans.
             std::cerr << "ERROR: Memory pool extension not fully implemented." << std::endl;
#ifdef _WIN32
            VirtualFree(new_mem, 0, MEM_RELEASE);
#else
            munmap(new_mem, size_to_request);
#endif
            stats.os_memory_requested -= size_to_request; // Rollback stat
            stats.failed_allocations++;
            return nullptr;
        }

        memory_pool_start = new_mem;
        total_pool_size = size_to_request;

        // Initialize the new span as a single free block
        Header* new_block = reinterpret_cast<Header*>(new_mem);
        new_block->size = size_to_request;
        new_block->is_free = true;
        new_block->next_free = nullptr;
        new_block->prev_free = nullptr;
        // Initialize footer
        get_footer(new_block)->size = new_block->size;

        // Add this large block to the appropriate global free list
        size_t index = getSizeClassIndex(new_block->size);
        std::lock_guard<std::mutex> list_lock(global_mutexes[index]); // Lock the target list
        add_to_list(new_block, global_free_heads[index]. Msvc cannot implicitly convert std::atomic to Header*& // store(new_block, std::memory_order_relaxed) WRONG LIST TYPE //); // Add to global list
        // Correction: global_free_heads is atomic<Header*> for lock-free push, not a list head ref.
        // Need a separate array for actual list heads protected by the mutexes.

        // *** REVISED Global State ***
        // Header* global_free_lists[SIZE_CLASS_COUNT] = {nullptr}; // Protected by mutexes
        // std::atomic<Header*> global_free_heads_atomic[SIZE_CLASS_COUNT]; // For lock-free push target
        // Let's simplify: Use ONE mechanism. If we use lock-free push for dealloc,
        // allocation from global needs to handle the singly-linked atomic list.

        // *** Let's use the ATOMIC list for global ***
        // Allocation from global list needs to pop atomically (more complex CAS loop)
        // Deallocation (cross-thread) pushes atomically (as implemented in add_to_list_atomic)

        add_to_list_atomic(new_block, global_free_heads[index]); // Add to the atomic global list

        return new_block; // Return the header of the new span block
    }

    // --- Try to allocate from the Global Pool (requires locking or atomic pop) ---
    Header* allocateFromGlobalPool(size_t size_class_index, size_t required_block_size) {
        stats.global_list_alloc_attempts++;

        // --- Atomic Pop from Singly-Linked List ---
        Header* block = global_free_heads[size_class_index].lo
