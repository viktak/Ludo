#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <rom/rtc.h>

#include <WiFi.h>
#include <rom/rtc.h>

#include "settings.h"
#include "TimeChangeRules.h"

#define INTERNET_SERVER_NAME "diy.viktak.com"

static TimeChangeRule *tcr;

extern char *stack_start;

void SetRandomSeed();
String DateTimeToString(const time_t time);
void DateTimeToString(char *dest);
String TimeIntervalToString(const time_t time);
uint32_t GetChipID();
int8_t GetPrintableCardID(char *dest, const char *source);
int8_t GetPrintableCardUID(char *dest, const char *source);
String GetFirmwareVersionString();
boolean checkInternetConnection();

const char *GetResetReasonString(RESET_REASON reason);

uint32_t stack_size();

uint32_t hexColorToDecimal(const char *hexColor);

void parseRGBW(const char *rgbwStr, uint8_t &red, uint8_t &green, uint8_t &blue, uint8_t &white);

#endif