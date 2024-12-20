#include <cstddef>
#include <forward_list>
#include <memory>
#include <utility>

template <typename T>
struct ListNode {
    T element;
    std::unique_ptr<ListNode<T>> next;
};

template <typename T>
class LinkedList {
 public:
    LinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    // Move constructor
    LinkedList(LinkedList&& other) noexcept : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    // Copy assignment operator
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            for (auto iter = other.begin(); iter != other.end(); ++iter) {
                emplace_back(*iter);
            }
        }
        return *this;
    }

    // Move assignment operator
    LinkedList& operator=(LinkedList&& other) noexcept {
        swap(other);
        return *this;
    }

    // Destructor
    ~LinkedList() { clear(); }

    // Element access
    T& front() { return head_->element; }
    const T& front() const { return head_->element; }

    T& back() { return tail_->element; }
    const T& back() const { return tail_->element; }

    // Iterators
    template <typename U>
    friend class iterator;

    typedef iterator<T> iterator;
    typedef iterator<const T> const_iterator;

    iterator begin() { return iterator(head_); }
    const_iterator begin() const { return const_iterator(head_); }

    iterator end() { return iterator(nullptr); }
    const_iterator end() const { return const_iterator(nullptr); }

    // Capacity
    size_type size() const { return size_; }
    bool empty() const { return !size_; }

    // Modifiers
    void push_front(const T& x) { insert(x, head_); }
    void push_front(T&& x) { insert(std::move(x), head_); }

    void push_back(const T& x) { insert(x, nullptr); }
    void push_back(T&& x) { insert(std::move(x), nullptr); }

    void pop_front() { erase(head_); }
    void pop_back() { erase(tail_); }

    void clear() {
        while (!empty()) {
            pop_front();
        }
    }

 private:
    // Helper functions
    void insert(const T& x, ListNode<T>* position) {
        auto newNode = std::make_unique<ListNode<T>>(x);
        if (position == nullptr) {
            head_ = newNode.get();
            tail_ = newNode.get();
        } else {
            newNode->next = position->next;
            position->next = newNode.get();
            if (newNode->next == nullptr) {
                tail_ = newNode.get();
            }
        }
        size_++;
    }

    void insert(T&& x, ListNode<T>* position) {
        auto newNode = std::make_unique<ListNode<T>>(std::move(x));
        if (position == nullptr) {
            head_ = newNode.get();
            tail_ = newNode.get();
        } else {
            newNode->next = position->next;
            position->next = newNode.get();
            if (newNode->next == nullptr) {
                tail_ = newNode.get();
            }
        }
        size_++;
    }

    void erase(ListNode<T>* position) {
        if (position == nullptr || empty()) {
            return;
        }
        if (position == head_) {
            head_ = position->next;
        }
        if (position == tail_) {
            tail_ = position->prev;
        }
        if (position->prev != nullptr) {
            position->prev->next = position->next;
        }
        if (position->next != nullptr) {
            position->next->prev = position->prev;
        }
        size_--;
    }

    void swap(LinkedList& other) {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
        std::swap(size_, other.size_);
    }

    ListNode<T>* head_;
    ListNode<T>* tail_;
    size_t size_;
};
