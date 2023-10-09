#pragma once

class Driver {
public:
    std::string name;
    std::function<void()> entryPoint;
    std::atomic<bool> initialized{false};
    std::atomic<uint8_t> status{0x00};

    static void init(Driver& drv) {
        drv.initialized = true;
        drv.status = 0x01;
    }

    static void load(Driver& drv) {
        if (!drv.initialized || drv.status != 0x01) {
            throw std::runtime_error("Cannot load driver");
        }
        drv.status = 0x02;
        drv.entryPoint();
    }

    static void unload(Driver& drv) {
        if (!drv.initialized || drv.status == 0x03) {
            throw std::runtime_error("Cannot unload driver");
        }
        drv.status = 0x03;
    }

    template <typename T>
    static void sendCommand(Driver& drv, T command) {
        if (!drv.initialized || drv.status != 0x02) {
            throw std::runtime_error("Cannot send command to driver");
        }
        // Send command to driver
    }

    template <typename T>
    static T receiveData(Driver& drv) {
        if (!drv.initialized || drv.status != 0x02) {
            throw std::runtime_error("Cannot receive data from driver");
        }
        // Receive data from driver
        return {};
    }
};

#define DriverInit(drv) \
    do { \
        drv.initialized = true; \
        drv.status = 0x01; \
    } while (0)

#define DriverLoad(drv) \
    do { \
        if (!drv.initialized || drv.status != 0x01) { \
            throw std::runtime_error("Cannot load driver"); \
        } \
        drv.status = 0x02; \
        drv.entryPoint(); \
    } while (0)

#define DriverUnload(drv) \
    do { \
        if (!drv.initialized || drv.status == 0x03) { \
            throw std::runtime_error("Cannot unload driver"); \
        } \
        drv.status = 0x03; \
    } while (0)
