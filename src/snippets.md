These are some code snippets which have previously been used.

```cpp
void taskButtons(void *pvParameters)
{
    while (true)
    {
        if (spotconn.accessTokenSet)
        {
            if (backwardButton.wasPressed())
            {
                Serial.println("backward button pressed");
                if (xSemaphoreTake(httpMutex, 5000))
                {
                    spotconn.skipBack();
                    xSemaphoreGive(httpMutex);
                }
            }
            else if (playButton.wasPressed())
            {
                Serial.println("play button pressed");
                if (xSemaphoreTake(httpMutex, 5000))
                {
                    spotconn.togglePlay();
                    xSemaphoreGive(httpMutex);
                }
            }
            else if (forwardButton.wasPressed())
            {
                Serial.println("forward button pressed");
                if (xSemaphoreTake(httpMutex, 5000))
                {
                    spotconn.skipForward();
                    xSemaphoreGive(httpMutex);
                }
            }

            // int requestedVolume = map(volumePot.read(), 0, 4095, 0, 100);

            // if (abs(requestedVolume - spotconn.currVol) > 5)
            // {
            //     Serial.println("Adjusting volume from " + String(spotconn.currVol) + " to " + String(requestedVolume));
            //     if (xSemaphoreTake(httpMutex, 1000))
            //     {
            //         spotconn.adjustVolume(requestedVolume);
            //         xSemaphoreGive(httpMutex);
            //     }
            // }
        }

        vTaskDelay(BUTTON_REFRESH_TIME / portTICK_PERIOD_MS);
    }
}
```

```cpp
void taskRefreshToken(void *pvParameters)
{
    while (true)
    {
        vTaskDelay(spotconn.tokenExpireTime * 1000UL / portTICK_PERIOD_MS);

        if (spotconn.accessTokenSet)
        {
            Serial.println("refreshing token");

            bool result = false;

            if (xSemaphoreTake(httpMutex, 1000))
            {
                result = spotconn.refreshAuth();
                xSemaphoreGive(httpMutex);
            }

            if (result)
            {
                Serial.println("refreshed token");
            }
            else
            {
                Serial.println("failed to refresh token");
                currentStatus = ERROR;
                currentError = "Failed to refresh token";
            }
        }
    }
}
```

```cpp
Serial.println("Setting up refresh token task");
currentStatus = INIT_AUTH_REFRESH;
xTaskCreate(
    taskRefreshToken,   /* Task function. */
    "TaskRefreshToken", /* String with name of task. */
    10000,              /* Stack size in bytes. */
    NULL,               /* Parameter passed as input of the task */
    4,                  /* Priority of the task. */
    &taskRefreshTokenHandle);              /* Core where the task should run */
Serial.println("Set up refresh token task");
currentStatus = DONE_AUTH_REFRESH;

Serial.println("Setting up buttons task");
currentStatus = INIT_BUTTONS;
xTaskCreate(
    taskButtons,   /* Task function. */
    "TaskButtons", /* String with name of task. */
    10000,         /* Stack size in bytes. */
    NULL,          /* Parameter passed as input of the task */
    5,             /* Priority of the task. */
    NULL);
Serial.println("Set up buttons task");
currentStatus = DONE_BUTTONS;
```