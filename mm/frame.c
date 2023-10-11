#include <iostream>
#include <list>
#include <memory>

class FrameList {
public:
    FrameList() : _head(), _tail() {}
    
    ~FrameList() { clear(); }
    
    bool empty() const { return _head == nullptr && _tail == nullptr; }
    
    size_t size() const { return std::distance(_head, _tail); }
    
    void push_front(const Frame& frame) {
        auto node = std::make_unique<Node>(frame);
        if (_head == nullptr) {
            _head = node.get();
            _tail = node.get();
        } else {
            node->setNext(_head);
            _head->setPrev(node.get());
            _head = node.get();
        }
        
        _size++;
    }
    
    void pop_back() {
        if (!empty()) {
            Node* last = _tail;
            _tail = last->getPrev();
            
            if (_tail == nullptr) {
                _head = nullptr;
            } else {
                _tail->setNext(nullptr);
            }
            
            delete last;
            _size--;
        }
    }
    
    void clear() {
        while (!empty()) {
            pop_back();
        }
    }
private:
    struct Node {
        explicit Node(const Frame& frame) : _frame(frame), _next(nullptr), _prev(nullptr) {}
        
        Frame _frame;
        Node* _next;
        Node* _prev;
    };
    
    Node* _head;
    Node* _tail;
    size_t _size;
};
