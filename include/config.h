// GENERAL SETTINGS
// ========================================================
#define HAS_VOLUME_POTENTIOMETER false // Set to false if you don't have a volume potentiometer
#define MDNS_HOSTNAME "spotbud"        // mDNS hostname (e.g. spotbud --> connect to http://spotbud.local instead of IP address)
// ========================================================

// PINS
// ========================================================
#define PIN_BACKWARD_BUTTON GPIO_NUM_32
#define PIN_PLAY_BUTTON GPIO_NUM_33
#define PIN_FORWARD_BUTTON GPIO_NUM_34
#define PIN_VOLUME_POTENTIOMETER GPIO_NUM_35 // irrelevant if HAS_VOLUME_POTENTIOMETER is false

#define PIN_DISPLAY_CS 5
#define PIN_DISPLAY_DC 17
#define PIN_DISPLAY_RESET -1

#define PIN_RESET_ENABLED true
#define PIN_RESET_SETTINGS GPIO_NUM_15 // Reset settings to default
#define PIN_RESET_ACTIVE LOW           // Active state of the reset pin (HIGH or LOW)
// ========================================================

// TIMING
// ========================================================
#define DISPLAY_REFRESH_RATE 2                // How often the display is refreshed (in FPS)
#define SONG_REFRESH_TIME 1000                // How often the song is refreshed (in ms)
#define BUTTON_DEBOUNCE_TIME 50               // In ms
#define POTENTIOMETER_FILTER_COEFFICIENT 0.9f // Between 0 (no change) and 1 (no filtering)
// ========================================================

// ERROR HANDLING
// ========================================================
#define HTTP_MUTEX_TIME 200  // Timeout for the HTTP mutex (in ms)
#define GETINFO_MAX_FAILED 5 // How often the getinfo command is retried in a row before showing an error
// ========================================================

// LOCALIZATION
// ========================================================
#define UTC_OFFSET_S 7200    // UTC offset in seconds
#define WEEKDAYS_LANGUAGE DE // options: EN (0), DE (1), FR (2), IT (3), ES (4), PT (5), NL (6)
// ========================================================

// DEBUGGING
// ========================================================
#define PRINT_USAGES false // Set to true to print the memory usage every 5 seconds
// ========================================================
