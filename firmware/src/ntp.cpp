#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#include <NTPClient.h>

#include "ntp.h"
#include "common.h"

WiFiUDP ntpUDP;

#ifdef __localNTP
char timeServer[] = "192.168.123.6";
#else
char timeServer[] = "europe.pool.ntp.org";
#endif

NTPClient timeClient(ntpUDP, timeServer, 0, NTP_UPDATE_INTERVAL_MS);

void setupNTP()
{
    timeClient.begin();
}

void loopNTP()
{
    timeClient.update();
    if (timeClient.isTimeSet())
    {
        setTime(timeClient.getEpochTime());
    }
}
