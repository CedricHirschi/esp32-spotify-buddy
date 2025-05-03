#ifndef SCROLLINGTEXT_H
#define SCROLLINGTEXT_H

#include <Arduino.h>
#include <Adafruit_HX8357.h>

class ScrollingText
{
public:
    ScrollingText(Adafruit_HX8357 &display, int yPosition, unsigned displayRefreshRate, int pauseDuration = 1000, int scrollSpeed = 10);

    void update(const String &newText);

private:
    Adafruit_HX8357 &display;
    String currentText = "";
    int yPosition;
    unsigned displayRefreshRate;
    int pauseDuration;
    int scrollSpeed;
    int advanceIndex = 0;
    int remainingWidth = 0;
    int8_t direction = 1;
    unsigned long pauseRemaining = 0;
};

#endif // SCROLLINGTEXT_H