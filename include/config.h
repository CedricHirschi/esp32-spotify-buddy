// simply paste the default u8g2 constructor here (see https://github.com/olikraus/u8g2/wiki/u8g2setupcpp)
#define U8G2_DISPLAY U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, PIN_DISPLAY_CS, PIN_DISPLAY_DC, PIN_DISPLAY_RESET);

// GENERAL SETTINGS
// ========================================================
#define HAS_VOLUME_POTENTIOMETER true // Set to false if you don't have a volume potentiometer
#define MDNS_HOSTNAME "spotbud"       // mDNS hostname (e.g. spotbud --> connect to http://spotbud.local instead of IP address)
// ========================================================

// PINS
// ========================================================
#define PIN_BACKWARD_BUTTON GPIO_NUM_8
#define PIN_PLAY_BUTTON GPIO_NUM_18
#define PIN_FORWARD_BUTTON GPIO_NUM_17
#define PIN_VOLUME_POTENTIOMETER GPIO_NUM_1 // irrelevant if HAS_VOLUME_POTENTIOMETER is false

#define PIN_DISPLAY_CS GPIO_NUM_4
#define PIN_DISPLAY_DC GPIO_NUM_5
#define PIN_DISPLAY_RESET GPIO_NUM_6
#define PIN_SPI_SCK GPIO_NUM_7
#define PIN_SPI_MOSI GPIO_NUM_15
#define PIN_SPI_MISO GPIO_NUM_16

#define PIN_RESET_SETTINGS GPIO_NUM_45 // Reset settings to default
#define PIN_RESET_ACTIVE HIGH          // Active state of the reset pin (HIGH or LOW)
// ========================================================

// TIMING
// ========================================================
#define DISPLAY_REFRESH_RATE 10               // How often the display is refreshed (in FPS)
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
#define PRINT_USAGES true // Set to true to print the memory usage every 5 seconds
// ========================================================
