// driver_i2c.h — Template for I²C device drivers (ESP-IDF I²C master)
// =============================================================================
// Use this template for devices that communicate via I²C (I2C):
//   - Sensors (BME280, BH1750, MPU6050, …)
//   - RTCs (DS1307, DS3231, …)
//   - I/O expanders (PCF8574, MCP23017, …)
//   - EEPROMs (AT24Cxx, …)
//   - Any peripheral with register-based I²C interface.
//
// PATTERN: Constructor initialises the I²C master bus, adds the device,
//          and optionally probes it.  Private helpers `i2cWriteByte()` and
//          `i2cReadBytes()` encapsulate the register-address + data protocol.
//          Public `readRegister()`, `writeRegister()`, `readRegisters()`,
//          `writeRegisters()` provide raw register access.  Device-specific
//          high-level methods (e.g. `getReading()`) build on top of these.
//
// ESP-IDF v5.4 I²C MASTER DRIVER API:
//   #include "driver/i2c_master.h"
//   i2c_new_master_bus()          — create bus
//   i2c_master_bus_add_device()   — add device to bus
//   i2c_master_probe()            — ping device address
//   i2c_master_transmit()         — write-only transaction
//   i2c_master_transmit_receive() — write-then-read transaction
//   i2c_master_bus_rm_device()    — remove device
//   i2c_del_master_bus()          — delete bus
//
// I²C bus standard speeds: 100 kHz (standard), 400 kHz (fast).
// Always check the device datasheet for its max SCL frequency.
//
// INSTRUCTIONS:
//   1. Copy and rename (e.g. CMySensor.h).
//   2. Replace __DEVICE_NAME__ / __device_name__ with your class name.
//   3. Set I²C slave address, register map, bit masks.
//   4. Add device-specific reading struct and high-level methods.
//   5. In the .cpp, implement getReading() / setConfig() etc.
// =============================================================================

#ifndef _DRIVER_I2C_H_
#define _DRIVER_I2C_H_

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include <stdint.h>

// --- I²C CONFIGURATION -------------------------------------------------------
// TODO: Set your device's I²C address (7-bit, shifted right by 1).
//       e.g. DS1307 = 0x68, BME280 = 0x76 or 0x77, BH1750 = 0x23.
#define __DEVICE_NAME___I2C_ADDR            0x00    // 7-bit slave address

// TODO: Set the default I²C clock speed (Hz).
//       Standard mode = 100 000, Fast mode = 400 000.
#define __DEVICE_NAME___I2C_CLK_SPEED       100000

// I²C transaction timeout in milliseconds.
// Increase for slow devices or large burst transfers.
#define __DEVICE_NAME___I2C_TIMEOUT_MS      10

// --- REGISTER MAP (device-specific) ------------------------------------------
// TODO: Add your device's register addresses and bit masks.
//       Example (DS1307 RTC):
//         #define __DEVICE_NAME___REG_CONTROL      0x07
//         #define __DEVICE_NAME___REG_DATA_START   0x08
//         #define __DEVICE_NAME___BIT_ENABLE       0x80

#define __DEVICE_NAME___REG_START          0x00    // First register
#define __DEVICE_NAME___REG_END            0x3F    // Last  register (or 0x00 for single)

// --- READING STRUCT ----------------------------------------------------------
// TODO: Customize with the values your device returns.
//       For simple sensors, include raw registers + parsed values.
//       For I/O expanders, this might hold pin states.
typedef struct {
    // --- Raw register values ---
    // uint8_t data[__DEVICE_NAME___REG_COUNT];

    // --- Parsed/decimal values (device-specific) ---
    // float   temperature;
    // float   humidity;
    // uint8_t status;

    bool    valid;              // True if read succeeded / checksum matched
} __device_name___reading_t;


// =============================================================================
// CLASS: __DeviceName__ — I²C peripheral driver
// =============================================================================
class __DeviceName__ {

public:
    // -------------------------------------------------------------------------
    // Constructor — Initialises I²C bus, adds device, optionally probes.
    //
    // @param port       I²C port number (I2C_NUM_0 or I2C_NUM_1)
    // @param sdaPin     GPIO pin for SDA
    // @param sclPin     GPIO pin for SCL
    // @param clkSpeedHz I²C clock speed (Hz).  Override for fast-mode devices.
    // -------------------------------------------------------------------------
    __DeviceName__(i2c_port_num_t port,
                   gpio_num_t      sdaPin,
                   gpio_num_t      sclPin,
                   uint32_t        clkSpeedHz = __DEVICE_NAME___I2C_CLK_SPEED);

    // -------------------------------------------------------------------------
    // Destructor — Removes device from bus and deletes the bus.
    // -------------------------------------------------------------------------
    ~__DeviceName__();

    // ===== RAW REGISTER ACCESS ===============================================

    // -------------------------------------------------------------------------
    // readRegister() — Read a single register byte.
    //
    // @param reg   Register address
    // @param value Output: register value
    // @return      true on success
    // -------------------------------------------------------------------------
    bool readRegister(uint8_t reg, uint8_t& value);

    // -------------------------------------------------------------------------
    // writeRegister() — Write a single register byte.
    //
    // @param reg    Register address
    // @param value  Value to write
    // @return       true on success
    // -------------------------------------------------------------------------
    bool writeRegister(uint8_t reg, uint8_t value);

    // -------------------------------------------------------------------------
    // readRegisters() — Read a block of consecutive registers.
    //                    Uses the device's auto-increment feature.
    //
    // @param startReg  Starting register address
    // @param buffer    Output buffer
    // @param len       Number of bytes to read
    // @return          true on success
    // -------------------------------------------------------------------------
    bool readRegisters(uint8_t startReg, uint8_t* buffer, size_t len);

    // -------------------------------------------------------------------------
    // writeRegisters() — Write a block of consecutive registers.
    //
    // @param startReg  Starting register address
    // @param buffer    Data to write
    // @param len       Number of bytes to write
    // @return          true on success
    // -------------------------------------------------------------------------
    bool writeRegisters(uint8_t startReg, const uint8_t* buffer, size_t len);

    // ===== DEVICE-SPECIFIC API ===============================================

    // -------------------------------------------------------------------------
    // getReading() — Perform a full device read and parse into a reading struct.
    //                TODO: Implement this for your device.
    // -------------------------------------------------------------------------
    __device_name___reading_t getReading();

    // -------------------------------------------------------------------------
    // (Add more device-specific methods here, e.g. setMode(), calibrate(), …)
    // -------------------------------------------------------------------------

    // ===== DIAGNOSTICS =======================================================

    // -------------------------------------------------------------------------
    // isConnected() — Probe the I²C bus to check if the device is responding.
    // -------------------------------------------------------------------------
    bool isConnected();

    // -------------------------------------------------------------------------
    // isInitialised() — Returns true if the constructor completed successfully
    //                   and the driver is ready for use.
    // -------------------------------------------------------------------------
    bool isInitialised() const;

protected:
    i2c_port_num_t           m_port;        // I²C port number
    gpio_num_t               m_sdaPin;      // SDA GPIO
    gpio_num_t               m_sclPin;      // SCL GPIO
    i2c_master_bus_handle_t  m_busHandle;   // I²C bus handle
    i2c_master_dev_handle_t  m_devHandle;   // I²C device handle
    const char*              TAG;           // Log tag for ESP_LOGx
    bool                     m_initialised; // Init status

    // --- Low-level I²C helpers -----------------------------------------------

    // Write one byte to a register.
    // I²C sequence: START → devAddr(W) → regAddr → data → STOP
    bool i2cWriteByte(uint8_t reg, uint8_t value);

    // Read N bytes starting from a register (auto-increment).
    // I²C sequence: START → devAddr(W) → regAddr → REP START → devAddr(R) → data[N] → STOP
    bool i2cReadBytes(uint8_t reg, uint8_t* buffer, size_t len);
};

#endif // _DRIVER_I2C_H_
