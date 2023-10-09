#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>
#include <concepts>

// Define a concept for system calls
template <typename T>
concept bool IsSystemCall = requires(T t) {
    { t.number } -> std::same_as<int>;
    { t.function } -> std::is_pointer_v<decltype(t)>;
};

// Define an interface for system call implementations
struct SystemCall {
    virtual ~SystemCall() = default;
    virtual int operator()(IsSystemCall auto sc) = 0;
};

// Implementations of specific system calls
struct Exit : public SystemCall {
    explicit Exit(int status) : _status{status} {}
    int operator()(IsSystemCall auto sc) override { return _status; }
 private:
    int _status;
};

struct Read : public SystemCall {
    explicit Read(ssize_t count) : _count{count} {}
    int operator()(IsSystemCall auto sc) override { return static_cast<int>(_count); }
 private:
    ssize_t _count;
};

struct Write : public SystemCall {
    explicit Write(const void *buf, size_t count) : _buf{buf}, _count{count} {}
    int operator()(IsSystemCall auto sc) override { return static_cast<int>(_count); }
 private:
    const void *_buf;
    size_t _count;
};

struct Open : public SystemCall {
    explicit Open(const char *pathname, int flags) : _pathname{pathname}, _flags{flags} {}
    int operator()(IsSystemCall auto sc) override { return static_cast<int>(_flags); }
 private:
    const char *_pathname;
    int _flags;
};

struct Close : public SystemCall {
    explicit Close(int fd) : _fd{fd} {}
    int operator()(IsSystemCall auto sc) override { return static_cast<int>(_fd); }
 private:
    int _fd;
};

struct Creat : public SystemCall {
    explicit Creat(const char *pathname, int mode) : _pathname{pathname}, _mode{mode} {}
    int operator()(IsSystemCall auto sc) override { return static_cast<int>(_mode); }
 private:
    const char *_pathname;
    int _mode;
};

struct Unlink : public SystemCall {
    explicit Unlink(const char *pathname) : _pathname{pathname} {}
    int operator()(IsSystemCall auto sc) override { return static_cast<int>(_pathname); }
 private:
    const char *_pathname;
};

struct GetPid : public SystemCall {
    explicit GetPid() {}
    int operator()(IsSystemCall auto sc) override { return static_cast<int>(getpid()); }
};

struct Sleep : public SystemCall {
    explicit Sleep(unsigned int seconds) : _seconds{seconds} {}
    int operator()(IsSystemCall auto sc) override { return static_cast<int>(sleep(_seconds)); }
 private:
    unsigned int _seconds;
};

// A factory function to create instances of system call objects based on their names
std::unique_ptr<SystemCall> CreateSystemCall(const std::string &name) {
    // TODO: Add support for other system calls as needed
    if (name == "exit") {
        return std::make_unique<Exit>();
    } else if (name == "read") {
        return std::make_unique<Read>();
    } else if (name == "write") {
        return std::make_unique<Write>();
    } else if (name == "open") {
        return std::make_unique<Open>();
    } else if (name == "close") {
        return std::make_unique<Close>();
    } else if (name == "creat") {
        return std::make_unique<Creat>();
    } else if (name == "unlink") {
        return std::make_unique<Unlink>();
    } else if (name == "getpid") {
        return std::make_unique<GetPid>();
    } else if (name == "sleep") {
        return std::make_unique<Sleep>();
    } else {
        throw std::runtime
error("Unsupported system call");
}
}
