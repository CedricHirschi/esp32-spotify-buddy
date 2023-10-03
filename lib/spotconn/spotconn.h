#ifndef _SPOTCONN_H
#define _SPOTCONN_H

#include <Arduino.h>

#include <base64.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

String getValue(HTTPClient &http, String key);

// #define F(string) (string)

// http response struct
struct httpResponse
{
    int responseCode;
    String responseMessage;
};

struct songDetails
{
    int durationMs;
    String album;
    String artist;
    String song;
    String Id;
    bool isLiked;

    void print(void)
    {
        log_d("'%s' by '%s' on '%s' [%d s]", song.c_str(), artist.c_str(), album.c_str(), durationMs / 1000);
    }
};

struct Device
{
    String id;
    String name;
    bool isActive;
    bool isRestricted;
    bool supportsVolume;

    void print(void)
    {
        log_d("'%s' [%s] (%s, %s, %s)", name.c_str(), id.c_str(), isActive ? "active" : "inactive", isRestricted ? "restricted" : "unrestricted", supportsVolume ? "supports volume" : "doesn't support volume");
    }
};

class SpotConn
{
public:
    /**
     * @brief Constructs a new SpotConn object.
     *
     * @details Initializes a WiFiClientSecure object
     *
     */
    SpotConn();
    /**
     * @brief Destroys the SpotConn object
     *
     * @details Stops the WiFiClientSecure client
     *
     */
    ~SpotConn();

    /**
     * @brief Sets the client credentials
     *
     * @param clientID The client ID
     * @param clientSecret The client secret
     *
     * @warning This function has to be called before `this.getAuth()` or `this.refreshAuth()`
     *
     */
    void setClientCredentials(const char *clientID, const char *clientSecret);
    /**
     * @brief Sets the access and refresh tokens
     *
     * @param accessToken The access token
     * @param refreshToken The refresh token
     *
     * @warning This function has to be called before `this.getTrackInfo()`, `this.togglePlay()`, `this.adjustVolume()`, `this.skipForward()` or `this.skipBack()`
     *
     * @details This function doesn't have to be called if getAuth() or refreshAuth() were called before
     *
     */
    void setTokens(const char *accessToken, const char *refreshToken);

    /**
     * @brief Gets the access and refresh tokens from the Spotify API
     *
     * @param serverCode The server code from the Spotify API
     *
     * @return `true` if the tokens were successfully set, `false` otherwise
     *
     * @details Uses the server code from the Spotify API to get the access and refresh tokens
     *
     */
    bool getAuth(String serverCode);
    /**
     * @brief Refreshes the access token
     *
     * @return `true` if the access token was successfully refreshed, `false` otherwise
     *
     * @details Uses the refresh token to get a new access token
     * @warning This function has to be called before `this.tokenExpireTime` (duration in seconds)
     *
     */
    bool refreshAuth();

    /**
     * @brief Gets the current song's information
     *
     * @return `true` if the song information was successfully retrieved, `false` otherwise
     *
     */
    bool getTrackInfo();
    /**
     * @brief Get the current device's information
     *
     * @return `true` if the device information was successfully retrieved, `false` otherwise
     *
     */
    bool getDeviceInfo();

    /**
     * @brief Toggle the play/pause state of the current song
     *
     * @return `true` if the song was successfully toggled, `false` otherwise
     *
     */
    bool togglePlay();
    /**
     * @brief Adjust the playback volume
     *
     * @param vol The volume to set the playback to
     *
     * @return `true` if the volume was successfully adjusted, `false` otherwise
     *
     */
    bool adjustVolume(int vol);
    /**
     * @brief Skip to the next song
     *
     * @return `true` if the song was successfully skipped, `false` otherwise
     *
     */
    bool skipForward();
    /**
     * @brief Skip to the previous song
     *
     * @return `true` if the song was successfully skipped, `false` otherwise
     *
     */
    bool skipBack();

    bool accessTokenSet = false; /// @brief Whether the access token has been set
    long tokenStartTime;         /// @brief The time the access token was set
    int tokenExpireTime;         /// @brief The timespan in which the access token expires in seconds
    songDetails currentSong;     /// @brief The current song's information
    float currentSongPositionMs; /// @brief The current song's position in milliseconds
    int currVol;                 /// @brief The current volume (0 - 100)
    int lastError;               /// @brief The last error code (mostly HTTP response codes)

    String accessToken;
    String refreshToken;

    Device devices[10];
    Device activeDevice;
    int deviceCount;

private:
    std::unique_ptr<WiFiClientSecure> client;
    HTTPClient https;

    bool isPlaying = false;
    float lastSongPositionMs;

    const char *clientID;
    const char *clientSecret;

    char redirect_uri[100];

    String authBasic;
    String authBearer;
    String requestBody;
    String url;
};

#endif // _SPOTCONN_H