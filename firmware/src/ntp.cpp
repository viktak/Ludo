#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

#include <NTPClient.h>

#include "ntp.h"
#include "common.h"
#include "main.h"

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, appSettings.ntpServer, 0, NTP_UPDATE_INTERVAL_MS);

void setupNTP()
{
    timeClient.begin();
}

void loopNTP()
{
    timeClient.update();
    if (timeClient.isTimeSet())
        setTime(timeClient.getEpochTime());
}
