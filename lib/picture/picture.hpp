#pragma once

#include "FS.h"
#include "HTTPClient.h"
#include "Adafruit_HX8357.h"
#include "TJpg_Decoder.h"

class Picture
{
    fs::FS &fs;
    String directory;

public:
    Picture(fs::FS &fs, uint8_t scale, String directory = "");

    bool fetch(String url, String filename, bool replace = false);
    void remove(String filename);
    void list(std::function<void(String)> callback);

    void display(Adafruit_HX8357 *display, String filename, int16_t x, int16_t y);
};