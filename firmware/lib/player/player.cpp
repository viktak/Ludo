#include "player.h"

Preferences playerPrefs;

RgbwColor Color;
bool IsActive = false;
bool IsFinished = false;

uint16_t GarageTiles[4] = {0, 0, 0, 0};
uint16_t FinishTiles[4] = {0, 0, 0, 0};
uint16_t ThreatenedTiles[4] = {255, 255, 255, 255};
uint16_t HomeTile;
Piece Pieces[4];

// bool IsPieceInGarage(uint8_t pieceID)
// {
//     for (size_t i = 0; i < 4; i++)
//         if (GarageTiles[i] == Pieces[i].GetPosition())
//             return true;
//     return false;
// }

uint8_t Player::GetId()
{
    return _id;
}

void Player::SetId(const uint8_t id)
{
    _id = id;
}

void Player::Save()
{
    char PlayerSettingsName[9];
    sprintf(PlayerSettingsName, "player-%u", _id);
    playerPrefs.begin(PlayerSettingsName, false);

    // SerialMon.printf("Player #%u - Namespace: %s\r\n", _id, PlayerSettingsName);

    playerPrefs.putUChar("RED", Player::Color.R);
    playerPrefs.putUChar("GREEN", Player::Color.G);
    playerPrefs.putUChar("BLUE", Player::Color.B);
    playerPrefs.putUChar("WHITE", Player::Color.W);

    // SerialMon.printf("R: %u\tG: %u\tB: %u\tW: %u\r\n", Player::Color.R, Player::Color.G, Player::Color.B, Player::Color.W);

    playerPrefs.end();
    delay(100);

    Serial.printf("Player #%u preferences saved.\r\n", _id);
}

void Player::Load()
{
    char PlayerSettingsName[9];
    sprintf(PlayerSettingsName, "player-%u", _id);
    playerPrefs.begin(PlayerSettingsName, false);

    // SerialMon.printf("Player #%u - Namespace: %s\r\n", _id, PlayerSettingsName);

    Player::Color.R = playerPrefs.getUChar("RED", 0);
    Player::Color.G = playerPrefs.getUChar("GREEN", 0);
    Player::Color.B = playerPrefs.getUChar("BLUE", 0);
    Player::Color.W = playerPrefs.getUChar("WHITE", 0);

    playerPrefs.end();

    // SerialMon.printf("Player: %u\tR: %u\tG: %u\tB: %u\tW: %u\r\n", _id, Player::Color.R, Player::Color.G, Player::Color.B, Player::Color.W);
}

Player::Player(uint8_t newId)
{
    _id = newId;

    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
    {
        GarageTiles[i] = 17 - i + _id * PLAYER_SLICE_OFFSET;
        FinishTiles[i] = 13 - i + _id * PLAYER_SLICE_OFFSET;
    }

    // GarageTiles[0] = 17 + _id * PLAYER_SLICE_OFFSET;
    // GarageTiles[1] = 16 + _id * PLAYER_SLICE_OFFSET;
    // GarageTiles[2] = 15 + _id * PLAYER_SLICE_OFFSET;
    // GarageTiles[3] = 14 + _id * PLAYER_SLICE_OFFSET;

    // FinishTiles[0] = 13 + _id * PLAYER_SLICE_OFFSET;
    // FinishTiles[1] = 12 + _id * PLAYER_SLICE_OFFSET;
    // FinishTiles[2] = 11 + _id * PLAYER_SLICE_OFFSET;
    // FinishTiles[3] = 10 + _id * PLAYER_SLICE_OFFSET;

    HomeTile = HomeTile = 5 + _id * PLAYER_SLICE_OFFSET;
    SerialMon.printf("Player #%u's home tile is #%u.\r\n", _id, HomeTile);
}

Player::Player()
{
}
