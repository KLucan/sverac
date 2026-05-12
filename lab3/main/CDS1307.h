#ifndef _CDS1307_H_
#define _CDS1307_H_

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include <stdint.h>

#define DS1307_I2C_ADDR 0x68 // 7-bit fiksna adresa

// REGISTRI
#define DS1307_REG_SECONDS 0x00    // bit 7 = halt / sekunde = bit 6-4 * 10 + bit 3-0
#define DS1307_REG_MINUTES 0x01    // minute = bit 6-4 * 10 + bit 3-0
#define DS1307_REG_HOURS 0x02      // 24satni - bit 6 = 1 / sati = bit 5-4 * 10 + bit 3-0
#define DS1307_REG_DAY 0x03        // dan u tjednu = bit 3-0
#define DS1307_REG_DATE 0x04       // dan u mjesecu = bit 5-4 * 10 + bit 3-0
#define DS1307_REG_MONTH 0x05      // mjesec = bit 4 * 10 + bit 3-0
#define DS1307_REG_YEAR 0x06       // godina = bit 7-4 * 10 + bit 3-0
#define DS1307_REG_CONTROL 0x07    // kontrola SQW
#define DS1307_REG_SRAM_START 0x08 // pocetak SRAM
#define DS1307_REG_SRAM_END 0x3F   // kraj SRAM
#define DS1307_SRAM_SIZE 56        // 0x08–0x3F

// MASKE
// sekunde
#define DS1307_CH 0x80       // CH (halt)
#define DS1307_SEC_MASK 0x7F // ne-ch

// sati
#define DS1307_12_24 0x40       // 12satni / 24 satni
#define DS1307_AM_PM 0x20       // AM / PM
#define DS1307_24_10H_MASK 0x30 // 24satni - sati * 10
#define DS1307_12_10H_MASK 0x10 // 12satni - sati * 10
#define DS1307_HOUR_MASK 0x0F   // sati

// kontrola SQW
#define DS1307_OUT 0x80  // razina kad je SQWE = 0
#define DS1307_SQWE 0x10 // SQW enable
#define DS1307_RS1 0x02  // rate select 1
#define DS1307_RS0 0x01  // rate select 0

#define DS1307_SQW_1HZ 0x00     // RS = 00 -> 1 Hz
#define DS1307_SQW_4096HZ 0x01  // RS = 01 -> 4.096 kHz
#define DS1307_SQW_8192HZ 0x02  // RS = 10 -> 8.192 kHz
#define DS1307_SQW_32768HZ 0x03 // RS = 11 -> 32.768 kHz

// STRUKTURE

typedef struct
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    bool is12h;
    bool isPM;
    uint8_t dayOfWeek; // 1 = Ponedjeljak jer nismo u americi
    uint8_t date;
    uint8_t month;
    uint8_t year;
    bool valid;
} ds1307_time_t;

// KLASE

class CDS1307
{

public:
    CDS1307(i2c_port_num_t port,         // I2C port
            gpio_num_t sdaPin,           // GPIO SDA
            gpio_num_t sclPin,           // GPIO SCL
            uint32_t clkSpeedHz = 100000 // max 100000
    );
    ~CDS1307();

    bool setTime(ds1307_time_t time);
    ds1307_time_t getTime();

    bool readRegister(uint8_t reg, uint8_t &value);
    bool writeRegister(uint8_t reg, uint8_t value);

    bool readRegisters(uint8_t startReg, uint8_t *buffer, size_t len);
    bool writeRegisters(uint8_t startReg, const uint8_t *buffer, size_t len);

    bool setClockHalt(bool halt);
    bool isClockHalted();

    bool set24HourMode(bool mode24);

    bool setSquareWave(bool enable, uint8_t frequency = DS1307_SQW_1HZ);
    bool setOutputLevel(bool high);

    bool writeSRAM(uint8_t address, uint8_t value);
    bool readSRAM(uint8_t address, uint8_t &value);
    bool writeSRAMBlock(uint8_t address, const uint8_t *buffer, size_t len);
    bool readSRAMBlock(uint8_t address, uint8_t *buffer, size_t len);

    bool isConnected();
    bool isInitialised() const;

private:
    i2c_port_num_t m_port;               // I2C port
    gpio_num_t m_sdaPin;                 // GPIO SDA
    gpio_num_t m_sclPin;                 // GPIO SCL
    i2c_master_bus_handle_t m_busHandle; // I2C bus ESP-IDF
    i2c_master_dev_handle_t m_devHandle; // I2C device ESP_IDF
    const char *TAG;
    bool m_initialised;

    // I2C POMAGAČI

    // jedan bit u registar reg: [START][devAddr+W][regAddr][value][STOP]
    bool i2cWriteByte(uint8_t reg, uint8_t value);

    // n bitova od registra reg: [START][devAddr+W][regAddr][REP START][devAddr+R][data0…dataN-1 NACK][STOP]
    bool i2cReadBytes(uint8_t reg, uint8_t *buffer, size_t len);

    // BCD POMAGAČI
    static uint8_t decToBcd(uint8_t val);
    static uint8_t bcdToDec(uint8_t bcd);
};

#endif // _CDS1307_H_