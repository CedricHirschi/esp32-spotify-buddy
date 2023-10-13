#include "spotconn.h"

String getValue(HTTPClient &http, String key);

SpotConn::SpotConn()
{
    client = std::unique_ptr<WiFiClientSecure>(new WiFiClientSecure);
    client->setInsecure();
}

SpotConn::~SpotConn()
{
    client->stop();

    // delete client;
}

void SpotConn::setClientCredentials(const char *clientID, const char *clientSecret)
{
    this->clientID = clientID;
    this->clientSecret = clientSecret;
}

void SpotConn::setTokens(const char *accessToken, const char *refreshToken)
{
    this->accessToken = accessToken;
    this->refreshToken = refreshToken;
}

bool SpotConn::getAuth(String serverCode)
{
    sprintf(redirect_uri, "http://%s/callback", WiFi.localIP().toString().c_str());
    https.begin(*client, F("https://accounts.spotify.com/api/token"));
    String clientCreds = clientID;
    clientCreds += ":";
    clientCreds += clientSecret;
    authBasic = "Basic "; //  + base64::encode(String(CLIENT_ID) + ":" + String(CLIENT_SECRET));
    authBasic += base64::encode(clientCreds);
    https.addHeader(F("Authorization"), authBasic);
    https.addHeader(F("Content-Type"), F("application/x-www-form-urlencoded"));
    requestBody = "grant_type=authorization_code&code=";
    requestBody += serverCode;
    requestBody += "&redirect_uri=";
    requestBody += redirect_uri;
    // Send the POST request to the Spotify API
    int httpResponseCode = https.POST(requestBody);
    // Check if the request was successful
    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = https.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);
        accessToken = String((const char *)doc[F("access_token")]);
        refreshToken = String((const char *)doc[F("refresh_token")]);
        tokenExpireTime = doc[F("expires_in")];
        tokenStartTime = millis();
        accessTokenSet = true;

        authBearer = "Bearer " + accessToken;

        // log_d(accessToken);
        // log_d(refreshToken);
        // log_d("access token: %s\nrefresh token: %s", accessToken.c_str(), refreshToken.c_str());
        log_d("Tokens set");
    }
    else
    {
        log_e("Error getting tokens: %s", https.getString().c_str());
        lastError = httpResponseCode;
    }
    // Disconnect from the Spotify API
    https.end();
    return accessTokenSet;
}

bool SpotConn::refreshAuth()
{
    https.begin(*client, F("https://accounts.spotify.com/api/token"));
    String clientCreds = clientID;
    clientCreds += ":";
    clientCreds += clientSecret;
    authBasic = "Basic "; //  + base64::encode(String(CLIENT_ID) + ":" + String(CLIENT_SECRET));
    authBasic += base64::encode(clientCreds);
    https.addHeader(F("Authorization"), authBasic);
    https.addHeader(F("Content-Type"), F("application/x-www-form-urlencoded"));
    requestBody = "grant_type=refresh_token&refresh_token="; // + String(refreshToken);
    requestBody += refreshToken;
    // Send the POST request to the Spotify API
    int httpResponseCode = https.POST(requestBody);
    accessTokenSet = false;
    // Check if the request was successful
    if (httpResponseCode == HTTP_CODE_OK)
    {
        String response = https.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        accessToken = String((const char *)doc[F("access_token")]);

        authBearer = "Bearer " + accessToken;

        tokenExpireTime = doc[F("expires_in")];
        tokenStartTime = millis();
        accessTokenSet = true;

        log_d("Tokens refreshed");
    }
    else
    {
        log_e("Error getting tokens: %s", https.getString().c_str());
        lastError = httpResponseCode;
    }
    // Disconnect from the Spotify API
    https.end();
    return accessTokenSet;
}

bool SpotConn::getTrackInfo()
{
    log_d("Start");
    // url = F("https://api.spotify.com/v1/me/player/currently-playing");
    char urlNew[] = "https://api.spotify.com/v1/me/player/currently-playing";
    https.useHTTP10(true);
    // https.begin(*client, url);
    https.begin(*client, urlNew);
    // authBearer = "Bearer " + String(accessToken);
    https.addHeader(F("Authorization"), authBearer);
    https.addHeader(F("Content-Type"), F("application/json")); // Ensure Content-Type is set to application/json
    int httpResponseCode = https.GET();                        // Send an empty JSON object as the body
    bool success = false;
    String songId = "";
    // Check if the request was successful
    if (httpResponseCode == 200)
    {
        log_d("Response: successful");

        String currentSongProgress = getValue(https, F("progress_ms"));
        currentSongPositionMs = currentSongProgress.toFloat();

        String artistName = getValue(https, F("name"));
        String albumName = getValue(https, F("name"));
        String songDuration = getValue(https, F("duration_ms"));
        currentSong.durationMs = songDuration.toInt();
        String songName = getValue(https, F("name"));
        songId = getValue(https, F("uri"));
        String isPlay = getValue(https, F("is_playing"));
        isPlaying = isPlay == "true";

        songId = songId.substring(15, songId.length() - 1);

        https.end();

        currentSong.album = albumName.substring(1, albumName.length() - 1);
        currentSong.artist = artistName.substring(1, artistName.length() - 1);
        currentSong.song = songName.substring(1, songName.length() - 1);
        currentSong.Id = songId;

        success = true;
    }
    else if (httpResponseCode == 204)
    {
        log_d("Response: no content");

        https.end();

        currentSong.album = "";
        currentSong.artist = "";
        currentSong.song = "";
        currentSong.Id = "";
        currentSongPositionMs = 0;

        success = true;
    }
    else
    {
        log_e("Error getting track info: %s", https.getString().c_str());
        lastError = httpResponseCode;
        https.end();
    }

    // Disconnect from the Spotify API
    if (success)
    {
        lastSongPositionMs = currentSongPositionMs;
    }

    log_d("End");

    return success;
}

bool SpotConn::parseDevices(String &json)
{
    DynamicJsonDocument doc(12288);

    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        log_e("Error parsing devices: %s", error.c_str());

        return false;
    }

    currentDevice.id = doc["device"]["id"].as<String>();
    currentDevice.name = doc["device"]["name"].as<String>();
    currentDevice.isActive = doc["device"]["is_active"].as<bool>();
    currentDevice.isRestricted = doc["device"]["is_restricted"].as<bool>();
    currentDevice.supportsVolume = doc["device"]["supports_volume"].as<bool>();

    doc.garbageCollect();
    doc.clear();

    return true;
}

bool SpotConn::getDeviceInfo()
{
    log_d("Start");
    // url = F("https://api.spotify.com/v1/me/player/currently-playing");
    char urlNew[] = "https://api.spotify.com/v1/me/player/devices";
    https.useHTTP10(true);
    // https.begin(*client, url);
    https.begin(*client, urlNew);
    // authBearer = "Bearer " + String(accessToken);
    https.addHeader(F("Authorization"), authBearer);
    https.addHeader(F("Content-Type"), F("application/json")); // Ensure Content-Type is set to application/json
    int httpResponseCode = https.GET();                        // Send an empty JSON object as the body
    bool success = false;

    if (httpResponseCode == 200)
    {
        log_d("Response: successful");

        String response = https.getString();

        https.end();

        // log_e("Response: %s", response.c_str());

        success = parseDevices(response);
    }
    else
    {
        log_e("Error getting track info: %s", https.getString().c_str());
        lastError = httpResponseCode;
        https.end();
    }

    log_d("End");

    return success;
}

bool SpotConn::parseSong(String &json)
{
    DynamicJsonDocument doc(12288);

    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        log_e("Error parsing song: %s", error.c_str());

        return false;
    }

    currentSong.Id = doc["item"]["id"].as<String>();
    currentSong.song = doc["item"]["name"].as<String>();
    currentSong.artist = doc["item"]["artists"][0]["name"].as<String>();
    currentSong.album = doc["item"]["album"]["name"].as<String>();
    currentSong.durationMs = doc["item"]["duration_ms"].as<int>();

    currentSongPositionMs = doc["progress_ms"].as<float>();

    isPlaying = doc["is_playing"].as<bool>();

    doc.garbageCollect();
    doc.clear();

    return true;
}

bool SpotConn::parseInfo(String &json)
{
    DynamicJsonDocument filter(256);

    JsonObject filter_device = filter.createNestedObject("device");
    filter_device["id"] = true;
    filter_device["name"] = true;
    filter_device["is_active"] = true;
    filter_device["is_restricted"] = true;
    filter_device["supports_volume"] = true;

    JsonObject filter_item = filter.createNestedObject("item");
    filter_item["id"] = true;
    filter_item["name"] = true;
    filter_item["artists"] = true;
    filter_item["album"] = true;
    filter_item["duration_ms"] = true;

    filter["progress_ms"] = true;
    filter["is_playing"] = true;

    filter.shrinkToFit();

    DynamicJsonDocument doc(8096);

    DeserializationError error = deserializeJson(doc, json, DeserializationOption::Filter(filter));
    if (error)
    {
        log_e("Error parsing song: %s", error.c_str());

        return false;
    }
    doc.shrinkToFit();

    JsonObject device = doc["device"];
    currentDevice.id = device["id"].as<String>();
    currentDevice.name = device["name"].as<String>();
    currentDevice.isActive = device["is_active"].as<bool>();
    currentDevice.isRestricted = device["is_restricted"].as<bool>();
    currentDevice.supportsVolume = device["supports_volume"].as<bool>();

    JsonObject item = doc["item"];
    currentSong.Id = item["id"].as<String>();
    currentSong.song = item["name"].as<String>();
    currentSong.artist = item["artists"][0]["name"].as<String>();
    currentSong.album = item["album"]["name"].as<String>();
    currentSong.durationMs = item["duration_ms"].as<int>();

    currentSongPositionMs = doc["progress_ms"].as<float>();

    isPlaying = doc["is_playing"].as<bool>();

    return true;
}

bool SpotConn::getInfo()
{
    log_d("Start");

    char urlNew[] = "https://api.spotify.com/v1/me/player";

    https.begin(*client, urlNew);
    https.addHeader(F("Authorization"), authBearer);

    int httpsResponse = https.GET();

    bool success = false;

    switch (httpsResponse)
    {
    case 200: // normal response
    {
        String response = https.getString();
        https.end();

        success = parseInfo(response);
    }
    break;
    case 204: // Playback not acailable or active
        currentDevice = Device();
        currentSong = songDetails();

        success = true;
    case 401: // Bad or expired token
    case 403: // Bad OAuth request
    case 429: // The app has exceeded its rate limits
    default:
        https.end();
        lastError = httpsResponse;
    }

    log_d("End");
    return success;
}

bool SpotConn::togglePlay()
{
    if (!currentDevice.id || currentDevice.isRestricted)
    {
        log_w("No device or device is restricted");
        return false;
    }

    log_d("%s", isPlaying ? F("Pausing") : F("Playing"));
    String url = "https://api.spotify.com/v1/me/player/" + String(isPlaying ? F("pause") : F("play"));
    isPlaying = !isPlaying;
    https.begin(*client, url);

    https.addHeader(F("Authorization"), authBearer);
    https.addHeader(F("Content-Length"), F("0"));
    int httpResponseCode = https.PUT(F("")); // Send an empty JSON object as the body
    bool success = false;
    // Check if the request was successful
    if (httpResponseCode == 204)
    {
        log_d("%s", (isPlaying ? F("Playing") : F("Pausing")));
        success = true;
    }
    else
    {
        log_d("Error pausing or playing: %s", https.getString().c_str());
        lastError = httpResponseCode;
    }

    // Disconnect from the Spotify API
    https.end();
    return success;
}

bool SpotConn::adjustVolume(int vol)
{
    if (!currentDevice.id || currentDevice.isRestricted || !currentDevice.supportsVolume)
    {
        log_w("No device or device is restricted or device doesn't support volume");
        return false;
    }

    String url = "https://api.spotify.com/v1/me/player/volume?volume_percent=" + String(vol);
    https.begin(*client, url);

    https.addHeader(F("Authorization"), authBearer);
    https.addHeader(F("Content-Length"), F("0"));
    int httpResponseCode = https.PUT(F("")); // Send an empty JSON object as the body
    bool success = false;
    // Check if the request was successful
    if (httpResponseCode == 204)
    {
        currVol = vol;
        success = true;
    }
    else
    {
        log_e("Error setting volume: %s", https.getString().c_str());
        lastError = httpResponseCode;
    }

    // Disconnect from the Spotify API
    https.end();
    return success;
}

bool SpotConn::skipForward()
{
    if (!currentDevice.id || currentDevice.isRestricted)
    {
        log_w("No device or device is restricted");
        return false;
    }

    String url = F("https://api.spotify.com/v1/me/player/next");
    https.begin(*client, url);

    https.addHeader(F("Authorization"), authBearer);
    https.addHeader(F("Content-Length"), F("0"));
    int httpResponseCode = https.POST(F("")); // Send an empty JSON object as the body
    bool success = false;
    // Check if the request was successful
    if (httpResponseCode == 204)
    {
        // String response = https.getString();
        log_d("skipping forward");
        success = true;
    }
    else
    {
        log_e("Error skipping forward: %s", https.getString().c_str());
        lastError = httpResponseCode;
    }

    // Disconnect from the Spotify API
    https.end();
    return success;
}

bool SpotConn::skipBack()
{
    if (!currentDevice.id || currentDevice.isRestricted)
    {
        log_w("No device or device is restricted");
        return false;
    }

    String url = F("https://api.spotify.com/v1/me/player/previous");
    https.begin(*client, url);

    https.addHeader(F("Authorization"), authBearer);
    https.addHeader(F("Content-Length"), F("0"));
    int httpResponseCode = https.POST(F("")); // Send an empty JSON object as the body
    bool success = false;
    // Check if the request was successful
    if (httpResponseCode == 204)
    {
        log_d("skipping backward");
        success = true;
    }
    else
    {
        log_e("Error skipping backward: %s", https.getString().c_str());
        lastError = httpResponseCode;
    }

    // Disconnect from the Spotify API
    https.end();
    return success;
}

String getValue(HTTPClient &http, String key)
{
    bool found = false, look = false, seek = true;
    int ind = 0;
    String ret_str = F("");

    int len = http.getSize();
    char char_buff[1];
    WiFiClient *stream = http.getStreamPtr();
    while (http.connected() && (len > 0 || len == -1))
    {
        size_t size = stream->available();
        if (size)
        {
            int c = stream->readBytes(char_buff, ((size > sizeof(char_buff)) ? sizeof(char_buff) : size));
            if (found)
            {
                if (seek && char_buff[0] != ':')
                {
                    continue;
                }
                else if (char_buff[0] != '\n')
                {
                    if (seek && char_buff[0] == ':')
                    {
                        seek = false;
                        int c = stream->readBytes(char_buff, 1);
                    }
                    else
                    {
                        ret_str += char_buff[0];
                    }
                }
                else
                {
                    break;
                }
            }
            else if ((!look) && (char_buff[0] == key[0]))
            {
                look = true;
                ind = 1;
            }
            else if (look && (char_buff[0] == key[ind]))
            {
                ind++;
                if ((unsigned)ind == key.length())
                    found = true;
            }
            else if (look && (char_buff[0] != key[ind]))
            {
                ind = 0;
                look = false;
            }
        }
    }
    if (*(ret_str.end() - 1) == ',')
    {
        ret_str = ret_str.substring(0, ret_str.length() - 1);
    }
    return ret_str;
}