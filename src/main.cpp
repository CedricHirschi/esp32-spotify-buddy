#define CORE_DEBUG_LEVEL 2 // 0: None, 1: Error, 2: Warning, 3: Info, 4: Debug, 5: Verbose
// #define ARDUHAL_LOG_LEVEL CORE_DEBUG_LEVEL
#define CONFIG_ARDUHAL_LOG_COLORS 1

#include <Arduino.h>

#include <WiFiManager.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include <U8g2lib.h>
#include <SPI.h>

#include "index.h"
#include "spotconn.h"
#include "debutton.h"
#include "analogfilter.h"
#include "nvshandler.h"

#define ESP_DRD_USE_SPIFFS true
#define DOUBLERESETDETECTOR_DEBUG true

#include <ESP_DoubleResetDetector.h>

#define DRD_TIMEOUT 7
#define DRD_ADDRESS 0

#define EN 0
#define DE 1
#define FR 2
#define IT 3
#define ES 4
#define PT 5
#define NL 6

#include "config.h"

#define STACK_SIZE_DISPLAY 2000
#define STACK_SIZE_REFRESH_SONG 5500
#define STACK_SIZE_LOOP 8192

#define CORE_TASK_DISPLAY 0
#define CORE_TASK_SERVE 0
#define CORE_TASK_REFRESH_SONG 1
#define CORE_TASK_LOOP 1

const char spotifyRedirectURL[100] = "http://%s/callback";
char spotifyRedirectURLFormatted[100];

U8G2_DISPLAY
WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_S);
DebouncedButton backwardButton(PIN_BACKWARD_BUTTON, BUTTON_DEBOUNCE_TIME);
DebouncedButton playButton(PIN_PLAY_BUTTON, BUTTON_DEBOUNCE_TIME);
DebouncedButton forwardButton(PIN_FORWARD_BUTTON, BUTTON_DEBOUNCE_TIME);
SpotConn spotconn;
WiFiManager wifiManager;

#if HAS_VOLUME_POTENTIOMETER
FilteredAnalog volumePot(PIN_VOLUME_POTENTIOMETER, POTENTIOMETER_FILTER_COEFFICIENT);
#endif

#if CLEAR_ON_DOUBLE_RESET
DoubleResetDetector *drd;
unsigned long startMillis;
#endif

#if WEEKDAYS_LANGUAGE == EN
char daysOfTheWeek[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
#elif WEEKDAYS_LANGUAGE == DE
char daysOfTheWeek[7][12] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
#elif WEEKDAYS_LANGUAGE == FR
char daysOfTheWeek[7][10] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};
#elif WEEKDAYS_LANGUAGE == IT
char daysOfTheWeek[7][10] = {"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato"};
#elif WEEKDAYS_LANGUAGE == ES
char daysOfTheWeek[7][10] = {"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};
#elif WEEKDAYS_LANGUAGE == PT
char daysOfTheWeek[7][10] = {"Domingo", "Segunda-feira", "Terça-feira", "Quarta-feira", "Quinta-feira", "Sexta-feira", "Sábado"};
#elif WEEKDAYS_LANGUAGE == NL
char daysOfTheWeek[7][10] = {"Zondag", "Maandag", "Dinsdag", "Woensdag", "Donderdag", "Vrijdag", "Zaterdag"};
#else
#error "WEEKDAYS_LANGUAGE not set correctly. See config.h for more information."
#endif

typedef enum currentStatus_e
{
    NONE,
    INIT_WIFI,
    DONE_WIFI,
    INIT_SERVER,
    DONE_SERVER,
    INIT_PINS,
    DONE_PINS,
    INIT_CREDENTIALS,
    DONE_CREDENTIALS,
    INIT_AUTHORIZATION,
    DONE_AUTHORIZATION,
    INIT_SONG_REFRESH,
    DONE_SONG_REFRESH,
    INIT_BUTTONS,
    DONE_BUTTONS,
    DONE_SETUP,
    NO_SONG,
    PLAYING_SONG,
    NOT_AUTHORIZED,
    ERROR
} currentStatus_t;

char statusStrings[18][20] = {
    "NONE",
    "INIT_WIFI",
    "DONE_WIFI",
    "INIT_SERVER",
    "DONE_SERVER",
    "INIT_PINS",
    "DONE_PINS",
    "INIT_AUTH_REFRESH",
    "DONE_AUTH_REFRESH",
    "INIT_SONG_REFRESH",
    "DONE_SONG_REFRESH",
    "INIT_BUTTONS",
    "DONE_BUTTONS",
    "DONE_SETUP",
    "NO_SONG",
    "PLAYING_SONG",
    "NOT_AUTHORIZED"};

currentStatus_t currentStatus = NONE;
String currentError;

WiFiManagerParameter clientIDParameter("clientID", "Spotify clientID", "", 32);
WiFiManagerParameter clientSecretParameter("clientSecret", "Spotify clientSecret", "", 32);

const char *savedClientID;
const char *savedClientSecret;

SemaphoreHandle_t httpMutex = xSemaphoreCreateMutex();

TaskHandle_t taskDisplayHandle = NULL;
TaskHandle_t taskServeHandle = NULL;
TaskHandle_t taskRefreshSongHandle = NULL;

void displayClock(bool withIp = false)
{
    timeClient.update();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    char *day = daysOfTheWeek[timeClient.getDay()];
    u8g2.setCursor((u8g2.getWidth() - u8g2.getStrWidth(day)) / 2, 10);
    u8g2.println(day);
    u8g2.setFont(u8g2_font_inb19_mf);
    String time = timeClient.getFormattedTime();
    if (withIp)
        u8g2.setCursor((u8g2.getWidth() - u8g2.getStrWidth(time.c_str())) / 2, 40);
    else
        u8g2.setCursor((u8g2.getWidth() - u8g2.getStrWidth(time.c_str())) / 2, 45);
    u8g2.println(time);
}

void displayIP()
{
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor((u8g2.getWidth() - u8g2.getStrWidth(WiFi.localIP().toString().c_str())) / 2, 60);
    u8g2.println(WiFi.localIP());
}

String previousSongId = "";
String prevoiusSongName = "";
int advanceIndex = 0;
int remainingWidth = 0;
int8_t direction = 1;
unsigned long pauseRemaining = 0;

String formatTime(uint ms)
{
    String result = String(ms / 60000);

    result += ":";
    if (ms % 60000 < 10000)
        result += "0";
    result += String(ms % 60000 / 1000);

    return result;
}

void displaySong()
{
    u8g2.setFont(u8g2_font_ncenB08_tr);

    if (previousSongId != spotconn.currentSong.Id)
    {
        previousSongId = spotconn.currentSong.Id;
        prevoiusSongName = spotconn.currentSong.song;
        remainingWidth = (u8g2.getStrWidth(prevoiusSongName.c_str()) - u8g2.getWidth()) + 5;
        advanceIndex = 0;
        direction = 1;
        pauseRemaining = 0;
        // int charWidth = u8g2.getStrWidth("W");
        // log_e("Width too long: %d", remainingWidth);
        // log_e("Width of char: %d", charWidth);
        // remainingWidth = remainingWidth / charWidth + 5;
        // log_e("Chars too long: %d", remainingWidth);
    }

    // log_d("Displaying song info");

    u8g2.clearBuffer();
    // log_d("Cleared buffer");
    // splitString(spotconn.currentSong.song, lines, lineCount, u8g2);
    // // log_d("Split song");
    // for (int i = 0; i < lineCount; i++)
    // {
    //     u8g2.setCursor(0, 10 + (i * 10));
    //     // log_d(" - " + lines[i]);
    //     u8g2.println(lines[i]);
    // }

    u8g2.drawStr(-advanceIndex, 10, prevoiusSongName.c_str());
    if (pauseRemaining > 0)
    {
        pauseRemaining -= 1000 / DISPLAY_REFRESH_RATE;
    }
    else if (remainingWidth > 0)
    {
        advanceIndex += direction;

        if (advanceIndex >= remainingWidth)
        {
            advanceIndex = remainingWidth;
            direction = -5;
            pauseRemaining = 1000;
            // log_e("Resetting song name");
        }
        if (advanceIndex < 0)
        {
            advanceIndex = 0;
            direction = 1;
            pauseRemaining = 1000;
            // log_e("Resetting song name");
        }
    }

    // log_d("Printed song");
    u8g2.setFont(u8g2_font_PixelTheatre_te);
    // splitString(spotconn.currentSong.artist, lines, lineCount, u8g2);
    // // log_d("Split artist");
    // for (int i = 0; i < lineCount; i++)
    // {
    //     u8g2.setCursor(0, 35 + (i * 10));
    //     // log_d(" - " + lines[i]);
    //     u8g2.println(lines[i]);
    // }
    u8g2.drawStr(0, 21, spotconn.currentSong.artist.c_str());
    // log_d("Printed artist");
    // splitString(spotconn.currentSong.album, lines, lineCount, u8g2);
    // // log_d("Split album");
    // for (int i = 0; i < lineCount; i++)
    // {
    //     u8g2.setCursor(0, 45 + (i * 10));
    //     // log_d(" - " + lines[i]);
    //     u8g2.println(lines[i]);
    // }
    u8g2.drawStr(0, 31, spotconn.currentSong.album.c_str());
    // log_d("Printed album");
    if (spotconn.activeDevice.name != "")
    {
        u8g2.setCursor(0, 43);
        u8g2.printf("On '%s'", spotconn.activeDevice.name.c_str());
    }

    // print current time (left aligned) and duration (right aligned)
    u8g2.setCursor(0, 55);
    u8g2.printf("%s", formatTime(spotconn.currentSongPositionMs).c_str());
    u8g2.setCursor(u8g2.getWidth() - u8g2.getStrWidth(formatTime(spotconn.currentSong.durationMs).c_str()), 55);
    u8g2.printf("%s", formatTime(spotconn.currentSong.durationMs).c_str());

    // draw progress bar
    float progress = spotconn.currentSongPositionMs / spotconn.currentSong.durationMs;
    int barLength = u8g2.getWidth() * progress;
    int barHeight = 4;
    u8g2.drawBox(0, u8g2.getHeight() - barHeight - 1, barLength, barHeight);

    // delete[] lines;
}

void printUsages()
{
    // print usages of all tasks
    log_d("================================================================================");

    auto displayTaskUsage = uxTaskGetStackHighWaterMark(taskDisplayHandle);
    auto serveTaskUsage = uxTaskGetStackHighWaterMark(taskServeHandle);
    auto refreshSongTaskUsage = uxTaskGetStackHighWaterMark(taskRefreshSongHandle);
    auto loopTaskUsage = uxTaskGetStackHighWaterMark(NULL);

    log_d("Display task usage:      [%.2f%%]", (float)(STACK_SIZE_DISPLAY - displayTaskUsage) / STACK_SIZE_DISPLAY * 100);
    log_d("Refresh song task usage: [%.2f%%]", (float)(STACK_SIZE_REFRESH_SONG - refreshSongTaskUsage) / STACK_SIZE_REFRESH_SONG * 100);
    log_d("Loop task usage:         [%.2f%%]", (float)(STACK_SIZE_LOOP - loopTaskUsage) / STACK_SIZE_LOOP * 100);

    log_d("--------------------------------------------------------------------------------");

    // print heap and psram
    auto freePSRAM = ESP.getFreePsram();
    auto maxPSRAM = ESP.getPsramSize();
    auto freeHeap = ESP.getFreeHeap();
    auto maxHeap = ESP.getHeapSize();

    auto freeFlash = ESP.getFreeSketchSpace();
    auto maxFlash = ESP.getSketchSize();

    // print used space in percent
    if (maxPSRAM > 0)
        log_d("Used PSRAM:  %.2f%%", (float)(maxPSRAM - freePSRAM) / maxPSRAM * 100);
    log_d("Used Heap:   %.2f%%", (float)(maxHeap - freeHeap) / maxHeap * 100);
    log_d("Loop running on core %d", xPortGetCoreID());

    log_d("================================================================================");
}

void taskDisplay(void *pvParameters)
{
    u8g2.begin();
    u8g2.setContrast(0); // reduce brightness
    u8g2.setFont(u8g2_font_ncenB08_tr);

    while (true)
    {
        unsigned long displayLoopStartTime = millis();
        u8g2.clearBuffer();

        switch (currentStatus)
        {
        case NONE:
        case DONE_WIFI:
        case INIT_SERVER:
        case DONE_SERVER:
        case INIT_PINS:
        case DONE_PINS:
        case DONE_CREDENTIALS:
        case DONE_AUTHORIZATION:
        case INIT_SONG_REFRESH:
        case DONE_SONG_REFRESH:
        case INIT_BUTTONS:
        case DONE_BUTTONS:
            u8g2.drawStr(0, 10, "Setup");
            u8g2.drawStr(0, 20, "Status:");
            u8g2.drawStr(0, 30, statusStrings[currentStatus]);
            break;
        case INIT_CREDENTIALS:
            u8g2.setCursor(0, 10);
            u8g2.print("Go to");
            u8g2.setCursor(0, 20);
            u8g2.printf("http://%s", WiFi.localIP().toString().c_str());
            u8g2.setCursor(0, 30);
            u8g2.printf("in your browser and");
            u8g2.setCursor(0, 40);
            u8g2.printf("enter your Spotify");
            u8g2.setCursor(0, 50);
            u8g2.printf("credentials");
            break;
        case INIT_AUTHORIZATION:
            u8g2.drawStr(0, 10, "Go to");
            u8g2.drawStr(0, 20, ("http://" + WiFi.localIP().toString()).c_str());
            u8g2.drawStr(0, 30, "in your browser");
            u8g2.drawStr(0, 40, "to authorize Spotify");
            break;
        case INIT_WIFI:
            u8g2.drawStr(0, 10, "Connect to WiFi");
            u8g2.drawStr(0, 20, "'Spotify Connect Setup'");
            u8g2.drawStr(0, 30, "and go to");
            u8g2.drawStr(0, 40, "http://192.168.4.1");
            u8g2.drawStr(0, 50, "to set up WiFi");
            u8g2.drawStr(0, 60, "and Spotify");
            break;
        case DONE_SETUP:
            u8g2.drawStr(0, 10, "Setup");
            u8g2.drawStr(0, 20, "Done");
            break;
        case NO_SONG:
            displayClock(false);
            break;
        case PLAYING_SONG:
            displaySong();
            break;
        case NOT_AUTHORIZED:
            displayClock(true);
            displayIP();
            break;
        case ERROR:
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.drawStr(0, 10, "Error");
            u8g2.drawStr(0, 20, currentError.c_str());
            break;
        }

        u8g2.sendBuffer();
        unsigned long displayLoopEndTime = millis();
        if (displayLoopEndTime - displayLoopStartTime < 1000 / DISPLAY_REFRESH_RATE)
            vTaskDelay((1000 / DISPLAY_REFRESH_RATE - (displayLoopEndTime - displayLoopStartTime)) / portTICK_PERIOD_MS);
    }
}

void configStopCallback()
{
    auto enteredClientID = clientIDParameter.getValue();
    auto enteredClientSecret = clientSecretParameter.getValue();

    log_d("Entered clientID: %s\n", enteredClientID);
    log_d("Entered clientSecret: %s\n", enteredClientSecret);

    // check if clientID and clientSecret are valid
    // have to be of length 32 and hex characters
    if (strlen(enteredClientID) != 32)
    {
        log_e("ClientID should be of length 32");
        return;
    }
    if (strlen(enteredClientSecret) != 32)
    {
        log_e("ClientSecret should be of length 32");
        return;
    }

    for (int i = 0; i < 32; i++)
    {
        if (!isxdigit(enteredClientID[i]))
        {
            log_e("ClientID should only contain hex characters");
            return;
        }
        if (!isxdigit(enteredClientSecret[i]))
        {
            log_e("ClientSecret should only contain hex characters");
            return;
        }
    }

    NVSHandler nvsHandler;
    nvsHandler.set("clientID", enteredClientID);
    nvsHandler.set("clientSecret", enteredClientSecret);

    log_d("Saved clientID and clientSecret");
    log_d("Restarting ESP, please refresh the page after approx 10 seconds");

    if (wifiManager.getWiFiIsSaved())
    {
        ESP.restart();
        delay(1000);
    }
}

void handlePageRoot()
{
    char page[600];
    sprintf(page, mainPage, savedClientID, spotifyRedirectURLFormatted);
    // log_e("redirect uri: %s", redirect_uri);
    server.send(200, "text/html", String(page));
}

void handlePageCallback()
{
    if (!spotconn.accessTokenSet)
    {
        if (server.arg("code") == "")
        { // Parameter not found
            log_e("No code found");
            char page[600];
            sprintf(page, errorPage, savedClientID, spotifyRedirectURLFormatted);
            // log_e("redirect uri: %s", redirect_uri);
            server.send(200, "text/html", String(page)); // Send web page
        }
        else
        { // Parameter found
            log_d("Code found");
            spotconn.setClientCredentials(savedClientID, savedClientSecret);
            if (spotconn.getAuth(server.arg("code")))
            {
                server.send(200, "text/html", "Spotify connected successfully <br>Authentication will be refreshed in: " + String(spotconn.tokenExpireTime / 60) + " minutes");
            }
            else
            {
                char page[600];
                sprintf(page, errorPage, savedClientID, spotifyRedirectURLFormatted);
                server.send(200, "text/html", String(page)); // Send web page
            }
        }
    }
    else
    {
        server.send(200, "text/html", "Spotify setup complete");
    }
}

void taskRefreshSong(void *pvParameters)
{
    while (true)
    {
        if (spotconn.accessTokenSet)
        {
            log_d("Getting track info");

            bool status = false;

            if (xSemaphoreTake(httpMutex, 1000))
            {
                status = spotconn.getTrackInfo();
                xSemaphoreGive(httpMutex);
            }

            if (status)
            {
                currentStatus = PLAYING_SONG;

                if (spotconn.currentSong.song == "")
                {
                    currentStatus = NO_SONG;
                    log_d("No song playing");
                }
                else
                {
                    log_d("Current song:");
                    spotconn.currentSong.print();
                }

                // log_d(spotconn.currentSong.song + " by " + spotconn.currentSong.artist);
                // log_d(spotconn.currentSong.album + " - [" + spotconn.currentSongPositionMs + "/" + spotconn.currentSong.durationMs + "] ms");
            }
            else
            {
                log_e("Failed to get track info");
                currentStatus = ERROR;
                currentError = "Failed to get track info";
            }

            log_d("Getting device info");

            if (xSemaphoreTake(httpMutex, 1000))
            {
                status = spotconn.getDeviceInfo();
                xSemaphoreGive(httpMutex);
            }

            if (status)
            {
                log_d("Current device:");
                spotconn.activeDevice.print();
            }
            else
            {
                log_e("Failed to get device info");
                currentStatus = ERROR;
                currentError = "Failed to get device info";
            }
        }

        vTaskDelay(SONG_REFRESH_TIME / portTICK_PERIOD_MS);
    }
}

void IRAM_ATTR isrBackwardButton()
{
    backwardButton.update();
}

void IRAM_ATTR isrPlayButton()
{
    playButton.update();
}

void IRAM_ATTR isrForwardButton()
{
    forwardButton.update();
}

unsigned long tokenRefreshMillis = 0;

void setup()
{
#if CLEAR_ON_DOUBLE_RESET
    drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
    pinMode(5, OUTPUT);
    digitalWrite(5, LOW);
    startMillis = millis();
    if (drd->detectDoubleReset())
    {
        log_d("Double reset detected");

        NVSHandler nvsHandler;

        nvsHandler.eraseAll();
        nvsHandler.set("accessToken", "null");
        nvsHandler.set("refreshToken", "null");
        wifiManager.resetSettings();

        log_d("Erased all saved data");
    }
#endif

    log_d("Setting up display task");
    xTaskCreatePinnedToCore(
        taskDisplay,        /* Task function. */
        "TaskDisplay",      /* String with name of task. */
        STACK_SIZE_DISPLAY, /* Stack size in bytes. */
        NULL,               /* Parameter passed as input of the task */
        2,                  /* Priority of the task. */
        &taskDisplayHandle,
        CORE_TASK_DISPLAY);
    log_d("Set up display task");

    log_d("Connecting to WiFi");
    currentStatus = INIT_WIFI;
    wifiManager.setDebugOutput(false);
    wifiManager.setCaptivePortalEnable(true);
    wifiManager.addParameter(&clientIDParameter);
    wifiManager.addParameter(&clientSecretParameter);
    wifiManager.setSaveParamsCallback(configStopCallback);
    wifiManager.setSaveConfigCallback(configStopCallback);
    if (!wifiManager.autoConnect("Spotify Connect Setup"))
    {
        log_e("Failed to connect to WiFi, restarting (hit timeout)");
        ESP.restart();
        delay(1000);
    }
    // log_d(WiFi.localIP());
    log_d("Connected with IP: %s", WiFi.localIP().toString().c_str());
    currentStatus = DONE_WIFI;

    sprintf(spotifyRedirectURLFormatted, spotifyRedirectURL, WiFi.localIP().toString().c_str());

    log_d("Getting credentials");
    currentStatus = INIT_CREDENTIALS;
    NVSHandler nvsHandler;
    savedClientID = nvsHandler.get("clientID");
    savedClientSecret = nvsHandler.get("clientSecret");
    auto savedRefreshToken = nvsHandler.get("refreshToken");
    auto savedAccessToken = nvsHandler.get("accessToken");

    if (savedClientID == NULL || savedClientSecret == NULL)
    {
        log_e("Please enter clientID and clientSecret in the captive portal");
        wifiManager.startConfigPortal("Spotify Buddy Setup");
    }

    savedClientID = nvsHandler.get("clientID");
    savedClientSecret = nvsHandler.get("clientSecret");

    log_d("Done getting credentials");
    currentStatus = DONE_CREDENTIALS;

    log_d("Setting up authorization");
    currentStatus = INIT_AUTHORIZATION;

    if (savedAccessToken != NULL && savedRefreshToken != NULL && strcmp(savedAccessToken, "null") != 0 && strcmp(savedRefreshToken, "null") != 0)
    {
        log_d("Found saved access and refresh tokens:");
        log_d("%s / %s\n", savedAccessToken, savedRefreshToken);
        log_d("Using client credentials:");
        log_d("%s / %s\n", savedClientID, savedClientSecret);
        spotconn.setTokens(savedAccessToken, savedRefreshToken);
        spotconn.setClientCredentials(savedClientID, savedClientSecret);
        spotconn.accessTokenSet = true;
        log_d("Refreshing auth");
        spotconn.refreshAuth();
        log_d("Auth refreshed");
        tokenRefreshMillis = millis();
    }
    else
    {
        log_d("Starting auth server");

        server.on("/", handlePageRoot);
        server.on("/wifisave", handlePageRoot);
        server.on("/callback", handlePageCallback);
        server.begin();

        log_d("Go to http://%s in your browser (or refresh if already there)\nTo verify your spotify account.", WiFi.localIP().toString().c_str());

        while (!spotconn.accessTokenSet)
        {
            server.handleClient();
        }

        log_d("Stopping auth server");

        server.stop();

        NVSHandler nvsHandler;
        nvsHandler.set("accessToken", spotconn.accessToken.c_str());
        nvsHandler.set("refreshToken", spotconn.refreshToken.c_str());

        log_d("Saved access and refresh tokens:");
        log_d("%s / %s\n", spotconn.accessToken.c_str(), spotconn.refreshToken.c_str());
    }

    log_d("Done authorizing");
    currentStatus = DONE_AUTHORIZATION;

    log_d("Setting up pins");
    currentStatus = INIT_PINS;
    pinMode(PIN_BACKWARD_BUTTON, INPUT_PULLUP);
    pinMode(PIN_PLAY_BUTTON, INPUT_PULLUP);
    pinMode(PIN_FORWARD_BUTTON, INPUT_PULLUP);
    // pinMode(PIN_VOLUME_POTENTIOMETER, INPUT);

    attachInterrupt(PIN_BACKWARD_BUTTON, isrBackwardButton, CHANGE);
    attachInterrupt(PIN_PLAY_BUTTON, isrPlayButton, CHANGE);
    attachInterrupt(PIN_FORWARD_BUTTON, isrForwardButton, CHANGE);
    log_d("Set up pins");
    currentStatus = DONE_PINS;

    log_d("Setting up refresh song task");
    currentStatus = INIT_SONG_REFRESH;
    xTaskCreatePinnedToCore(
        taskRefreshSong,         /* Task function. */
        "TaskRefreshSong",       /* String with name of task. */
        STACK_SIZE_REFRESH_SONG, /* Stack size in bytes. */
        NULL,                    /* Parameter passed as input of the task */
        3,                       /* Priority of the task. */
        &taskRefreshSongHandle,
        CORE_TASK_REFRESH_SONG);
    log_d("Set up refresh song task");
    currentStatus = DONE_SONG_REFRESH;

    log_d("Setup done");
    currentStatus = DONE_SETUP;
}

unsigned long lastUsagePrint = 0;

void loop()
{
#if CLEAR_ON_DOUBLE_RESET
    if (millis() - startMillis > DRD_TIMEOUT * 1000)
    {
        digitalWrite(5, HIGH);
    }

    drd->loop();
#endif

    if (spotconn.accessTokenSet)
    {
        if (backwardButton.wasPressed())
        {
            log_d("backward button pressed");
            if (xSemaphoreTake(httpMutex, 5000))
            {
                spotconn.skipBack();
                xSemaphoreGive(httpMutex);
            }
        }
        else if (playButton.wasPressed())
        {
            log_d("play button pressed");
            if (xSemaphoreTake(httpMutex, 5000))
            {
                spotconn.togglePlay();
                xSemaphoreGive(httpMutex);
            }
        }
        else if (forwardButton.wasPressed())
        {
            log_d("forward button pressed");
            if (xSemaphoreTake(httpMutex, 5000))
            {
                spotconn.skipForward();
                xSemaphoreGive(httpMutex);
            }
        }

#if HAS_VOLUME_POTENTIOMETER
        int requestedVolume = map(volumePot.read(), 0, 4095, 0, 100);

        if (abs(requestedVolume - spotconn.currVol) > 5)
        {
            log_d("adjusting volume from %d to %d", spotconn.currVol, requestedVolume);
            if (xSemaphoreTake(httpMutex, 1000))
            {
                spotconn.adjustVolume(requestedVolume);
                xSemaphoreGive(httpMutex);
            }
        }
#endif

        if ((millis() - spotconn.tokenStartTime) / 1000 >= spotconn.tokenExpireTime - 1) // refresh token 1 second before it expires (just to be sure)
        {
            log_d("refreshing token");

            bool result = false;

            if (xSemaphoreTake(httpMutex, 1000))
            {
                result = spotconn.refreshAuth();
                xSemaphoreGive(httpMutex);
            }

            if (result)
            {
                log_d("refreshed token");
            }
            else
            {
                log_d("failed to refresh token");
                currentStatus = ERROR;
                currentError = "Failed to refresh token";
            }
        }

        // if (millis() - lastUsagePrint > 5000)
        // {
        //     printUsages();
        //     lastUsagePrint = millis();
        // }
    }
    else
    {
        currentStatus = NOT_AUTHORIZED;
    }

    vTaskDelay(100);
}
