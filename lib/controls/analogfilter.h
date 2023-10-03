#ifndef _ANALOGFILTER_H
#define _ANALOGFILTER_H

#include <Arduino.h>

class FilteredAnalog
{
    uint8_t pin;
    uint16_t value;
    float alpha;

public:
    FilteredAnalog(uint8_t pin, float alpha = 0.5) : pin(pin), alpha(alpha)
    {
        pinMode(pin, INPUT);
        value = analogRead(pin);
    }

    uint16_t read()
    {
        value = alpha * (float)analogRead(pin) + (1.0f - alpha) * (float)value;

        return value;
    }
};

#endif // _ANALOGFILTER_H