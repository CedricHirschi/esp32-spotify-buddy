#define CORE_DEBUG_LEVEL 2 // 0: None, 1: Error, 2: Warning, 3: Info, 4: Debug, 5: Verbose
#define CONFIG_ARDUHAL_LOG_COLORS 1

#include <Arduino.h>

#include <WiFiManager.h>
#include <WebServer.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESPmDNS.h>

#include <U8g2lib.h>
#include <SPI.h>

#include "index.h"
#include "spotconn.h"
#include "debutton.h"
#include "analogfilter.h"
#include "nvshandler.h"
#include "scrollingtext.h"
#include "timeformats.h"

#include "config.h"
#include "status.h"

#define STACK_SIZE_DISPLAY 3000
#define STACK_SIZE_REFRESH_SONG 6500
#define STACK_SIZE_LOOP 8192

#define CORE_TASK_DISPLAY 0
#define CORE_TASK_SERVE 0
#define CORE_TASK_REFRESH_SONG 1
#define CORE_TASK_LOOP 1

const char spotifyRedirectURL[100] = "http://spotifybuddy.local/callback";
const char *savedClientID;
const char *savedClientSecret;

U8G2_DISPLAY
WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET_S);
DebouncedButton backwardButton(PIN_BACKWARD_BUTTON, BUTTON_DEBOUNCE_TIME);
DebouncedButton playButton(PIN_PLAY_BUTTON, BUTTON_DEBOUNCE_TIME);
DebouncedButton forwardButton(PIN_FORWARD_BUTTON, BUTTON_DEBOUNCE_TIME);
#if HAS_VOLUME_POTENTIOMETER
FilteredAnalog volumePot(PIN_VOLUME_POTENTIOMETER, POTENTIOMETER_FILTER_COEFFICIENT);
#endif
SpotConn spotconn;
WiFiManager wifiManager;

currentStatus_t currentStatus = NONE;
String currentError;

WiFiManagerParameter clientIDParameter("clientID", "Spotify clientID", "", 32);
WiFiManagerParameter clientSecretParameter("clientSecret", "Spotify clientSecret", "", 32);

SemaphoreHandle_t httpMutex = xSemaphoreCreateMutex();
TaskHandle_t taskDisplayHandle = NULL;
TaskHandle_t taskServeHandle = NULL;
TaskHandle_t taskRefreshSongHandle = NULL;

bool detect_reset()
{
    pinMode(PIN_RESET_SETTINGS, INPUT_PULLUP);
    delay(10);
    bool reset = digitalRead(PIN_RESET_SETTINGS) == LOW;
    pinMode(PIN_RESET_SETTINGS, INPUT);
    return reset;
}

void displayClock()
{
    timeClient.update();

    u8g2.setFont(u8g2_font_ncenB08_tr);
    String date = formatCurrentDate(timeClient);
    u8g2.setCursor((u8g2.getWidth() - u8g2.getStrWidth(date.c_str())) / 2, 10);
    u8g2.println(date);

    u8g2.setFont(u8g2_font_inb19_mf);
    String time = timeClient.getFormattedTime();
    u8g2.setCursor((u8g2.getWidth() - u8g2.getStrWidth(time.c_str())) / 2, 45);
    u8g2.println(time);
}

ScrollingText songName(u8g2, 10, DISPLAY_REFRESH_RATE);
ScrollingText artistName(u8g2, 21, DISPLAY_REFRESH_RATE);
ScrollingText albumName(u8g2, 31, DISPLAY_REFRESH_RATE);

void displaySong()
{
    // print song name
    u8g2.setFont(u8g2_font_ncenB08_tr);
    songName.update(spotconn.currentSong.song);

    // print artist and album name
    u8g2.setFont(u8g2_font_PixelTheatre_te);
    artistName.update(spotconn.currentSong.artist);
    albumName.update(spotconn.currentSong.album);

    // print device name if available
    if (spotconn.currentDevice.name != "")
    {
        u8g2.setCursor(0, 43);
        u8g2.printf("On '%s'", spotconn.currentDevice.name.c_str());
    }

    // print current time (left aligned) and duration (right aligned)
    u8g2.setCursor(0, 55);
    u8g2.printf("%s", formatSongTime(spotconn.currentSongPositionMs).c_str());
    u8g2.setCursor(u8g2.getWidth() - u8g2.getStrWidth(formatSongTime(spotconn.currentSong.durationMs).c_str()), 55);
    u8g2.printf("%s", formatSongTime(spotconn.currentSong.durationMs).c_str());

    // draw progress bar
    float progress = spotconn.currentSongPositionMs / spotconn.currentSong.durationMs;
    int barLength = u8g2.getWidth() * progress;
    int barHeight = 4;
    u8g2.drawBox(0, u8g2.getHeight() - barHeight - 1, barLength, barHeight);

    // update song position
    if (spotconn.isPlaying)
    {
        spotconn.currentSongPositionMs += 1000 / DISPLAY_REFRESH_RATE;
        if (spotconn.currentSongPositionMs > spotconn.currentSong.durationMs)
        {
            spotconn.currentSongPositionMs = spotconn.currentSong.durationMs;
        }
    }
}

void printUsages()
{
    // print usages of all tasks
    log_e("================================================================================");

    auto displayTaskUsage = uxTaskGetStackHighWaterMark(taskDisplayHandle);
    auto serveTaskUsage = uxTaskGetStackHighWaterMark(taskServeHandle);
    auto refreshSongTaskUsage = uxTaskGetStackHighWaterMark(taskRefreshSongHandle);
    auto loopTaskUsage = uxTaskGetStackHighWaterMark(NULL);

    log_e("Display task usage:      [%.2f%%]", (float)(STACK_SIZE_DISPLAY - displayTaskUsage) / STACK_SIZE_DISPLAY * 100);
    log_e("Refresh song task usage: [%.2f%%]", (float)(STACK_SIZE_REFRESH_SONG - refreshSongTaskUsage) / STACK_SIZE_REFRESH_SONG * 100);
    log_e("Loop task usage:         [%.2f%%]", (float)(STACK_SIZE_LOOP - loopTaskUsage) / STACK_SIZE_LOOP * 100);

    log_e("--------------------------------------------------------------------------------");

    // print heap and psram
    auto freePSRAM = ESP.getMinFreePsram();
    auto maxPSRAM = ESP.getPsramSize();

    auto freeHeap = ESP.getMinFreeHeap();
    auto maxHeap = ESP.getHeapSize();

    auto maxAllocPSRAM = ESP.getMaxAllocPsram();
    auto maxAllocHeap = ESP.getMaxAllocHeap();

    // auto freeSketch = ESP.getFreeSketchSpace();
    // auto maxSketch = ESP.getSketchSize();

    // print used space in percent
    if (maxPSRAM > 0)
    {
        log_e("Used PSRAM:  %.2f%%", (float)(maxPSRAM - freePSRAM) / maxPSRAM * 100);
        log_e("Max alloc PSRAM: %d kB", maxAllocPSRAM / 1024);
    }
    log_e("Used Heap:   %.2f%%", (float)(maxHeap - freeHeap) / maxHeap * 100);
    log_e("Max alloc Heap: %d kB", maxAllocHeap / 1024);
    // log_d("Used Sketch: %.2f%%", (float)(maxSketch - freeSketch) / maxSketch * 100);
    log_e("--------------------------------------------------------------------------------");
    log_e("Loop running on core %d", xPortGetCoreID());

    log_e("================================================================================");
}

void taskDisplay(void *pvParameters)
{
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);
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
            u8g2.drawStr(0, 10, "Go to");
            u8g2.drawStr(0, 20, "http://spotifybuddy.local");
            u8g2.drawStr(0, 30, "in your browser and");
            u8g2.drawStr(0, 40, "enter your Spotify");
            u8g2.drawStr(0, 50, "credentials");
            break;
        case NOT_AUTHORIZED:
        case INIT_AUTHORIZATION:
            u8g2.drawStr(0, 10, "Go to");
            u8g2.drawStr(0, 20, "http://spotifybuddy.local");
            u8g2.drawStr(0, 30, "in your browser");
            u8g2.drawStr(0, 40, "to authorize Spotify");
            break;
        case INIT_WIFI:
            u8g2.drawStr(0, 10, "Connect to WiFi");
            u8g2.drawStr(0, 20, "'Spotify Connect Setup'");
            u8g2.drawStr(0, 30, "and go to");
            u8g2.setCursor(0, 40);
            u8g2.printf("http://%s", WiFi.softAPIP().toString().c_str());
            u8g2.drawStr(0, 50, "to set up WiFi");
            if (savedClientID == NULL || savedClientSecret == NULL)
            {
                u8g2.drawStr(0, 60, "and Spotify");
            }
            break;
        case DONE_SETUP:
        case NO_SONG:
            displayClock();
            break;
        case PLAYING_SONG:
            displaySong();
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
    if (savedClientID == NULL)
    {
        auto enteredClientID = clientIDParameter.getValue();
        log_d("Entered clientID: %s\n", enteredClientID);

        if (strlen(enteredClientID) != 32)
        {
            log_e("ClientID should be of length 32");
            return;
        }

        for (int i = 0; i < 32; i++)
        {
            if (!isxdigit(enteredClientID[i]))
            {
                log_e("ClientID should only contain hex characters");
                return;
            }
        }

        NVSHandler nvsHandler;
        nvsHandler.set("clientID", enteredClientID);

        log_d("Saved clientID");
    }

    if (savedClientSecret == NULL)
    {
        auto enteredClientSecret = clientSecretParameter.getValue();
        log_d("Entered clientSecret: %s\n", enteredClientSecret);

        if (strlen(enteredClientSecret) != 32)
        {
            log_e("ClientSecret should be of length 32");
            return;
        }

        for (int i = 0; i < 32; i++)
        {
            if (!isxdigit(enteredClientSecret[i]))
            {
                log_e("ClientSecret should only contain hex characters");
                return;
            }
        }

        NVSHandler nvsHandler;
        nvsHandler.set("clientSecret", enteredClientSecret);

        log_d("Saved clientSecret");
    }

    log_d("Restarting ESP, please refresh the page after approx 10 seconds");

    ESP.restart();
    delay(1000);
}

void handlePageRoot()
{
    char page[600];
    sprintf(page, mainPage, savedClientID, spotifyRedirectURL);
    // log_e("redirect uri: %s", redirect_uri);
    server.send(200, "text/html", String(page));
}

void handlePageCallback()
{
    char page[600];
    if (!spotconn.accessTokenSet)
    {
        if (server.arg("code") == "")
        { // Parameter not found
            log_e("No code found");
            sprintf(page, errorPage, "No code found");
        }
        else
        { // Parameter found
            log_d("Code found");
            spotconn.setClientCredentials(savedClientID, savedClientSecret);
            if (spotconn.getAuth(server.arg("code")))
            {
                sprintf(page, successPage);
            }
            else
            {
                log_e("Failed to get access token");
                sprintf(page, errorPage, "Failed to get access token");
            }
        }
    }
    else
    {
        sprintf(page, successPage);
    }
    server.send(200, "text/html", String(page)); // Send web page
}

void taskRefreshSong(void *pvParameters)
{
    while (true)
    {
        if (spotconn.accessTokenSet)
        {
            log_d("Getting track info");

            bool status = false;

            // if (xSemaphoreTake(httpMutex, 1000))
            // {
            //     status = spotconn.getTrackInfo();
            //     xSemaphoreGive(httpMutex);
            // }

            // if (status)
            // {
            //     currentStatus = PLAYING_SONG;

            //     if (spotconn.currentSong.song == "")
            //     {
            //         currentStatus = NO_SONG;
            //         log_d("No song playing");
            //     }
            //     else
            //     {
            //         log_d("Current song:");
            //         spotconn.currentSong.print();

            //         log_d("Getting device info");

            //         if (xSemaphoreTake(httpMutex, 1000))
            //         {
            //             status = spotconn.getDeviceInfo();
            //             xSemaphoreGive(httpMutex);
            //         }

            //         if (status)
            //         {
            //             log_d("Current device:");
            //             spotconn.activeDevice.print();
            //         }
            //         else
            //         {
            //             log_e("Failed to get device info");
            //             currentStatus = ERROR;
            //             currentError = "Failed to get device info";
            //         }
            //     }

            //     // log_d(spotconn.currentSong.song + " by " + spotconn.currentSong.artist);
            //     // log_d(spotconn.currentSong.album + " - [" + spotconn.currentSongPositionMs + "/" + spotconn.currentSong.durationMs + "] ms");
            // }
            // else
            // {
            //     log_e("Failed to get track info");
            //     currentStatus = ERROR;
            //     currentError = "Failed to get track info";
            // }

            if (xSemaphoreTake(httpMutex, 1000))
            {
                status = spotconn.getInfo();
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

                    log_d("Current device:");
                    spotconn.currentDevice.print();
                }
            }
            else
            {
                log_e("Failed to get info");
                currentStatus = ERROR;
                currentError = "Failed to get info";
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
    if (detect_reset())
    {
        log_e("Settings reset detected");

        NVSHandler nvsHandler;

        nvsHandler.eraseAll();
        nvsHandler.set("accessToken", "null");
        nvsHandler.set("refreshToken", "null");
        wifiManager.resetSettings();

        log_e("Erased all saved data");
    }

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

    NVSHandler nvsHandler;
    savedClientID = nvsHandler.get("clientID");
    savedClientSecret = nvsHandler.get("clientSecret");

    log_d("Connecting to WiFi");
    currentStatus = INIT_WIFI;
    wifiManager.setDebugOutput(false);
    // wifiManager.setCaptivePortalEnable(true);
    if (savedClientID == NULL)
    {
        wifiManager.addParameter(&clientIDParameter);
    }
    if (savedClientSecret == NULL)
    {
        wifiManager.addParameter(&clientSecretParameter);
    }
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

    log_d("Getting credentials");
    currentStatus = INIT_CREDENTIALS;

    char savedRefreshToken[100];
    char savedAccessToken[100];

    nvsHandler.get("refreshToken", savedRefreshToken, sizeof(savedRefreshToken));
    nvsHandler.get("accessToken", savedAccessToken, sizeof(savedAccessToken));

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
        log_d("Setting up MDNS responder");
        if (!MDNS.begin("spotifybuddy"))
        {
            log_e("Error setting up MDNS responder!");
            currentError = "Error setting up MDNS responder!";
            currentStatus = ERROR;
            assert(false);
        }
        else
        {
            log_d("mDNS responder started");
        }
        log_d("Starting auth server");
        server.on("/", handlePageRoot);
        server.on("/wifisave", handlePageRoot);
        server.on("/callback", handlePageCallback);
        server.begin();

        log_d("Go to http://spotifybuddy.local in your browser to verify your spotify account.");

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

#if PRINT_USAGES
        if (millis() - lastUsagePrint > 5000)
        {
            printUsages();
            lastUsagePrint = millis();
        }
#endif
    }
    else
    {
        currentStatus = NOT_AUTHORIZED;
    }

    vTaskDelay(100);
}
