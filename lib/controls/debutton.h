#ifndef _DEBUTTON_H
#define _DEBUTTON_H

#include <Arduino.h>

struct DebouncedButton
{
    bool lastState;
    unsigned long lastTime;
    unsigned long debounceDelay;
    uint8_t pin;
    bool pressed = false;

    DebouncedButton(uint8_t pin, int debounceDelay)
    {
        this->pin = pin;
        this->debounceDelay = debounceDelay;
        this->lastState = digitalRead(pin);
        this->lastTime = millis();
    }

    void update()
    {
        int currentState = digitalRead(pin);
        if (currentState != this->lastState)
        {
            if (millis() - lastTime > debounceDelay)
            {
                if (lastState == LOW && currentState == HIGH)
                {
                    pressed = true;
                }
                lastState = currentState;
                lastTime = millis();
            }
        }
    }

    bool wasPressed()
    {
        bool result = pressed;
        pressed = false;
        return result;
    }
};

#endif