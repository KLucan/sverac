#include "CDS1307.h"

#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_err.h"

#define I2C_TIMEOUT_MS 10

CDS1307::CDS1307(i2c_port_num_t port,
                 gpio_num_t sdaPin,
                 gpio_num_t sclPin,
                 uint32_t clkSpeedHz)
    : m_port(port), m_sdaPin(sdaPin), m_sclPin(sclPin), m_busHandle(nullptr), m_devHandle(nullptr), TAG("CDS1307"), m_initialised(false)
{
    ESP_LOGI(TAG, "INIT DS1307 I2C port %d (SDA=GPIO%d, SCL=GPIO%d, SCL=%u Hz)",
             (int)port, (int)sdaPin, (int)sclPin, clkSpeedHz);

    i2c_master_bus_config_t busConfig = {};
    busConfig.i2c_port = port;
    busConfig.sda_io_num = sdaPin;
    busConfig.scl_io_num = sclPin;
    busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
    busConfig.glitch_ignore_cnt = 7;
    busConfig.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&busConfig, &m_busHandle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C ERROR: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "I2C bus INIT complete!");

    i2c_device_config_t devConfig = {};
    devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    devConfig.device_address = DS1307_I2C_ADDR;
    devConfig.scl_speed_hz = clkSpeedHz;

    err = i2c_master_bus_add_device(m_busHandle, &devConfig, &m_devHandle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "DS1307 ERROR: %s", esp_err_to_name(err));
        // čišćenje
        i2c_del_master_bus(m_busHandle);
        m_busHandle = nullptr;
        return;
    }

    // CH INIT
    uint8_t secReg = 0;
    if (readRegister(DS1307_REG_SECONDS, secReg))
    {
        if (secReg & DS1307_CH)
        {
            ESP_LOGI(TAG, "SAT ZAUSTAVLJEN - POKRECEM");
            setClockHalt(false);
        }
    }

    m_initialised = true;
    ESP_LOGI(TAG, "DS1307 INIT complete!");
}

CDS1307::~CDS1307()
{
    if (m_devHandle)
    {
        i2c_master_bus_rm_device(m_devHandle);
        m_devHandle = nullptr;
    }
    if (m_busHandle)
    {
        i2c_del_master_bus(m_busHandle);
        m_busHandle = nullptr;
    }
    m_initialised = false;
    ESP_LOGI(TAG, "DS1307 DELETE");
}

// BCD POMAGAČI

uint8_t CDS1307::decToBcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

uint8_t CDS1307::bcdToDec(uint8_t bcd)
{
    return ((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F);
}

// I2C POMAGAČI

bool CDS1307::i2cWriteByte(uint8_t reg, uint8_t value)
{
    if (!m_devHandle)
    {
        ESP_LOGE(TAG, "I2C handle ne postoji!");
        return false;
    }

    uint8_t buffer[2] = {reg, value};
    esp_err_t err = i2c_master_transmit(m_devHandle, buffer, sizeof(buffer), I2C_TIMEOUT_MS);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C WRITE ERROR (reg=0x%02X, val=0x%02X) %s", reg, value, esp_err_to_name(err));
        return false;
    }
    return true;
}

bool CDS1307::i2cReadBytes(uint8_t reg, uint8_t *buffer, size_t len)
{
    if (!m_devHandle)
    {
        ESP_LOGE(TAG, "I2C handle ne postoji!");
        return false;
    }
    if (!buffer || len == 0)
    {
        ESP_LOGE(TAG, "Buffer ne postoji");
        return false;
    }

    uint8_t regAddr = reg;
    esp_err_t err = i2c_master_transmit_receive(m_devHandle, &regAddr, 1, buffer, len, I2C_TIMEOUT_MS);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C READ ERROR (reg=0x%02X, len=%d): %s", reg, (int)len, esp_err_to_name(err));
        return false;
    }
    return true;
}

// REGISTRI

bool CDS1307::readRegister(uint8_t reg, uint8_t &value)
{
    return i2cReadBytes(reg, &value, 1);
}

bool CDS1307::writeRegister(uint8_t reg, uint8_t value)
{
    return i2cWriteByte(reg, value);
}

bool CDS1307::readRegisters(uint8_t startReg, uint8_t *buffer, size_t len)
{
    return i2cReadBytes(startReg, buffer, len);
}

bool CDS1307::writeRegisters(uint8_t startReg, const uint8_t *buffer, size_t len)
{
    if (!m_devHandle || !buffer || len == 0)
    {
        return false;
    }

    int maxStog = 64;
    if (len <= maxStog)
    {
        uint8_t tmp[maxStog + 1];
        tmp[0] = startReg;
        for (int i = 0; i < len; i++)
        {
            tmp[i + 1] = buffer[i];
        }
        esp_err_t err = i2c_master_transmit(m_devHandle, tmp, len + 1, I2C_TIMEOUT_MS);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "REGISTER WRITE ERROR (start=0x%02X, len=%d): %s", startReg, (int)len, esp_err_to_name(err));
            return false;
        }
        return true;
    }
    return false;
}

bool CDS1307::setTime(ds1307_time_t time)
{

    if (!time.valid)
        return false;

    ESP_LOGI(TAG, "SET TIME: %02d/%02d/%02d (%d) %02d:%02d:%02d", time.year, time.month, time.date, time.dayOfWeek, time.hours, time.minutes, time.seconds);

    if (time.year > 99 || time.month < 1 || time.month > 12 || time.date < 1 || time.date > 31 || time.dayOfWeek < 1 || time.dayOfWeek > 7 || time.hours > 23 || time.minutes > 59 || time.seconds > 59)
    {
        ESP_LOGE(TAG, "Neispravno vrijeme!");
        return false;
    }

    // čuvamo 12/24satno
    uint8_t hourReg = 0;
    bool is24h = true;
    if (readRegister(DS1307_REG_HOURS, hourReg))
    {
        is24h = ((hourReg & DS1307_12_24) == 0);
    }

    uint8_t regs[7];

    // čuvamo CH
    uint8_t secReg = 0;
    readRegister(DS1307_REG_SECONDS, secReg);
    regs[0] = (secReg & DS1307_CH) | decToBcd(time.seconds);

    // minute
    regs[1] = decToBcd(time.minutes);

    // sati
    if (is24h)
    {
        regs[2] = decToBcd(time.hours);
    }
    else
    {
        uint8_t hour12;
        bool pm;
        if (time.hours == 0)
        {
            hour12 = 12;
            pm = false;
        }
        else if (time.hours < 12)
        {
            hour12 = time.hours;
            pm = false;
        }
        else if (time.hours == 12)
        {
            hour12 = 12;
            pm = true;
        }
        else
        {
            hour12 = time.hours - 12;
            pm = true;
        }
        regs[2] = DS1307_12_24 | (pm ? DS1307_AM_PM : 0) | decToBcd(hour12);
    }

    regs[3] = decToBcd(time.dayOfWeek);

    regs[4] = decToBcd(time.date);

    regs[5] = decToBcd(time.month);

    regs[6] = decToBcd(time.year);

    return writeRegisters(DS1307_REG_SECONDS, regs, 7);
}

ds1307_time_t CDS1307::getTime()
{
    ds1307_time_t t = {};
    t.valid = false;

    uint8_t regs[7] = {};
    if (!readRegisters(DS1307_REG_SECONDS, regs, 7))
    {
        ESP_LOGE(TAG, "TIME READ ERROR!");
        return t;
    }

    uint8_t secRaw = regs[0];
    t.seconds = bcdToDec(secRaw & DS1307_SEC_MASK);

    t.minutes = bcdToDec(regs[1]);

    uint8_t hourRaw = regs[2];
    if (hourRaw & DS1307_12_24)
    {
        // 12satno
        t.is12h = true;
        t.isPM = (hourRaw & DS1307_AM_PM) != 0;
        uint8_t hour12 = bcdToDec(hourRaw & (DS1307_12_10H_MASK | DS1307_HOUR_MASK));
        if (hour12 == 0)
            hour12 = 12;
        // spremamo samo 24satno
        if (t.isPM)
        {
            t.hours = (hour12 == 12) ? 12 : hour12 + 12;
        }
        else
        {
            t.hours = (hour12 == 12) ? 0 : hour12;
        }
    }
    else
    {
        // 24satno
        t.is12h = false;
        t.isPM = false;
        t.hours = bcdToDec(hourRaw & (DS1307_24_10H_MASK | DS1307_HOUR_MASK));
    }

    t.dayOfWeek = bcdToDec(regs[3] & 0x07);

    t.date = bcdToDec(regs[4] & 0x3F);

    t.month = bcdToDec(regs[5] & 0x1F);

    t.year = bcdToDec(regs[6]);

    if (t.seconds > 59)
        t.seconds = 0;
    if (t.minutes > 59)
        t.minutes = 0;
    if (t.hours > 23)
        t.hours = 0;
    if (t.dayOfWeek < 1 || t.dayOfWeek > 7)
        t.dayOfWeek = 1;
    if (t.date < 1 || t.date > 31)
        t.date = 1;
    if (t.month < 1 || t.month > 12)
        t.month = 1;

    t.valid = true;
    return t;
}

bool CDS1307::setClockHalt(bool halt)
{

    if (!writeRegister(DS1307_REG_SECONDS, halt ? DS1307_CH : ~DS1307_CH))
    {
        ESP_LOGE(TAG, "CH WRITE ERROR");
        return false;
    }

    ESP_LOGI(TAG, "Clock %s", halt ? "HALT" : "GO");
    return true;
}

bool CDS1307::isClockHalted()
{
    uint8_t secReg = 0;
    if (!readRegister(DS1307_REG_SECONDS, secReg))
    {
        ESP_LOGE(TAG, "CH READ ERROR");
        return true; // računamo da ne radi
    }
    return (secReg & DS1307_CH) != 0;
}

bool CDS1307::set24HourMode(bool mode24)
{
    ds1307_time_t t = getTime();
    if (!t.valid)
    {
        ESP_LOGE(TAG, "TIME READ ERROR");
        return false;
    }

    uint8_t hourReg = 0;
    if (!readRegister(DS1307_REG_HOURS, hourReg))
    {
        ESP_LOGE(TAG, "HOUR READ ERROR");
        return false;
    }

    if (mode24)
    {
        hourReg &= ~DS1307_12_24;                       // stavi samo 12/24 bit na 0
        hourReg = (hourReg & 0x80) | decToBcd(t.hours); // bit 7 ostaje
    }
    else
    {
        hourReg |= DS1307_12_24; // stavi samo 12/24 bit na 1
        uint8_t hour12;
        bool pm;
        uint8_t h = t.hours;
        if (h == 0)
        {
            hour12 = 12;
            pm = false;
        }
        else if (h < 12)
        {
            hour12 = h;
            pm = false;
        }
        else if (h == 12)
        {
            hour12 = 12;
            pm = true;
        }
        else
        {
            hour12 = h - 12;
            pm = true;
        }

        hourReg = (hourReg & 0x80) | DS1307_12_24 | (pm ? DS1307_AM_PM : 0) | decToBcd(hour12);
    }

    if (!writeRegister(DS1307_REG_HOURS, hourReg))
    {
        ESP_LOGE(TAG, "TIME WRITE ERROR");
        return false;
    }

    ESP_LOGI(TAG, "%s VRIJEME!", mode24 ? "24SATNO" : "12SATNO");
    return true;
}

// SQW

bool CDS1307::setSquareWave(bool enable, uint8_t frequency) {
    uint8_t control = 0;

    if (!readRegister(DS1307_REG_CONTROL, control)) {
        ESP_LOGE(TAG, "SQW READ ERROR");
        return false;
    }

    control &= DS1307_OUT; // čuvaj samo OUT

    if (enable) {
        control |= DS1307_SQWE;
        control |= (frequency & 0x03);
    }

    if (!writeRegister(DS1307_REG_CONTROL, control)) {
        ESP_LOGE(TAG, "SQW WRITE ERROR");
        return false;
    }

    if (enable) {
        const char* freqStr = "?";
        switch (frequency & 0x03) {
            case DS1307_SQW_1HZ:     freqStr = "1 Hz";       break;
            case DS1307_SQW_4096HZ:  freqStr = "4.096 kHz";  break;
            case DS1307_SQW_8192HZ:  freqStr = "8.192 kHz";  break;
            case DS1307_SQW_32768HZ: freqStr = "32.768 kHz"; break;
        }
        ESP_LOGI(TAG, "SQW ENABLE @ %s", freqStr);
    } else {
        ESP_LOGI(TAG, "SQW DISABLE");
    }
    return true;
}

bool CDS1307::setOutputLevel(bool high) {
    uint8_t control = 0;

    if (!readRegister(DS1307_REG_CONTROL, control)) {
        ESP_LOGE(TAG, "SQW READ ERROR");
        return false;
    }

    control &= ~DS1307_SQWE; // SQW = 0
    if (high) {
        control |= DS1307_OUT; // OUT = 1
    } else {
        control &= ~DS1307_OUT; // OUT = 0
    }

    if (!writeRegister(DS1307_REG_CONTROL, control)) {
        ESP_LOGE(TAG, "SQW WRITE ERROR");
        return false;
    }

    ESP_LOGI(TAG, "SQW/OUT = %u", high);
    return true;
}

//SRAM

bool CDS1307::writeSRAM(uint8_t address, uint8_t value) {
    if (address >= DS1307_SRAM_SIZE) {
        ESP_LOGE(TAG, "SRAM WRITE OOB");
        return false;
    }
    return writeRegister(DS1307_REG_SRAM_START + address, value);
}

bool CDS1307::readSRAM(uint8_t address, uint8_t& value) {
    if (address >= DS1307_SRAM_SIZE) {
        ESP_LOGE(TAG, "SRAM READ OOB");
        return false;
    }
    return readRegister(DS1307_REG_SRAM_START + address, value);
}

bool CDS1307::writeSRAMBlock(uint8_t address, const uint8_t* buffer, size_t len) {
    if (address + len > DS1307_SRAM_SIZE) {
        ESP_LOGE(TAG, "SRAM WRITE OOB");
        return false;
    }
    return writeRegisters(DS1307_REG_SRAM_START + address, buffer, len);
}

bool CDS1307::readSRAMBlock(uint8_t address, uint8_t* buffer, size_t len) {
    if (address + len > DS1307_SRAM_SIZE) {
        ESP_LOGE(TAG, "SRAM READ OOB");
        return false;
    }
    return readRegisters(DS1307_REG_SRAM_START + address, buffer, len);
}

bool CDS1307::isConnected() {
    if (!m_busHandle) {
        return false;
    }
    esp_err_t err = i2c_master_probe(m_busHandle, DS1307_I2C_ADDR, I2C_TIMEOUT_MS * 2);
    return (err == ESP_OK);
}

bool CDS1307::isInitialised() const {
    return m_initialised && m_busHandle && m_devHandle;
}