#include <iostream>
#include <memory>
#include <stdexcept>

class Driver {
public:
    explicit Driver(const std::string& name): name_(name), status_(DriverStatus::UNLOADED) {}

    void load() {
        if (status_ != DriverStatus::UNLOADED) {
            throw std::runtime_error("Cannot load driver: already loaded");
        }
        // Perform any necessary initialization steps here
        // ...
        status_ = DriverStatus::LOADED;
    }

    void unload() {
        if (status_ != DriverStatus::LOADED) {
            throw std::runtime_error("Cannot unload driver: not loaded");
        }
        // Perform any necessary cleanup steps here
        // ...
        status_ = DriverStatus::UNLOADED;
    }

private:
    const std::string name_;
    DriverStatus status_;
};

enum class DriverStatus { UNLOADED, LOADED };

int main() {
    try {
        Driver driver("mydevice");
        driver.load();
        // Do something with the driver
        driver.unload();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
