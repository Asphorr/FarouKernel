#include <cstdio>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

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
        size_t bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        printf("%zu bytes read:\n", bytes_read);
        fflush(stdout);
    }
};

class Write : public SystemCall {
public:
    void Execute() override {
        char buffer[4096];
        ssize_t bytes_written = write(STDOUT_FILENO, buffer, sizeof(buffer));
        printf("%zd bytes written.\n", bytes_written);
        fflush(stdout);
    }
};

class Open : public SystemCall {
public:
    void Execute() override {
        int file_descriptor = open("/dev/null", O_RDONLY);
        if (file_descriptor != -1) {
            close(file_descriptor);
        }
    }
};

class Close : public SystemCall {
public:
    void Execute() override {
        close(STDIN_FILENO);
    }
};

class Creat : public SystemCall {
public:
    void Execute() override {
        creat("/tmp/test.txt", 0644);
    }
};

class Unlink : public SystemCall {
public:
    void Execute() override {
        unlink("/tmp/test.txt");
    }
};

class GetPid : public SystemCall {
public:
    void Execute() override {
        pid_t pid = getpid();
        printf("Process ID: %d\n", pid);
        fflush(stdout);
    }
};

class Sleep : public SystemCall {
public:
    void Execute() override {
        sleep(5);
    }
};

using SystemCallVariant = std::variant<Exit, Read, Write, Open, Close, Creat, Unlink, GetPid, Sleep>;

inline bool operator==(const SystemCallVariant& lhs, const SystemCallVariant& rhs) {
    return std::visit([](const auto& lhs, const auto& rhs) { return lhs == rhs; }, lhs, rhs);
}

inline bool operator!=(const SystemCallVariant& lhs, const SystemCallVariant& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const SystemCallVariant& scv) {
    std::visit([&os](const auto& v) { os << v; }, scv);
    return os;
}

std::istream& operator>>(std::istream& is, SystemCallVariant& scv) {
    std::string input;
    is >> input;
    if (input == "exit"sv) {
        scv = Exit{};
    } else if (input == "read"sv) {
        scv = Read{};
    } else if (input == "write"sv) {
        scv = Write{};
    } else if (input == "open"sv) {
        scv = Open{};
    } else if (input == "close"sv) {
        scv = Close{};
    } else if (input == "creat"sv) {
        scv = Creat{};
    } else if (input == "unlink"sv) {
        scv = Unlink{};
    } else if (input == "getpid"sv) {
        scv = GetPid{};
    } else if (input == "sleep"sv) {
        scv = Sleep{};
    } else {
        throw std::invalid_argument{"Invalid system call"};
    }
    return is;
}

int main() {
    while (true) {
        try {
            SystemCallVariant scv;
            std::cin >> scv;
            scv.Execute();
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            break;
        }
    }
    return EXIT_FAILURE;
}
