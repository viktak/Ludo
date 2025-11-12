#include <TimeLib.h>

#include "common.h"
#include "version.h"
#include "main.h"

char *stack_start;

void SetRandomSeed()
{
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}

String DateTimeToString(const time_t time)
{
    char tmp[20];
    sprintf(tmp, "%u-%02u-%02u %02u:%02u:%02u", year(time), month(time), day(time), hour(time), minute(time), second(time));

    return (String)tmp;
}

void DateTimeToString(char *dest)
{
    sprintf(dest, "%u-%02u-%02u %02u:%02u:%02u", year(), month(), day(), hour(), minute(), second());
}

String TimeIntervalToString(const time_t time)
{
    char tmp[10];
    sprintf(tmp, "%02u:%02u:%02u", uint32_t(time / 3600), minute(time), second(time));
    return (String)tmp;
}

uint32_t GetChipID()
{
    uint32_t chipId = 0;
    for (int i = 0; i < 17; i = i + 8)
    {
        chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    return chipId;
}

int8_t GetPrintableCardID(char *dest, const char *source)
{
    return sprintf(dest, "%02X%02X%02X%02X%02X%02X%02X", source[0], source[1], source[2], source[3], source[4], source[5], source[6]);
}

int8_t GetPrintableCardUID(char *dest, const char *source)
{
    return sprintf(dest, "%02X%02X%02X%02X", source[0], source[1], source[2], source[3]);
}

const char *GetResetReasonString(RESET_REASON reason)
{
    switch (reason)
    {
    case 1:
        return ("Vbat power on reset");
        break;
    case 3:
        return ("Software reset digital core");
        break;
    case 4:
        return ("Legacy watch dog reset digital core");
        break;
    case 5:
        return ("Deep Sleep reset digital core");
        break;
    case 6:
        return ("Reset by SLC module, reset digital core");
        break;
    case 7:
        return ("Timer Group0 Watch dog reset digital core");
        break;
    case 8:
        return ("Timer Group1 Watch dog reset digital core");
        break;
    case 9:
        return ("RTC Watch dog Reset digital core");
        break;
    case 10:
        return ("Instrusion tested to reset CPU");
        break;
    case 11:
        return ("Time Group reset CPU");
        break;
    case 12:
        return ("Software reset CPU");
        break;
    case 13:
        return ("RTC Watch dog Reset CPU");
        break;
    case 14:
        return ("for APP CPU, reset by PRO CPU");
        break;
    case 15:
        return ("Reset when the vdd voltage is not stable");
        break;
    case 16:
        return ("RTC Watch dog reset digital core and rtc module");
        break;
    default:
        return ("NO_MEAN");
    }
}

String GetFirmwareVersionString()
{
    return String(FIRMWARE_VERSION) + " @ " + String(__TIME__) + " - " + String(__DATE__);
}

boolean checkInternetConnection()
{
    IPAddress timeServerIP;
    int result = WiFi.hostByName(INTERNET_SERVER_NAME, timeServerIP);
    return (result == 1);
}

uint32_t stack_size()
{
    char stack;
    return (uint32_t)stack_start - (uint32_t)&stack;
}

uint32_t hexColorToDecimal(const char *hexColor)
{
    // Skip '#' if present
    if (hexColor[0] == '#')
        hexColor++;

    // Convert hex string to 32-bit integer
    return strtoul(hexColor, NULL, 16);
}

void parseRGBW(const char *rgbwStr, uint8_t &red, uint8_t &green, uint8_t &blue, uint8_t &white)
{
    // Validate input format
    if (rgbwStr == nullptr || rgbwStr[0] != '#' || strlen(rgbwStr) != 9)
    {
        red = green = blue = white = 0;
        return;
    }

    // Helper function to convert hex char to numeric value
    auto charToHex = [](char c) -> uint8_t
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return 10 + (c - 'A');
        if (c >= 'a' && c <= 'f')
            return 10 + (c - 'a');
        return 0; // Invalid char (fallback)
    };

    // Extract and convert components
    red = (charToHex(rgbwStr[1]) << 4) | charToHex(rgbwStr[2]);
    green = (charToHex(rgbwStr[3]) << 4) | charToHex(rgbwStr[4]);
    blue = (charToHex(rgbwStr[5]) << 4) | charToHex(rgbwStr[6]);
    white = (charToHex(rgbwStr[7]) << 4) | charToHex(rgbwStr[8]);
}