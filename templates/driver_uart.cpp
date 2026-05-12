// driver_uart.cpp — Template for UART-based device drivers
// =============================================================================
// Fill in the stubs labelled "TODO" with your device-specific logic.
// See driver_uart.h for the full instructions.
// =============================================================================

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "__DeviceName__.h"       // TODO: rename to your header

// --- CONSTRUCTOR -------------------------------------------------------------
__DeviceName__::__DeviceName__(int baudRate) {
    m_uartPort   = __DEVICE_NAME___UART_PORT;
    m_eventQueue = nullptr;
    LogName      = "__DeviceName__";    // TODO: set your log tag

    int baud = (baudRate > 0) ? baudRate : __DEVICE_NAME___UART_BAUD_RATE;

    ESP_LOGI(LogName, "Initialising UART%d @ %d baud (TX=%d, RX=%d)",
             (int)m_uartPort, baud,
             __DEVICE_NAME___UART_TX_PIN, __DEVICE_NAME___UART_RX_PIN);

    // --- Step 1: Configure UART parameters -----------------------------------
    uart_config_t uartCfg = {};
    uartCfg.baud_rate           = baud;
    uartCfg.data_bits           = __DEVICE_NAME___UART_DATA_BITS;
    uartCfg.parity              = __DEVICE_NAME___UART_PARITY;
    uartCfg.stop_bits           = __DEVICE_NAME___UART_STOP_BITS;
    uartCfg.flow_ctrl           = __DEVICE_NAME___UART_FLOW_CTRL;
    uartCfg.rx_flow_ctrl_thresh = 122;
    uartCfg.source_clk          = UART_SCLK_DEFAULT;  // Use default clock

    ESP_ERROR_CHECK(uart_param_config(m_uartPort, &uartCfg));

    // --- Step 2: Set communication pins --------------------------------------
    ESP_ERROR_CHECK(uart_set_pin(
        m_uartPort,
        __DEVICE_NAME___UART_TX_PIN,
        __DEVICE_NAME___UART_RX_PIN,
        __DEVICE_NAME___UART_RTS_PIN,
        __DEVICE_NAME___UART_CTS_PIN
    ));

    // --- Step 3: Install UART driver -----------------------------------------
    ESP_ERROR_CHECK(uart_driver_install(
        m_uartPort,
        __DEVICE_NAME___UART_RX_BUF_SIZE,
        __DEVICE_NAME___UART_TX_BUF_SIZE,
        __DEVICE_NAME___UART_EVENT_QUEUE_SIZE,
        &m_eventQueue,
        0   // intr_alloc_flags
    ));

    // --- Step 4: (Optional) Set UART mode ------------------------------------
    // TODO: Uncomment if your device needs RS-485 or other modes.
    // ESP_ERROR_CHECK(uart_set_mode(m_uartPort, UART_MODE_RS485_HALF_DUPLEX));

    ESP_LOGI(LogName, "UART initialisation complete");

    // TODO: Add device-specific init here.
    //       e.g. send a wake-up command, wait for boot message, etc.
}

// --- DESTRUCTOR --------------------------------------------------------------
__DeviceName__::~__DeviceName__() {
    if (uart_is_driver_installed(m_uartPort)) {
        uart_driver_delete(m_uartPort);
    }
    ESP_LOGI(LogName, "UART deinitialised");
}

// --- SEND (raw bytes) --------------------------------------------------------
int __DeviceName__::send(const uint8_t* data, int len) {
    if (data == nullptr || len <= 0) return -1;

    int sent = uart_write_bytes(m_uartPort, data, len);
    if (sent < 0) {
        ESP_LOGE(LogName, "uart_write_bytes failed");
    }
    return sent;
}

// --- SEND STRING -------------------------------------------------------------
int __DeviceName__::sendString(const char* str) {
    if (str == nullptr) return -1;
    return send((const uint8_t*)str, strlen(str));
}

// --- SEND COMMAND (string + CR/LF) -------------------------------------------
int __DeviceName__::sendCommand(const char* cmd, int timeoutMs) {
    if (cmd == nullptr) return -1;

    // Build command with \r\n terminator (common for AT-style commands)
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);

    int sent = send((const uint8_t*)buffer, len);
    if (sent > 0) {
        // Wait for TX to complete
        uart_wait_tx_done(m_uartPort, pdMS_TO_TICKS(timeoutMs));
    }
    return sent;
}

// --- RECEIVE -----------------------------------------------------------------
int __DeviceName__::receive(uint8_t* buffer, int maxLen, int timeoutMs) {
    if (buffer == nullptr || maxLen <= 0) return -1;

    int len = uart_read_bytes(m_uartPort, buffer, maxLen,
                              pdMS_TO_TICKS(timeoutMs));
    if (len < 0) {
        ESP_LOGE(LogName, "uart_read_bytes failed");
    }
    return len;
}

// --- RECEIVE UNTIL -----------------------------------------------------------
int __DeviceName__::receiveUntil(uint8_t* buffer, int maxLen,
                                  char delimiter, int timeoutMs) {
    if (buffer == nullptr || maxLen <= 0) return -1;

    int totalLen = 0;
    int64_t start = esp_timer_get_time();

    while (totalLen < maxLen) {
        // Check timeout
        if (esp_timer_get_time() - start > (int64_t)timeoutMs * 1000LL) {
            ESP_LOGW(LogName, "receiveUntil: timeout after %d bytes", totalLen);
            return totalLen;   // return what we have so far
        }

        // Read one byte
        uint8_t ch;
        int len = uart_read_bytes(m_uartPort, &ch, 1, pdMS_TO_TICKS(10));
        if (len <= 0) continue;

        buffer[totalLen++] = ch;
        if (ch == (uint8_t)delimiter) {
            break;
        }
    }

    return totalLen;
}

// --- FLUSH -------------------------------------------------------------------
void __DeviceName__::flush() {
    uart_flush_input(m_uartPort);
}

// --- AVAILABLE ---------------------------------------------------------------
int __DeviceName__::available() {
    size_t len = 0;
    if (uart_get_buffered_data_len(m_uartPort, &len) == ESP_OK) {
        return (int)len;
    }
    return -1;
}

// --- GET READING -------------------------------------------------------------
__device_name___reading_t __DeviceName__::getReading() {
    __device_name___reading_t reading = {};
    reading.valid = false;

    // --- Read raw data ---
    int len = receive(reading.buffer, sizeof(reading.buffer) - 1, 200);
    if (len <= 0) {
        return reading;
    }

    reading.buffer[len] = '\0';   // null-terminate for string processing
    reading.length = len;

    ESP_LOGI(LogName, "Received %d bytes: \"%.*s\"", len, len, reading.buffer);

    // =========================================================================
    // TODO: Parse the received data according to your device's protocol.
    //
    // *** Example 1: NMEA GPS sentence ***
    //   Parse $GPGGA, $GPRMC, etc.
    //   sscanf or manual comma-separated field extraction.
    //
    // *** Example 2: Binary protocol with fixed-length frames ***
    //   Check start byte, length field, CRC, extract payload.
    //
    // *** Example 3: Line-oriented text protocol ***
    //   Use sscanf on the null-terminated buffer.
    // =========================================================================

    // TODO: Set reading.valid = true after successful parsing.
    reading.valid = true;

    return reading;
}
