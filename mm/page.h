#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

// A simple struct representing a page in memory
struct Page {
    std::unique_ptr<std::byte[]> buffer;   // Buffer containing the page data
    size_t length;                         // Length of the page data
    size_t offset;                         // Offset within the file where the page starts
    bool dirty;                            // Whether the page has been modified since last written to disk
    bool valid;                            // Whether the page contains valid data or not

    // Constructor
    Page(size_t length, size_t offset) : buffer{new std::byte[length]{}}, length{length}, offset{offset}, dirty{false}, valid{true} {}

    // Destructor
    ~Page() noexcept {
        if (buffer != nullptr) {
            delete[] buffer;
        }
    }

    // Copy constructor
    Page(const Page &other) : buffer{new std::byte[other.length]{}}, length{other.length}, offset{other.offset}, dirty{other.dirty}, valid{other.valid} {
        memcpy(buffer, other.buffer, length);
    }

    // Move constructor
    Page(Page &&other) noexcept : buffer{nullptr}, length{0}, offset{0}, dirty{false}, valid{false} {
        swap(*this, other);
    }

    // Assignment operator
    Page &operator=(Page other) {
        swap(*this, other);
        return *this;
    }

    // Swap function
    friend void swap(Page &a, Page &b) {
        std::swap(a.buffer, b.buffer);
        std::swap(a.length, b.length);
        std::swap(a.offset, b.offset);
        std::swap(a.dirty, b.dirty);
        std::swap(a.valid, b.valid);
    }

    // Equality comparison operator
    friend bool operator==(const Page &a, const Page &b) {
        return a.length == b.length && a.offset == b.offset && a.dirty == b.dirty && a.valid == b.valid;
    }

    // Inequality comparison operator
    friend bool operator!=(const Page &a, const Page &b) {
        return !(a == b);
    }

    // Less-than comparison operator
    friend bool operator<(const Page &a, const Page &b) {
        return a.offset < b.offset || (a.offset == b.offset && a.length < b.length);
    }

    // Greater-than comparison operator
    friend bool operator>(const Page &a, const Page &b) {
        return b < a;
    }

    // Less-than-or-equal-to comparison operator
    friend bool operator<=(const Page &a, const Page &b) {
        return !(b < a);
    }

    // Greater-than-or-equal-to comparison operator
    friend bool operator>=(const Page &a, const Page &b) {
        return !(a < b);
    }
};

int main() {
    // Create a vector of pages
    std::vector<Page> pages;

    // Add some sample pages to the vector
    pages.emplace_back(1024, 0);
    pages.emplace_back(512, 1024);
    pages.emplace_back(2048, 1536);

    // Print the contents of each page
    for (auto &page : pages) {
        std::cout << "Page #" << page.offset / 1024 + 1 << ": ";
        for (size_t i = 0; i < page.length; ++i) {
            std::cout << static_cast<char>(page.buffer[i]);
        }
        std::cout << "\n";
    }

    return 0;
}
