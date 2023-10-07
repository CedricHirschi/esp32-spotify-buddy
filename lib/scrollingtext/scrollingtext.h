#ifndef SCROLLINGTEXT_H
#define SCROLLINGTEXT_H

#include <Arduino.h>
#include <U8g2lib.h>

class ScrollingText
{
public:
    ScrollingText(U8G2 &display, int yPosition, unsigned displayRefreshRate, int pauseDuration = 1000, int scrollSpeed = 1);

    void update(const String &newText);

private:
    U8G2 &display;
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