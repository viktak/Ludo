#include "piece.h"

void Piece::SetId(uint8_t newId)
{
    _id = newId;
}

uint8_t Piece::GetId()
{
    return _id;
}

void Piece::SetPosition(uint16_t newPosition)
{
    _position = newPosition;
}

uint16_t Piece::GetPosition()
{
    return _position;
}

Piece::Piece()
{
}
