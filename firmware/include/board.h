// #ifndef BOARD_H
// #define BOARD_H

// #include <NeoPixelBus.h>
// #include <NeoPixelAnimator.h>

// #include "player.h"

// #define PIXEL_GPIO 35 //  The NeoPixel LEDs are connected to this GPIO

// #define PIXEL_COUNT 108 //  Total number of pixels (positions) on the board

// #define DIE_LENGTH 0 //  The die takes up this many positions / Before that we have the die

// #define PLAYER_SLICE_OFFSET 18 //  Each player takes up 18 spaces
// #define GARAGE_OFFSET 10       //  Start of garage positions
// #define GOAL_OFFSET 14         //  End of goal positions (i.e. it is in reverse order!!!)
// #define PLAYER_HOME_POSITION 6 // This is the position where a piece enters the game from the garage

// extern NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip;

// const RgbwColor DefaultCellBackgroundColor = RgbwColor(0, 0, 0, 1);

// const uint8_t DieLookup[6][7] = {
//     {0, 0, 0, 1, 0, 0, 0}, // 1
//     {0, 0, 1, 0, 1, 0, 0}, // 2
//     {1, 0, 0, 1, 0, 0, 1}, // 3
//     {1, 0, 1, 0, 1, 0, 1}, // 4
//     {1, 0, 1, 1, 1, 0, 1}, // 5
//     {1, 1, 1, 0, 1, 1, 1}  // 6
// };

// const uint8_t FieldLookup[] = {
//     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
//     18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
//     36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
//     54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
//     72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
//     90, 91, 92, 93, 94, 95, 96, 97, 98, 99};

// void ShowPlayer(uint8_t playerID, bool show);

// void setupBoard();
// void loopBoard();

// #endif
