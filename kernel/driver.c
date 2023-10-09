#include <iostream>
#include <memory>
#include <stdexcept>

// Define the driver interface
struct IDriver {
    virtual ~IDriver() = default;
    virtual void load() = 0;
    virtual void unload() = 0;
};

// Implement the driver for your device
class MyDevice : public IDriver {
public:
    MyDevice(const std::string& name) : name_(name) {}

    void load() override {
        // Perform any necessary initialization steps here
        // ...
    }

    void unload() override {
        // Perform any necessary cleanup steps here
        // ...
    }

private:
    const std::string name_;
};

// Create a factory method to create instances of the driver
template<typename T>
std::unique_ptr<T> make_driver(const std::string& name) {
    return std::make_unique<MyDevice>(name);
}

int main() {
    try {
        // Create an instance of the driver
        auto driver = make_driver<IDriver>("mydevice");

        // Load the driver
        driver->load();

        // Do something with the driver
        // ...

        // Unload the driver
        driver->unload();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
