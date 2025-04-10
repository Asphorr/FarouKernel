#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <system_error>

namespace devfs {

/**
 * @namespace devfs
 * @brief Device Filesystem implementation with advanced IO capabilities
 */

// ============================================================================
//                              Error Handling
// ============================================================================

/**
 * @enum ErrorCode
 * @brief Filesystem operation error codes (POSIX-compatible)
 */
enum class [[nodiscard]] ErrorCode {
    SUCCESS             = 0,
    EXISTS              = EEXIST,
    NOT_FOUND           = ENOENT,
    INVALID_ARG         = EINVAL,
    ACCESS_DENIED       = EACCES,
    IO_ERROR            = EIO,
    NOT_DIRECTORY       = ENOTDIR,
    NOT_FILE            = EISDIR,
    UNSUPPORTED_OP      = ENOSYS,
    BUSY                = EBUSY
};

/**
 * @class FilesystemError
 * @brief Exception wrapper for filesystem errors with POSIX error codes
 */
class FilesystemError : public std::system_error {
public:
    explicit FilesystemError(ErrorCode code)
        : std::system_error(static_cast<int>(code), std::generic_category()) {}
    
    explicit FilesystemError(ErrorCode code, const std::string& what)
        : std::system_error(static_cast<int>(code), std::generic_category(), what) {}
};

// ============================================================================
//                              Core Types
// ============================================================================

/**
 * @class Node
 * @brief Base class for all filesystem entries
 */
class Node : public std::enable_shared_from_this<Node> {
public:
    using Ptr = std::shared_ptr<Node>;
    using WeakPtr = std::weak_ptr<Node>;
    
    Node(std::string name, uint8_t major, uint8_t minor)
        : m_name(std::move(name)), 
          m_major(validate_major(major)),
          m_minor(validate_minor(minor)) {}
    
    virtual ~Node() = default;

    // Non-copyable, movable
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(Node&&) noexcept = default;
    Node& operator=(Node&&) noexcept = default;

    const std::string& name() const noexcept { return m_name; }
    uint8_t major() const noexcept { return m_major; }
    uint8_t minor() const noexcept { return m_minor; }
    virtual bool is_directory() const noexcept = 0;

    virtual std::string path() const {
        std::lock_guard lock(m_mutex);
        if (auto parent = m_parent.lock()) {
            return parent->path() + "/" + m_name;
        }
        return "/" + m_name;
    }

    void set_parent(const WeakPtr& parent) {
        std::lock_guard lock(m_mutex);
        m_parent = parent;
    }

protected:
    static uint8_t validate_major(uint8_t major) {
        if (major == 0 || major > 255) {
            throw FilesystemError(ErrorCode::INVALID_ARG, "Invalid major number");
        }
        return major;
    }

    static uint8_t validate_minor(uint8_t minor) {
        if (minor > 255) {
            throw FilesystemError(ErrorCode::INVALID_ARG, "Invalid minor number");
        }
        return minor;
    }

private:
    std::string m_name;
    uint8_t m_major;
    uint8_t m_minor;
    WeakPtr m_parent;
    mutable std::recursive_mutex m_mutex;
};

// ============================================================================
//                              File Interface
// ============================================================================

/**
 * @class File
 * @brief Abstract base class for file-like entities
 */
class File : public Node {
public:
    using Ptr = std::shared_ptr<File>;
    
    File(std::string name, uint8_t major, uint8_t minor)
        : Node(std::move(name), major, minor) {}
    
    bool is_directory() const noexcept final { return false; }

    // File operations interface
    virtual int open(int flags) = 0;
    virtual int close() = 0;
    virtual ssize_t read(char* buf, size_t count, off_t offset) = 0;
    virtual ssize_t write(const char* buf, size_t count, off_t offset) = 0;
    virtual int ioctl(unsigned int request, unsigned long argument) = 0;
    virtual void* mmap(void* addr, size_t length, int prot, int flags, off_t offset) = 0;
    virtual int munmap(void* addr, size_t length) = 0;
};

// ============================================================================
//                              Directory Implementation
// ============================================================================

/**
 * @class Directory
 * @brief Directory node implementation with thread-safe children management
 */
class Directory final : public Node {
public:
    using Ptr = std::shared_ptr<Directory>;
    
    Directory(std::string name, uint8_t major, uint8_t minor)
        : Node(std::move(name), major, minor) {}
    
    bool is_directory() const noexcept final { return true; }

    ErrorCode add_child(const Ptr& child) {
        std::lock_guard lock(m_mutex);
        
        if (!child) return ErrorCode::INVALID_ARG;
        if (m_children.contains(child->name())) 
            return ErrorCode::EXISTS;
        
        child->set_parent(weak_from_this());
        m_children.emplace(child->name(), child);
        return ErrorCode::SUCCESS;
    }

    ErrorCode remove_child(const std::string& name) {
        std::lock_guard lock(m_mutex);
        
        auto it = m_children.find(name);
        if (it == m_children.end())
            return ErrorCode::NOT_FOUND;
        
        it->second->set_parent({});
        m_children.erase(it);
        return ErrorCode::SUCCESS;
    }

    template<typename T = Node>
    std::shared_ptr<T> find_child(const std::string& name) const {
        std::lock_guard lock(m_mutex);
        
        auto it = m_children.find(name);
        if (it == m_children.end())
            return nullptr;
        
        return std::dynamic_pointer_cast<T>(it->second);
    }

    auto begin() const { return m_children.begin(); }
    auto end() const { return m_children.end(); }

private:
    mutable std::recursive_mutex m_mutex;
    std::unordered_map<std::string, Node::Ptr> m_children;
};

// ============================================================================
//                              Event System
// ============================================================================

/**
 * @class EventHandler
 * @brief Generic filesystem event handler with priority support
 */
class EventHandler {
public:
    enum class Priority { HIGH, NORMAL, LOW };

    explicit EventHandler(Priority priority = Priority::NORMAL)
        : m_priority(priority) {}
    
    virtual ~EventHandler() = default;
    
    virtual void handle(uint32_t event_mask, const Node& node) = 0;
    
    bool operator<(const EventHandler& other) const noexcept {
        return m_priority < other.m_priority;
    }

private:
    Priority m_priority;
};

/**
 * @class FunctionalEventHandler
 * @brief Type-erased event handler wrapper for functional objects
 */
template<typename Callable>
class FunctionalEventHandler final : public EventHandler {
public:
    FunctionalEventHandler(Callable&& func, Priority priority = Priority::NORMAL)
        : EventHandler(priority), m_func(std::move(func)) {}
    
    void handle(uint32_t event_mask, const Node& node) override {
        m_func(event_mask, node);
    }

private:
    Callable m_func;
};

template<typename Callable>
auto make_handler(Callable&& func, EventHandler::Priority priority = EventHandler::Priority::NORMAL) {
    return std::make_unique<FunctionalHandler<std::decay_t<Callable>>>(
        std::forward<Callable>(func), priority);
}

// ============================================================================
//                              Filesystem Manager
// ============================================================================

/**
 * @class Manager
 * @brief Thread-safe singleton filesystem manager
 */
class Manager final {
public:
    static Manager& instance() noexcept {
        static Manager instance;
        return instance;
    }

    ErrorCode mount(const std::string& path, Directory::Ptr root) {
        std::lock_guard lock(m_mutex);
        
        if (m_mounts.contains(path))
            return ErrorCode::EXISTS;
        
        m_mounts.emplace(path, std::move(root));
        return ErrorCode::SUCCESS;
    }

    ErrorCode unmount(const std::string& path) {
        std::lock_guard lock(m_mutex);
        
        return m_mounts.erase(path) ? ErrorCode::SUCCESS : ErrorCode::NOT_FOUND;
    }

    ErrorCode register_device(Directory::Ptr parent, Node::Ptr device) {
        if (!parent || !device)
            return ErrorCode::INVALID_ARG;
        
        return parent->add_child(std::move(device));
    }

    template<typename T, typename... Args>
    std::shared_ptr<T> create_device(Args&&... args) {
        auto device = std::make_shared<T>(std::forward<Args>(args)...);
        std::lock_guard lock(m_mutex);
        m_devices.push_back(device);
        return device;
    }

    void add_event_handler(std::unique_ptr<EventHandler> handler) {
        std::lock_guard lock(m_mutex);
        m_handlers.insert(std::upper_bound(m_handlers.begin(), m_handlers.end(), *handler),
                          std::move(handler));
    }

private:
    Manager() = default;
    ~Manager() = default;

    std::mutex m_mutex;
    std::unordered_map<std::string, Directory::Ptr> m_mounts;
    std::vector<Node::Ptr> m_devices;
    std::vector<std::unique_ptr<EventHandler>> m_handlers;
};

} // namespace devfs
