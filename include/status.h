#ifndef _STATUS_H
#define _STATUS_H

#define STATUS_STR(status) #status

typedef enum currentStatus_e
{
    NONE,
    INIT_WIFI,
    DONE_WIFI,
    INIT_SERVER,
    DONE_SERVER,
    INIT_PINS,
    DONE_PINS,
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

#endif // _STATUS_H