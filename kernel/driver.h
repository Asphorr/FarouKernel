#ifndef DRIVER_H
#define DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include "process.h"

// Structure to represent a driver
typedef struct {
    char* name;                   // Name of the driver
    void (*entry_point)(void);  // Entry point of the driver
    bool initialized;           // Whether the driver has been initialized
    uint8_t status;             // Status of the driver (0x00=unloaded, 0x01=loading, 0x02=loaded, 0x03=unloading)
} driver_t;

// Functions to manipulate drivers
void driver_init(driver_t* drv);
void driver_load(driver_t* drv);
void driver_unload(driver_t* drv);

// Functions to interact with the driver
void driver_send_command(driver_t* drv, uint8_t command);
uint8_t driver_receive_data(driver_t* drv);

// Macro to simplify driver initialization
#define DriverInit(drv) \
    do { \
        drv->initialized = false; \
        driver_init(drv); \
    } while (0)

// Macro to simplify driver loading
#define DriverLoad(drv) \
    do { \
        if (!drv->initialized) { \
            driver_load(drv); \
        } \
    } while (0)

// Macro to simplify driver unloading
#define DriverUnload(drv) \
    do { \
        if (drv->initialized) { \
            driver_unload(drv); \
        } \
    } while (0)

#endif  // DRIVER_H
