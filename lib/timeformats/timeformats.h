#ifndef TIMEFORMATS_H
#define TIMEFORMATS_H

#include <Arduino.h>
#include <NTPClient.h>

#define EN 0
#define DE 1
#define FR 2
#define IT 3
#define ES 4
#define PT 5
#define NL 6

#include "config.h"

#if WEEKDAYS_LANGUAGE == EN
char daysOfTheWeek[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
#elif WEEKDAYS_LANGUAGE == DE
char daysOfTheWeek[7][12] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
#elif WEEKDAYS_LANGUAGE == FR
char daysOfTheWeek[7][10] = {"Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"};
#elif WEEKDAYS_LANGUAGE == IT
char daysOfTheWeek[7][10] = {"Domenica", "Lunedì", "Martedì", "Mercoledì", "Giovedì", "Venerdì", "Sabato"};
#elif WEEKDAYS_LANGUAGE == ES
char daysOfTheWeek[7][10] = {"Domingo", "Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};
#elif WEEKDAYS_LANGUAGE == PT
char daysOfTheWeek[7][10] = {"Domingo", "Segunda-feira", "Terça-feira", "Quarta-feira", "Quinta-feira", "Sexta-feira", "Sábado"};
#elif WEEKDAYS_LANGUAGE == NL
char daysOfTheWeek[7][10] = {"Zondag", "Maandag", "Dinsdag", "Woensdag", "Donderdag", "Vrijdag", "Zaterdag"};
#else
#error "WEEKDAYS_LANGUAGE not set correctly. See config.h for more information."
#endif

String formatCurrentDate(NTPClient &timeClient)
{
    time_t rawtime = timeClient.getEpochTime();
    struct tm *timeinfo = localtime(&rawtime);

    String date = daysOfTheWeek[timeinfo->tm_wday];
    date += ",  ";

    if (timeinfo->tm_mday < 10)
        date += "0";
    date += String(timeinfo->tm_mday);
    date += ".";

    if (timeinfo->tm_mon < 9)
        date += "0";
    date += String(timeinfo->tm_mon + 1);
    date += ".";

    date += String(timeinfo->tm_year + 1900);

    return date;
}

String formatSongTime(uint ms)
{
    String result = String(ms / 60000);

    result += ":";
    if (ms % 60000 < 10000)
        result += "0";
    result += String(ms % 60000 / 1000);

    return result;
}

#endif // TIMEFORMATS_H