 #include "temperatura.h"

float read_temperature() {
    const float conversion_factor = 3.3f / (1 << 12);
    adc_select_input(4); // canal do sensor interno
    uint16_t raw = adc_read();
    float voltage = raw * conversion_factor;
    

    return ( (27.0f - (voltage - 0.706f) / 0.001721f) - 32) * 5/9;
}
