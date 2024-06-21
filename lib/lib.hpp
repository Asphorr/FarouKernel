#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>

namespace lib {
    template <typename T>
    class Array {
        std::unique_ptr<T[]> data;
        size_t length;
    
    public:
        explicit Array(size_t n) : data(std::make_unique<T[]>(n)), length(n) {}
        
        ~Array() = default;
        
        void free() {
            data.reset();
            length = 0;
        }
        
        constexpr auto begin() noexcept { return &data[0]; }
        constexpr auto end() noexcept { return &data[length]; }
        
        [[nodiscard]] constexpr bool empty() const noexcept { return !length; }
        [[nodiscard]] constexpr size_t size() const noexcept { return length; }
        
        constexpr T& operator[](size_t index) {
            assert(index < length && "Index out of bounds");
            return data[index];
        }
        
        constexpr const T& operator[](size_t index) const {
            assert(index < length && "Index out of bounds");
            return data[index];
        }
    };
    
    template <typename T>
    class LinkedList {
        struct Node {
            T value;
            std::shared_ptr<Node> next;
            
            Node(T val, std::shared_ptr<Node> nxt) : value(val), next(nxt) {}
        };
        
        std::shared_ptr<Node> head;
        size_t length;
    
    public:
        LinkedList() : head(), length(0) {}
        
        ~LinkedList() = default;
        
        void push_back(T value) {
            ++length;
            head = std::make_shared<Node>(value, head);
        }
        
        void pop_front() {
            --length;
            head = head->next;
        }
        
        [[nodiscard]] constexpr bool empty() const noexcept { return !head; }
        [[nodiscard]] constexpr size_t size() const noexcept { return length; }
        
        constexpr T front() const {
            assert(!empty());
            return head->value;
        }
        
        constexpr T back() const {
            assert(!empty());
            return head->next ? head->next->value : head->value;
        }
    };
    
    template <typename K, typename V>
    class HashMap {
        std::unordered_map<K, V> map;
    
    public:
        HashMap() : map() {}
        
        ~HashMap() = default;
        
        void insert(K key, V value) {
            map.insert({key, value});
        }
        
        void erase(K key) {
            map.erase(key);
        }
        
        [[nodiscard]] constexpr bool contains(K key) const noexcept {
            return map.find(key) != map.end();
        }
        
        [[nodiscard]] constexpr V get(K key) const noexcept {
            return map.at(key);
        }
        
        [[nodiscard]] constexpr size_t size() const noexcept {
            return map.size();
        }
    };
}
