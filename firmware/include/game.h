#ifndef GAME_H
#define GAME_H

#include <Button2.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

#include "player.h"
#include "gamestatistics.h"

#define BUTTON0_GPIO 42
#define BUTTON1_GPIO 6
#define BUTTON2_GPIO 9
#define BUTTON3_GPIO 11
#define BUTTON4_GPIO 35
#define BUTTON5_GPIO 38

#define LEDSTRIP_GPIO 48 //  The NeoPixel LEDs are connected to this GPIO

#define PIXEL_COUNT 115 //  Total number of pixels (positions) on the board

#define DIE_LENGTH 7 //  The die takes up this many positions / Before that we have the die

#define PLAYER_HOME_POSITION 5 // This is the position where a piece enters the game from the garage

#define NUMBEROFPIECESPERPLAYER 4

#define STARTUPANIMATIONSPEED 400
#define PIECEMOVESANIMATIONSPEED 500
#define ATTACKANIMATIONSPEED 500
#define PLAYERFINISHEDANIMATIONSPEED 200

#define ATTACKHIGHLIGHT_W 255

extern enum GameStages {
    SelectPlayers,
    ConfirmPlayers,
    PlayerRolling,
    PlayerRolled,
    DieRolled,
    EvaluateDie,
    ShowAttack,
    BrowseThreatenedTiles,
    MovePiece,
    EvaluateRound,
    Finished,
    IDLE
} stages;

extern GameStages Stage;
extern uint8_t DieValue;
extern GameStatistics Statistics;

extern char gameID[25];

extern NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip;

const RgbwColor DefaultCellBackgroundColor = RgbwColor(0, 0, 0, 1);
const RgbwColor DarkCellBackgroundColor = RgbwColor(0, 0, 0, 0);
const RgbwColor GarageBackgroundColor = RgbwColor(0, 0, 0, 0);
const RgbwColor FinishBackgroundColor = RgbwColor(0, 0, 0, 0);

const uint16_t DieLookup[] PROGMEM = {
    0, 0, 0, 1, 0, 0, 0, // 1
    0, 0, 1, 0, 1, 0, 0, // 2
    1, 0, 0, 1, 0, 0, 1, // 3
    1, 0, 1, 0, 1, 0, 1, // 4
    1, 0, 1, 1, 1, 0, 1, // 5
    1, 1, 1, 0, 1, 1, 1  // 6
};

const uint16_t FieldLookup[] PROGMEM = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
    54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
    90, 91, 92, 93, 94, 95, 96, 97, 98, 99};

const uint16_t GaragePositions[] PROGMEM = {
    14, 15, 16, 17,
    32, 33, 34, 35,
    50, 51, 52, 53,
    68, 69, 70, 71,
    86, 87, 88, 89,
    104, 105, 106, 107};

const uint16_t FinishPositions[] PROGMEM = {
    10, 11, 12, 13,
    28, 29, 30, 31,
    46, 47, 48, 49,
    64, 65, 66, 67,
    82, 83, 84, 85,
    100, 101, 102, 103};

extern Player Players[NUMBEROFPLAYERS];

extern uint8_t CurrentPlayerID;

int16_t GetNextPositionByPiece(const uint8_t PlayerID, const uint16_t piece);
int8_t GetNextPossibleMove();
int8_t GetPlayerByPosition(const uint16_t pos);
int8_t GetPieceByPosition(const uint16_t pos);
void ShowAttackingPieces();
uint8_t CalculatePossibleMoves(const uint8_t playerID, const uint8_t dia);
void TogglePlayerVisibility(const uint8_t playerID);
void SetNextPlayer(const bool randomPlayerNeeded = false);
int16_t GetNextPosition(const uint8_t playerID, const int16_t pos);
int16_t GetNextPositionByPiece(const uint8_t playerID, const uint16_t piece);
int8_t GetLastPieceInGarage(const uint8_t playerID);
bool IsPlayerFinished(const uint8_t playerID);
void MovePieceByNumber(const uint8_t playerID, const uint16_t piece, const uint8_t die);

uint8_t GetNumberOfActivePlayers();

void RefreshPieces(const uint8_t playerID);

void StartNewGame();
void ClearBoard();
void setupGame();
void loopGame();

#endif
