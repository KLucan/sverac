// DHT22.h

#ifndef _DHT22_h
#define _DHT22_h

typedef struct {
    float humidity;
    float temperature;
    uint8_t checksum;
    bool valid;
} dht22_reading_t;

class DHT22{
    
    public:
        DHT22(int port);
        dht22_reading_t getReading();
        void printWave();
    
    private:
        gpio_num_t m_pinNumber;
        const char *LogName = "DHT22";
        void waitForLevel(uint32_t level);
        bool getBit();
};
#endif