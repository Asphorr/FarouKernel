#include <iostream>
#include <variant>
#include <string>
#include <stdexcept>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>

using namespace std::literals;

class SystemCall {
public:
    virtual ~SystemCall() = default;
    virtual void Execute() = 0;
};

class Exit : public SystemCall {
public:
    void Execute() override {
        std::cout << "Exiting...\n";
        exit(EXIT_SUCCESS);
    }
};

class Read : public SystemCall {
public:
    void Execute() override {
        char buffer[4096];
        ssize_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (bytes_read == -1) {
            throw std::runtime_error("Read failed");
        }
        std::cout << bytes_read << " bytes read:\n";
    }
};

class Write : public SystemCall {
public:
    void Execute() override {
        char buffer[4096];
        ssize_t bytes_written = write(STDOUT_FILENO, buffer, sizeof(buffer));
        if (bytes_written == -1) {
            throw std::runtime_error("Write failed");
        }
        std::cout << bytes_written << " bytes written.\n";
    }
};

class Open : public SystemCall {
public:
    void Execute() override {
        int file_descriptor = open("/dev/null", O_RDONLY);
        if (file_descriptor == -1) {
            throw std::runtime_error("Open failed");
        }
        close(file_descriptor);
    }
};

class Close : public SystemCall {
public:
    void Execute() override {
        if (close(STDIN_FILENO) == -1) {
            throw std::runtime_error("Close failed");
        }
    }
};

class Creat : public SystemCall {
public:
    void Execute() override {
        int file_descriptor = creat("/tmp/test.txt", 0644);
        if (file_descriptor == -1) {
            throw std::runtime_error("Creat failed");
        }
        close(file_descriptor);
    }
};

class Unlink : public SystemCall {
public:
    void Execute() override {
        if (unlink("/tmp/test.txt") == -1) {
            throw std::runtime_error("Unlink failed");
        }
    }
};

class GetPid : public SystemCall {
public:
    void Execute() override {
        pid_t pid = getpid();
        std::cout << "Process ID: " << pid << '\n';
    }
};

class Sleep : public SystemCall {
public:
    void Execute() override {
        sleep(5);
    }
};

using SystemCallVariant = std::variant<Exit, Read, Write, Open, Close, Creat, Unlink, GetPid, Sleep>;

std::istream& operator>>(std::istream& is, std::unique_ptr<SystemCall>& sc) {
    std::string input;
    is >> input;
    if (input == "exit"sv) {
        sc = std::make_unique<Exit>();
    } else if (input == "read"sv) {
        sc = std::make_unique<Read>();
    } else if (input == "write"sv) {
        sc = std::make_unique<Write>();
    } else if (input == "open"sv) {
        sc = std::make_unique<Open>();
    } else if (input == "close"sv) {
        sc = std::make_unique<Close>();
    } else if (input == "creat"sv) {
        sc = std::make_unique<Creat>();
    } else if (input == "unlink"sv) {
        sc = std::make_unique<Unlink>();
    } else if (input == "getpid"sv) {
        sc = std::make_unique<GetPid>();
    } else if (input == "sleep"sv) {
        sc = std::make_unique<Sleep>();
    } else {
        throw std::invalid_argument("Invalid system call");
    }
    return is;
}

int main() {
    while (true) {
        try {
            std::unique_ptr<SystemCall> sc;
            std::cin >> sc;
            sc->Execute();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            break;
        }
    }
    return EXIT_FAILURE;
}
