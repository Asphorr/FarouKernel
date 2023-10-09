#ifndef DRIVER_H
#define DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include "process.h"

// Structure to represent a driver
typedef struct {
    const char* name;               // Name of the driver
    void (*entry_point)(void);      // Entry point of the driver
    volatile bool initialized;      // Whether the driver has been initialized
    volatile uint8_t status;        // Status of the driver (0x00=unloaded, 0x01=loading, 0x02=loaded, 0x03=unloading)
} driver_t;

// Functions to manipulate drivers
static inline void driver_init(driver_t* drv) {
    drv->initialized = true;
    drv->status = 0x01;
}

static inline void driver_load(driver_t* drv) {
    if (drv->initialized && drv->status == 0x01) {
        drv->status = 0x02;
        drv->entry_point();
    }
}

static inline void driver_unload(driver_t* drv) {
    if (drv->initialized && drv->status != 0x03) {
        drv->status = 0x03;
    }
}

// Functions to interact with the driver
static inline void driver_send_command(driver_t* drv, uint8_t command) {
    if (drv->initialized && drv->status == 0x02) {
        // Send command to driver
    }
}

static inline uint8_t driver_receive_data(driver_t* drv) {
    if (drv->initialized && drv->status == 0x02) {
        // Receive data from driver
        return 0;
    } else {
        return -1;
    }
}

// Macro to simplify driver initialization
#define DriverInit(drv) \
    do { \
        drv->initialized = true; \
        drv->status = 0x01; \
    } while (0)

// Macro to simplify driver loading
#define DriverLoad(drv) \
    do { \
        if (drv->initialized && drv->status == 0x01) { \
            drv->status = 0x02; \
            drv->entry_point(); \
        } \
    } while (0)

// Macro to simplify driver unloading
#define DriverUnload(drv) \
    do { \
        if (drv->initialized && drv->status != 0x03) { \
            drv->status = 0x03; \
        } \
    } while (0)

#endif // DRIVER_H
