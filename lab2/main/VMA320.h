// VMA320.h

#ifndef _VMA320_h
#define _VMA320_h

#define VCC 3.3f
#define Rkonst 10000
#define A 0.00014335859403649106
#define B 0.0003894856722794868
#define C -5.632548461886459e-07
#define KELVIN_ZERO 273.15f

typedef struct {
    float temperature;
} vma320_reading_t;

class VMA320{
    
    public:
        VMA320(int channel);
        vma320_reading_t getReading();
    
    private:
        adc_channel_t m_channel;
        const char *LogName = "VMA320";
};
#endif