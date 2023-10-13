// simply paste the default u8g2 constructor here
#define U8G2_DISPLAY U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, PIN_DISPLAY_CS, PIN_DISPLAY_DC, PIN_DISPLAY_RESET);

#define BUDDY_BOARD 1 // options: LOLIN32 (0), N16R8 (1)

// define the pins here
// ========================================================
#if BUDDY_BOARD == 0
#define PIN_BACKWARD_BUTTON GPIO_NUM_32
#define PIN_PLAY_BUTTON GPIO_NUM_33
#define PIN_FORWARD_BUTTON GPIO_NUM_25
#define PIN_VOLUME_POTENTIOMETER GPIO_NUM_35
#elif BUDDY_BOARD == 1
#define PIN_BACKWARD_BUTTON GPIO_NUM_8
#define PIN_PLAY_BUTTON GPIO_NUM_18
#define PIN_FORWARD_BUTTON GPIO_NUM_17
#define PIN_VOLUME_POTENTIOMETER GPIO_NUM_1
#else
#error "BUDDY_BOARD must be 0 (LOLIN32) or 1 (N16R8)"
#endif
// ========================================================
#if BUDDY_BOARD == 0
#define PIN_DISPLAY_CS GPIO_NUM_22
#define PIN_DISPLAY_DC GPIO_NUM_21
#define PIN_DISPLAY_RESET GPIO_NUM_17
#elif BUDDY_BOARD == 1
#define PIN_DISPLAY_CS GPIO_NUM_4
#define PIN_DISPLAY_DC GPIO_NUM_5
#define PIN_DISPLAY_RESET GPIO_NUM_6
#define PIN_SPI_SCK GPIO_NUM_7
#define PIN_SPI_MOSI GPIO_NUM_15
#define PIN_SPI_MISO GPIO_NUM_16
#endif
// ========================================================
#if BUDDY_BOARD == 0
#define PIN_RESET_SETTINGS GPIO_NUM_2
#elif BUDDY_BOARD == 1
#define PIN_RESET_SETTINGS GPIO_NUM_2
#endif
// ========================================================

#define DISPLAY_REFRESH_RATE 10 // In FPS (frames per second) or Hz
#define SONG_REFRESH_TIME 1000  // In ms

#define UTC_OFFSET_S 7200    // UTC offset in seconds
#define WEEKDAYS_LANGUAGE DE // options: EN (0), DE (1), FR (2), IT (3), ES (4), PT (5), NL (6)

#define BUTTON_DEBOUNCE_TIME 50               // In ms
#define POTENTIOMETER_FILTER_COEFFICIENT 0.9f // Between 0 (no change) and 1 (no filtering)

#define HAS_VOLUME_POTENTIOMETER true // Set to false if you don't have a volume potentiometer

#define PRINT_USAGES true // Set to true to print the memory usage every 5 seconds
