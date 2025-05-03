#include "scrollingtext.h"

ScrollingText::ScrollingText(Adafruit_HX8357 &display, int yPosition, unsigned displayRefreshRate, int pauseDuration, int scrollSpeed)
    : display(display), yPosition(yPosition), displayRefreshRate(displayRefreshRate), pauseDuration(pauseDuration), scrollSpeed(scrollSpeed) {}

void ScrollingText::update(const String &newText)
{
    static int lastAdvanceIndex = 0;
    static uint16_t h = 0;
    if (currentText != newText)
    {
        currentText = newText;
        advanceIndex = 0;
        direction = 1;
        pauseRemaining = 0;

        int16_t x, y;
        uint16_t w;
        display.getTextBounds(currentText, 0, 0, &x, &y, &w, &h);
        display.fillRect(0, yPosition, display.width(), h + 20, HX8357_BLACK);
        display.setCursor(-advanceIndex, yPosition);
        display.print(currentText);
        remainingWidth = w - display.width() + 5;
    }

    if (lastAdvanceIndex != advanceIndex)
    {
        display.fillRect(0, yPosition, display.width(), h + 20, HX8357_BLACK);
        lastAdvanceIndex = advanceIndex;
        display.setCursor(-advanceIndex, yPosition);
        display.print(currentText);
    }

    if (pauseRemaining > 0)
    {
        pauseRemaining -= 1000 / displayRefreshRate;
    }
    else if (remainingWidth > 0)
    {
        advanceIndex += direction * scrollSpeed;

        if (advanceIndex >= remainingWidth)
        {
            advanceIndex = remainingWidth;
            direction = -5;
            pauseRemaining = pauseDuration;
        }
        if (advanceIndex < 0)
        {
            advanceIndex = 0;
            direction = 1;
            pauseRemaining = pauseDuration;
        }
    }
}