#ifndef _INDEX_H
#define _INDEX_H

const char mainPage[] PROGMEM = R"=====(
<HTML>
    <HEAD>
        <TITLE>Spotify Buddy Authentication</TITLE>
        <META HTTP-EQUIV="refresh" CONTENT="1;URL=https://accounts.spotify.com/authorize?response_type=code&client_id=%s&redirect_uri=%s&scope=user-modify-playback-state user-read-currently-playing user-read-playback-state user-library-modify user-library-read"/>
    </HEAD>
    <BODY>
        <CENTER>
            <BR>
            <B>Redirecting...</B>
        </CENTER>
    </BODY>
</HTML>
)=====";

const char errorPage[] PROGMEM = R"=====(
<HTML>
    <HEAD>
        <TITLE>Spotify Buddy Authentication</TITLE>
    </HEAD>
    <BODY>
        <CENTER>
            <BR>
            <B>Error:</B> %s
            <BR>
            <BR>
            <A HREF="/">Retry</A>
        </CENTER>
    </BODY>
</HTML>
)=====";

const char successPage[] PROGMEM = R"=====(
<HTML>
    <HEAD>
        <TITLE>Spotify Buddy Authentication</TITLE>
    </HEAD>
    <BODY>
        <CENTER>
            <BR>
            <B>Success!</B>
            <BR>
            <BR>
            You can now close this page.
        </CENTER>
    </BODY>
</HTML>
)=====";

#endif