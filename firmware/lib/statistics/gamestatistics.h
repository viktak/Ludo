#ifndef GAMESTATISTICS_H
#define GAMESTATISTICS_H

#include <Arduino.h>

#define SerialMon Serial

#define NUMBEROFPLAYERS 6

////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////
struct PlayerStatistics
{
    unsigned long FinishedMillis;

    uint16_t Kicked[NUMBEROFPLAYERS];
    uint16_t Rolls[6];
    uint16_t EmptyRolls[6];
    uint16_t TotalPips;
    uint16_t EmptyPips;
};

class GameStatistics
{

private:
    String TimeIntervalToString(const time_t time);

public:
    uint16_t Round = 0;
    unsigned long StartMillis = millis();
    PlayerStatistics Player[NUMBEROFPLAYERS];

    GameStatistics();
    void Reset();
    void Print();
};

#endif