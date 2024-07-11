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

char *currentStatusToString(currentStatus_t status)
{
    switch (status)
    {
    case NONE:
        return "NONE";
    case INIT_WIFI:
        return "INIT_WIFI";
    case DONE_WIFI:
        return "DONE_WIFI";
    case INIT_SERVER:
        return "INIT_SERVER";
    case DONE_SERVER:
        return "DONE_SERVER";
    case INIT_PINS:
        return "INIT_PINS";
    case DONE_PINS:
        return "DONE_PINS";
    case INIT_AUTHORIZATION:
        return "INIT_AUTHORIZATION";
    case DONE_AUTHORIZATION:
        return "DONE_AUTHORIZATION";
    case INIT_SONG_REFRESH:
        return "INIT_SONG_REFRESH";
    case DONE_SONG_REFRESH:
        return "DONE_SONG_REFRESH";
    case INIT_BUTTONS:
        return "INIT_BUTTONS";
    case DONE_BUTTONS:
        return "DONE_BUTTONS";
    case DONE_SETUP:
        return "DONE_SETUP";
    case NO_SONG:
        return "NO_SONG";
    case PLAYING_SONG:
        return "PLAYING_SONG";
    case NOT_AUTHORIZED:
        return "NOT_AUTHORIZED";
    case ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }

    return "UNKNOWN";
}

#endif // _STATUS_H