#ifndef PIECE_H
#define PIECE_H

#include <Arduino.h>
#include <NeoPixelBus.h>

#define SerialMon Serial

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

class Piece
{

private:
    uint16_t _position;
    uint8_t _id;

public:
    uint16_t threatens;

    //  functions
    uint8_t GetId();
    void SetId(uint8_t newId);

    void SetPosition(uint16_t newPosition);
    uint16_t GetPosition();

    Piece();
    
};

#endif