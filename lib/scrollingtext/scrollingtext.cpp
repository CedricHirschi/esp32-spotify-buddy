#include "scrollingtext.h"

ScrollingText::ScrollingText(U8G2 &display, int yPosition, unsigned displayRefreshRate, int pauseDuration, int scrollSpeed)
    : display(display), yPosition(yPosition), displayRefreshRate(displayRefreshRate), pauseDuration(pauseDuration), scrollSpeed(scrollSpeed) {}

void ScrollingText::update(const String &newText)
{
    if (currentText != newText)
    {
        currentText = newText;
        remainingWidth = (display.getStrWidth(currentText.c_str()) - display.getWidth()) + 5;
        advanceIndex = 0;
        direction = 1;
        pauseRemaining = 0;
    }

    display.drawStr(-advanceIndex, yPosition, currentText.c_str());
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