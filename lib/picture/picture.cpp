#include "picture.hpp"

Adafruit_HX8357 *currentDisplay = nullptr;

#define PATH(directory, filename) ((strlen(directory.c_str()) != 0) ? ("/" + directory + "/" + filename) : ("/" + filename))

bool sketchCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *data)
{
    if (!currentDisplay)
    {
        return false;
    }
    currentDisplay->startWrite();
    currentDisplay->setAddrWindow(x, y, w, h);
    currentDisplay->writePixels(data, w * h);
    currentDisplay->endWrite();
    return true;
}

Picture::Picture(fs::FS &fs, uint8_t scale, String directory) : fs(fs), directory(directory)
{
    if (!fs.exists(directory))
    {
        fs.mkdir(directory);
    }
    TJpgDec.setJpgScale(scale);
    TJpgDec.setSwapBytes(false);
    TJpgDec.setCallback(sketchCallback);
}

bool Picture::fetch(String url, String filename, bool replace)
{
    bool result = false;
    if (replace)
    {
        remove(filename);
    }

    if (!fs.exists(PATH(directory, filename)))
    {
        HTTPClient http;
        http.begin(url);
        log_d("Fetching %s", url.c_str());
        unsigned long start = millis();
        int httpCode = http.GET();
        unsigned long elapsed = millis() - start;
        if (httpCode == HTTP_CODE_OK)
        {
            File file = fs.open(PATH(directory, filename), FILE_WRITE);
            if (file)
            {
                int size = http.writeToStream(&file);
                file.close();
                result = true;
                // Serial.println("Fetched " + String((float)size * 8.0f / 1000.0f, 3) + "kb in " + elapsed + "ms, equals to " + String((float)size * 8.0f / elapsed / 1000.0f, 3) + "Mbps");
            }
        }
        http.end();
    }

    return result;
}

void Picture::remove(String filename)
{
    fs.remove(PATH(directory, filename));
}

void Picture::list(std::function<void(String)> callback)
{
    File root = fs.open("/" + directory);
    File file = root.openNextFile();
    while (file)
    {
        callback(file.name());
        file = root.openNextFile();
    }
}

void Picture::display(Adafruit_HX8357 *display, String filename, int16_t x, int16_t y)
{
    File file = fs.open(PATH(directory, filename));
    if (file)
    {
        currentDisplay = display;
        TJpgDec.drawFsJpg(x, y, file);
        file.close();
    }
}