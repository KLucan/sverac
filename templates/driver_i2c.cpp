// driver_i2c.cpp — Template for I²C device driver implementation
// =============================================================================
// Implements the generic I²C register read/write pattern using the ESP-IDF
// v5.4 I²C master driver API.  Fill in device-specific logic in the sections
// marked "TODO".
//
// See driver_i2c.h for the full instructions.
// =============================================================================

#include "__DeviceName__.h"      // TODO: rename to your header

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_err.h"

// =============================================================================
// CONSTRUCTOR
// =============================================================================
__DeviceName__::__DeviceName__(i2c_port_num_t port,
                               gpio_num_t      sdaPin,
                               gpio_num_t      sclPin,
                               uint32_t        clkSpeedHz)
    : m_port(port)
    , m_sdaPin(sdaPin)
    , m_sclPin(sclPin)
    , m_busHandle(nullptr)
    , m_devHandle(nullptr)
    , TAG("__DeviceName__")     // TODO: set your log tag
    , m_initialised(false)
{
    ESP_LOGI(TAG, "Initialising on I²C port %d (SDA=GPIO%d, SCL=GPIO%d, SCL=%" PRIu32 " Hz)",
             (int)port, (int)sdaPin, (int)sclPin, clkSpeedHz);

    // --- 1. Configure I²C master bus -----------------------------------------
    i2c_master_bus_config_t busConfig = {};
    busConfig.i2c_port          = port;
    busConfig.sda_io_num        = sdaPin;
    busConfig.scl_io_num        = sclPin;
    busConfig.clk_source        = I2C_CLK_SRC_DEFAULT;
    busConfig.glitch_ignore_cnt = 7;
    busConfig.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&busConfig, &m_busHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I²C bus: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "I²C bus created");

    // --- 2. Add device to the bus --------------------------------------------
    i2c_device_config_t devConfig = {};
    devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    devConfig.device_address  = __DEVICE_NAME___I2C_ADDR;
    devConfig.scl_speed_hz    = clkSpeedHz;

    err = i2c_master_bus_add_device(m_busHandle, &devConfig, &m_devHandle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device: %s", esp_err_to_name(err));
        i2c_del_master_bus(m_busHandle);
        m_busHandle = nullptr;
        return;
    }
    ESP_LOGI(TAG, "Device added to bus (addr=0x%02X)", __DEVICE_NAME___I2C_ADDR);

    // --- 3. Probe the device -------------------------------------------------
    err = i2c_master_probe(m_busHandle, __DEVICE_NAME___I2C_ADDR,
                           __DEVICE_NAME___I2C_TIMEOUT_MS * 5);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Device probe failed: %s (check wiring/pull-ups)",
                 esp_err_to_name(err));
        // Don't abort — device may still work; call isConnected() to verify.
    } else {
        ESP_LOGI(TAG, "Device detected on I²C bus");
    }

    // --- 4. Device-specific initialisation -----------------------------------
    // TODO: Write config registers, clear flags, start conversions, etc.
    //       Example:
    //       writeRegister(REG_CONTROL, BIT_ENABLE);

    m_initialised = true;
    ESP_LOGI(TAG, "Driver initialised");
}

// =============================================================================
// DESTRUCTOR
// =============================================================================
__DeviceName__::~__DeviceName__() {
    ESP_LOGI(TAG, "Shutting down driver");

    // TODO: Put device into low-power / standby mode if needed.

    if (m_devHandle) {
        i2c_master_bus_rm_device(m_devHandle);
        m_devHandle = nullptr;
    }
    if (m_busHandle) {
        i2c_del_master_bus(m_busHandle);
        m_busHandle = nullptr;
    }

    m_initialised = false;
    ESP_LOGI(TAG, "Driver shut down");
}

// =============================================================================
// LOW-LEVEL I²C HELPERS
// =============================================================================

// Write a single byte to a register.
// I²C sequence: START → devAddr(W) → regAddr → data → STOP
bool __DeviceName__::i2cWriteByte(uint8_t reg, uint8_t value) {
    if (!m_devHandle) {
        ESP_LOGE(TAG, "i2cWriteByte: device handle is null");
        return false;
    }

    uint8_t buffer[2] = { reg, value };
    esp_err_t err = i2c_master_transmit(m_devHandle, buffer, sizeof(buffer),
                                        __DEVICE_NAME___I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2cWriteByte(reg=0x%02X, val=0x%02X) failed: %s",
                 reg, value, esp_err_to_name(err));
        return false;
    }
    return true;
}

// Read N bytes starting from a register.
// I²C sequence: START → devAddr(W) → regAddr → REP START → devAddr(R) → data[N] → STOP
// Most I²C devices auto-increment the register pointer on each byte read.
bool __DeviceName__::i2cReadBytes(uint8_t reg, uint8_t* buffer, size_t len) {
    if (!m_devHandle) {
        ESP_LOGE(TAG, "i2cReadBytes: device handle is null");
        return false;
    }
    if (!buffer || len == 0) {
        ESP_LOGE(TAG, "i2cReadBytes: invalid buffer or length");
        return false;
    }

    uint8_t regAddr = reg;
    esp_err_t err = i2c_master_transmit_receive(
        m_devHandle,
        &regAddr, 1,                               // Write: register address
        buffer, len,                                // Read: data bytes
        __DEVICE_NAME___I2C_TIMEOUT_MS
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2cReadBytes(reg=0x%02X, len=%d) failed: %s",
                 reg, (int)len, esp_err_to_name(err));
        return false;
    }
    return true;
}

// =============================================================================
// RAW REGISTER ACCESS
// =============================================================================

bool __DeviceName__::readRegister(uint8_t reg, uint8_t& value) {
    return i2cReadBytes(reg, &value, 1);
}

bool __DeviceName__::writeRegister(uint8_t reg, uint8_t value) {
    return i2cWriteByte(reg, value);
}

// --- Block read (auto-increment) ---------------------------------------------
bool __DeviceName__::readRegisters(uint8_t startReg, uint8_t* buffer, size_t len) {
    return i2cReadBytes(startReg, buffer, len);
}

// --- Block write (auto-increment) --------------------------------------------
bool __DeviceName__::writeRegisters(uint8_t startReg, const uint8_t* buffer,
                                    size_t len) {
    if (!m_devHandle || !buffer || len == 0) {
        return false;
    }

    // Build write buffer: [regAddr, data0, data1, …, dataN-1]
    // The device auto-increments the register pointer.
    // For small writes (≤ 64 bytes payload), use a stack buffer.
    constexpr size_t MAX_STACK = 64;
    if (len <= MAX_STACK) {
        uint8_t tmp[MAX_STACK + 1];
        tmp[0] = startReg;
        for (size_t i = 0; i < len; i++) {
            tmp[i + 1] = buffer[i];
        }
        esp_err_t err = i2c_master_transmit(m_devHandle, tmp, len + 1,
                                            __DEVICE_NAME___I2C_TIMEOUT_MS);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "writeRegisters(start=0x%02X, len=%d) failed: %s",
                     startReg, (int)len, esp_err_to_name(err));
            return false;
        }
        return true;
    }

    // For larger blocks, allocate on the heap.
    uint8_t* tmp = new uint8_t[len + 1];
    if (!tmp) {
        ESP_LOGE(TAG, "writeRegisters: memory allocation failed");
        return false;
    }
    tmp[0] = startReg;
    for (size_t i = 0; i < len; i++) {
        tmp[i + 1] = buffer[i];
    }
    esp_err_t err = i2c_master_transmit(m_devHandle, tmp, len + 1,
                                        __DEVICE_NAME___I2C_TIMEOUT_MS);
    delete[] tmp;

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "writeRegisters(start=0x%02X, len=%d) failed: %s",
                 startReg, (int)len, esp_err_to_name(err));
        return false;
    }
    return true;
}

// =============================================================================
// DEVICE-SPECIFIC API
// =============================================================================

// --- getReading() — Read and parse device data -------------------------------
// TODO: Implement this for your device.
//
// Generic pattern:
//   1. Read status/config register to check if data is ready.
//   2. Burst-read data registers using readRegisters().
//   3. Parse raw bytes into sensor values (scaling, two's complement, BCD, …).
//   4. Validate checksum/CRC if the device has one.
//   5. Return filled reading struct with valid = true/false.
//
// Example (simple sensor with 2-byte temperature value):
//
//   __device_name___reading_t __DeviceName__::getReading() {
//       __device_name___reading_t r = {};
//       uint8_t regs[2] = {};
//       if (!readRegisters(REG_TEMP_MSB, regs, 2)) return r;
//
//       int16_t raw = ((int16_t)regs[0] << 8) | regs[1];
//       r.temperature = raw * 0.01f;      // device-specific scaling
//       r.valid = true;
//       return r;
//   }

__device_name___reading_t __DeviceName__::getReading() {
    __device_name___reading_t reading = {};
    reading.valid = false;

    // TODO: Implement device-specific read sequence.
    // uint8_t regs[__DEVICE_NAME___REG_COUNT] = {};
    // if (!readRegisters(__DEVICE_NAME___REG_START, regs, sizeof(regs))) {
    //     return reading;
    // }
    //
    // … parse regs into reading fields …
    //
    // reading.valid = true;
    // return reading;

    ESP_LOGW(TAG, "getReading() not implemented — returning empty reading");
    return reading;
}

// =============================================================================
// DIAGNOSTICS
// =============================================================================

bool __DeviceName__::isConnected() {
    if (!m_busHandle) {
        return false;
    }
    esp_err_t err = i2c_master_probe(m_busHandle, __DEVICE_NAME___I2C_ADDR,
                                     __DEVICE_NAME___I2C_TIMEOUT_MS * 2);
    return (err == ESP_OK);
}

bool __DeviceName__::isInitialised() const {
    return m_initialised && m_busHandle && m_devHandle;
}
