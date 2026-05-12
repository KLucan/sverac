// driver_uart.h — Template for UART-based device drivers
// =============================================================================
// Use this template for devices that communicate via UART (serial):
//   - GPS modules (NMEA sentences)
//   - Serial LCD displays
//   - UART sensors (CO2, PM2.5, etc.)
//   - Any device with TX/RX lines using standard async serial.
//
// PATTERN: Constructor installs the UART driver with specified baud rate,
//          pins, buffer sizes.  Methods: send(), receive(), sendCommand().
//          Optional: event queue for async UART events (pattern detect, etc.).
//
// ESP-IDF v5.4 UART API:
//   #include "driver/uart.h"
//   uart_driver_install(), uart_param_config(), uart_set_pin(),
//   uart_write_bytes(), uart_read_bytes().
//
// INSTRUCTIONS:
//   1. Copy and rename (e.g. CGps.h).
//   2. Replace __DEVICE_NAME__ with your class name.
//   3. Set UART port, baud rate, TX/RX pins, buffer sizes.
//   4. Customize the reading struct if your device returns structured data.
//   5. Add protocol-specific parsing in the receive/parse methods.
// =============================================================================

#ifndef _DRIVER_UART_H_
#define _DRIVER_UART_H_

#include "driver/uart.h"        // uart_port_t, uart_config_t, etc.
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"     // QueueHandle_t
#include <stdint.h>

// --- UART HARDWARE CONFIGURATION ---------------------------------------------
// TODO: Set these based on your device's requirements and board wiring.
#define __DEVICE_NAME___UART_PORT        UART_NUM_1       // UART_NUM_0, UART_NUM_1, UART_NUM_2
#define __DEVICE_NAME___UART_BAUD_RATE   9600             // Common baud rates: 9600, 115200, etc.
#define __DEVICE_NAME___UART_TX_PIN      GPIO_NUM_17      // TODO: set your TX pin
#define __DEVICE_NAME___UART_RX_PIN      GPIO_NUM_16      // TODO: set your RX pin
#define __DEVICE_NAME___UART_RTS_PIN     UART_PIN_NO_CHANGE  // -1 if not used
#define __DEVICE_NAME___UART_CTS_PIN     UART_PIN_NO_CHANGE  // -1 if not used

// --- UART BUFFER SIZES -------------------------------------------------------
#define __DEVICE_NAME___UART_RX_BUF_SIZE 1024
#define __DEVICE_NAME___UART_TX_BUF_SIZE 1024
#define __DEVICE_NAME___UART_EVENT_QUEUE_SIZE 10

// --- UART FRAME FORMAT -------------------------------------------------------
#define __DEVICE_NAME___UART_DATA_BITS   UART_DATA_8_BITS
#define __DEVICE_NAME___UART_PARITY      UART_PARITY_DISABLE
#define __DEVICE_NAME___UART_STOP_BITS   UART_STOP_BITS_1
#define __DEVICE_NAME___UART_FLOW_CTRL   UART_HW_FLOWCTRL_DISABLE

// --- READING STRUCT ----------------------------------------------------------
// TODO: Customize with the values your device returns.
typedef struct {
    // --- Raw received data ---
    uint8_t buffer[__DEVICE_NAME___UART_RX_BUF_SIZE];
    int     length;             // Number of bytes received

    // --- Parsed values (device-specific) ---
    // float   latitude;
    // float   longitude;
    // float   altitude;
    // int     satellites;

    bool    valid;              // Whether the data was received and parsed OK
} __device_name___reading_t;


// --- CLASS DECLARATION -------------------------------------------------------
class __DeviceName__ {

public:
    // -------------------------------------------------------------------------
    // Constructor — installs UART driver and configures pins.
    // @param baudRate  Override default baud rate (0 = use default).
    // -------------------------------------------------------------------------
    __DeviceName__(int baudRate = 0);

    // -------------------------------------------------------------------------
    // Destructor — uninstalls UART driver and frees resources.
    // -------------------------------------------------------------------------
    ~__DeviceName__();

    // -------------------------------------------------------------------------
    // send() — Transmit raw bytes.
    // @param data   Pointer to data buffer
    // @param len    Number of bytes to send
    // @return       Number of bytes sent, or -1 on error
    // -------------------------------------------------------------------------
    int send(const uint8_t* data, int len);

    // -------------------------------------------------------------------------
    // sendString() — Transmit a null-terminated string.
    // -------------------------------------------------------------------------
    int sendString(const char* str);

    // -------------------------------------------------------------------------
    // sendCommand() — Send a command string + CR/LF terminator (common pattern).
    // @param cmd     Command string (e.g. "AT")
    // @param timeoutMs  Max wait for TX complete
    // -------------------------------------------------------------------------
    int sendCommand(const char* cmd, int timeoutMs = 100);

    // -------------------------------------------------------------------------
    // receive() — Read available bytes into buffer.
    // @param buffer    Output buffer
    // @param maxLen    Maximum number of bytes to read
    // @param timeoutMs Max wait time in ms
    // @return          Number of bytes read, or -1 on error
    // -------------------------------------------------------------------------
    int receive(uint8_t* buffer, int maxLen, int timeoutMs = 100);

    // -------------------------------------------------------------------------
    // receiveUntil() — Read until a delimiter character is found.
    // @param buffer     Output buffer
    // @param maxLen     Maximum number of bytes to read
    // @param delimiter  Stop character (e.g. '\n' for line-oriented protocols)
    // @param timeoutMs  Max wait time in ms
    // @return           Number of bytes read (including delimiter), or -1
    // -------------------------------------------------------------------------
    int receiveUntil(uint8_t* buffer, int maxLen, char delimiter, int timeoutMs = 500);

    // -------------------------------------------------------------------------
    // getReading() — Read and parse a full data frame from the device.
    //                TODO: Implement device-specific parsing here.
    // -------------------------------------------------------------------------
    __device_name___reading_t getReading();

    // -------------------------------------------------------------------------
    // flush() — Discard all data in the RX buffer.
    // -------------------------------------------------------------------------
    void flush();

    // -------------------------------------------------------------------------
    // available() — Return the number of bytes available in RX buffer.
    // -------------------------------------------------------------------------
    int available();

private:
    uart_port_t  m_uartPort;           // UART port number
    QueueHandle_t m_eventQueue;        // UART event queue handle
    const char*  LogName;              // Tag for ESP_LOGx macros

    // TODO: Add device-specific state here (e.g. parser state machine).
};

#endif // _DRIVER_UART_H_
