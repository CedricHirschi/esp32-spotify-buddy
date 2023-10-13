# ESP32 Spotify Buddy

This project is for a Spotify controller using an ESP32 microcontroller. It uses the Spotify API to control playback and display information on a display (currently monochrome, but can be adapted to colour displays). Based on the [Make it for less's](https://www.youtube.com/@makeit4less) Spotify controller project.

## Features
- Display Spotify playback information
    - track name
    - artist
    - album
    - song length
    - playback position
    - liked status
- Control Spotify playback
    - play/pause
    - next track
    - previous track
    - volume control (optional)
- Easy connection to your Spotify account via hosted web page

## Requirements

### Hardware Requirements
- ESP32 microcontroller
- Display compatible with `u8g2lib` ([compatibility list](https://github.com/olikraus/u8g2/wiki/u8g2setupcpp))
    - **Tested:** SSD1309 (128x64 monochrome OLED)
- Buttons and a potentiometer (optional) for controls

### Software Requirements
[PlatformIO](https://platformio.org/) on 
- [VSCode](https://platformio.org/install/ide?install=vscode) or 
- [CLion](https://www.jetbrains.com/help/clion/platformio.html) or 
- the [CLI](https://docs.platformio.org/en/latest/core/installation/index.html).

## Setup and Installation

### Get, build, upload
1. Clone this repository to your local machine:
```sh
git clone https://github.com/CedricHirschi/spotify-buddy-esp32.git
```
2. Open the project with PlatformIO
3. Configure your project in [`include/config.h`](include/config.h)
4. Connect your ESP32 to your computer (also connect the peripherals to the ESP32 of course)
5. [Build and Upload](https://docs.platformio.org/en/latest/integration/ide/vscode.html#quick-start) the code to your ESP32

### Setup Spotify API
1. Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard/) and create a new app
2. Add `http://<MDNS_HOSTNAME>.local/callback` as a redirect URI when setting up (replace `<MDNS_HOSTNAME>` with the MDNS_HOSTNAME you set in [`include/config.h`](include/config.h#7), standard is `spotbud`)
3. Note down the Client ID and Client Secret you get in the App Overview

### Connect to your device to WiFi and Spotify
Basically, follow the instructions on the display of your ESP32:

1. Connect to the WiFi network `<MDNS_HOSTNAME> Setup`
2. Open the displayed URL in your browser
3. Go to `WiFi configuration` and enter your WiFi and Spotify credentials
4. Confirm and wait until the ESP32 reboots
5. Now, once you connect yourself to the same WiFi network as the ESP32, you authorize the device via the once again displayed URL (the same as before)
7. Wait until you get redirected and authorize the app to access your Spotify account

**Your device should now be ready to use**

## Usage

### Normal operation
During normal operation, you should now see the current playback status on the display.

If no playback is active, a clock is displayed instead.

To control the playback, use the buttons and potentiometer connected to the ESP32.

### Reset
To reset your credentials (WiFi and Spotify), you can bring `PIN_RESET_SETTINGS` to `PIN_RESET_ACTIVE` while you reset the ESP32. This will reset the credentials and you can connect to a new WiFi network and authorize the app again.

### Connecting to a new WiFi network
If you want to connect to a new WiFi network, you just wait until the display prompts you to connect to the WiFi network `<MDNS_HOSTNAME> Setup` again. Now, you can enter the new WiFi credentials.

**Note:** Your Spotify credentials are kept, you shouldn't need to re-enter them.

# Credits

Base project was [Make it for less's](https://www.youtube.com/@makeit4less) Spotify controller project. This project was based on ESP8266 and a color LCD. So if you look forward to this project and its features, check out [his video](https://www.youtube.com/watch?v=SWiPIBWvgIU).