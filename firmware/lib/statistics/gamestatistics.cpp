#include <TimeLib.h>

#include "gamestatistics.h"

String GameStatistics::TimeIntervalToString(const time_t time)
{
    char tmp[10];
    sprintf(tmp, "%02u:%02u:%02u", uint32_t(time / 3600), minute(time), second(time));
    return (String)tmp;
}

void GameStatistics::Reset()
{
    Round = 0;
    this->StartMillis = millis();
    for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
    {
        this->Player[j].TotalPips = 0;
        this->Player[j].FinishedMillis = 0;
        this->Player[j].EmptyPips = 0;
        for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        {
            this->Player[j].Kicked[i] = 0;
            this->Player[j].Rolls[i] = 0;
            this->Player[j].EmptyRolls[i] = 0;
        }
    }
}

void GameStatistics::Print()
{
    SerialMon.printf("\r\nGame started at %s\r\n", TimeIntervalToString(this->StartMillis));
    SerialMon.printf("\r\nTotal round: %u\r\n", Round);
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
    {
        SerialMon.printf("Player #%u\r\n", i);
        SerialMon.printf("\trolled a total of %u pips (%u blanks)\r\n", this->Player[i].TotalPips, this->Player[i].EmptyPips);
        if (this->Player[i].FinishedMillis > 0)
            SerialMon.printf("\tand finished after %s of play time.\r\n", TimeIntervalToString((this->Player[i].FinishedMillis - this->StartMillis) / 1000));
        for (size_t j = 0; j < 6; j++)
            SerialMon.printf("\t... rolled %u %u times (%u blanks)\r\n", j + 1, Player[i].Rolls[j], Player[i].EmptyRolls[j]);
        for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
            SerialMon.printf("\t... kicked out player #%u %u times.\r\n", j, Player[i].Kicked[j]);
    }

    SerialMon.println();
}

GameStatistics::GameStatistics()
{
    Reset();
}
