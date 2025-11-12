#ifndef PLAYER_H
#define PLAYER_H

#include <Arduino.h>
#include <NeoPixelBus.h>
#include <Preferences.h>

#include "piece.h"

#define SerialMon Serial

#define NUMBEROFPIECESPERPLAYER 4
#define PLAYER_SLICE_OFFSET 18 //  Each player takes up 18 spaces

////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////
class Player
{
private:
    uint8_t _id;

public:
    RgbwColor Color;
    bool IsActive;
    bool IsFinished;

    uint16_t GarageTiles[4];
    uint16_t FinishTiles[4];
    uint16_t ThreatenedTiles[4];
    uint16_t HomeTile;

    Piece Pieces[NUMBEROFPIECESPERPLAYER];

    //  functions
    uint8_t GetId();
    void SetId(const uint8_t id);

    void Load();
    void Save();

    Player(uint8_t newId);
    Player();
};

#endif