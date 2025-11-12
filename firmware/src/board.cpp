// #include "board.h"
// #include "common.h"
// #include "main.h"

// #define ANIMATIONCHANNELS 6

// NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(PIXEL_COUNT, PIXEL_GPIO);

// NeoPixelAnimator animations(ANIMATIONCHANNELS);
// NeoGamma<NeoGammaTableMethod> colorGamma;

// boolean fadeToColor = true;

// struct MyAnimationState
// {
//     RgbwColor StartingColor;
//     RgbwColor EndingColor;
// };

// MyAnimationState animationState[ANIMATIONCHANNELS];

// uint16_t indexPixel = 0;
// uint16_t nextPixel;
// RgbwColor OldColor;

// uint16_t speed = 500;

// //  Gets the next position of a spcific piece based on the player, current position and distance to the next position
// uint16_t GetNextPosition(const uint8_t player, const uint16_t currentPosition, const uint8_t distance)
// {
//     return -1;
// }

// void PieceMovesFadeIn(const AnimationParam &param)
// {
//     RgbwColor updatedColor = RgbwColor::LinearBlend(
//         animationState[param.index].StartingColor,
//         animationState[param.index].EndingColor,
//         param.progress);

//     // apply the color to the strip
//     strip.SetPixelColor(indexPixel, updatedColor);
// }

// void PieceMovesFadeOut(const AnimationParam &param)
// {
//     RgbwColor updatedColor = RgbwColor::LinearBlend(
//         animationState[param.index].StartingColor,
//         animationState[param.index].EndingColor,
//         param.progress);

//     // apply the color to the strip
//     strip.SetPixelColor(nextPixel, updatedColor);
// }

// void MovePiece(RgbwColor myColor)
// {

//     //  Current pixel
//     animationState[0].StartingColor = myColor;
//     animationState[0].EndingColor = OldColor;

//     //  Next pixel
//     animationState[1].StartingColor = strip.GetPixelColor(nextPixel);
//     animationState[1].EndingColor = myColor;

//     OldColor = strip.GetPixelColor(nextPixel);

//     animations.StartAnimation(0, speed, PieceMovesFadeIn);
//     animations.StartAnimation(1, speed, PieceMovesFadeOut);
// }

// //  ###########################
// //  Show/Hide player's pieces #
// //  ###########################

// void ShowPlayerAnimation0(const AnimationParam &param)
// {
//     RgbwColor updatedColor = RgbwColor::LinearBlend(
//         animationState[param.index].StartingColor,
//         animationState[param.index].EndingColor,
//         param.progress);

//     for (size_t i = 0; i < 4; i++)
//         strip.SetPixelColor(players[0].GarageTiles[i], updatedColor);
// }
// void ShowPlayerAnimation1(const AnimationParam &param)
// {
//     RgbwColor updatedColor = RgbwColor::LinearBlend(
//         animationState[param.index].StartingColor,
//         animationState[param.index].EndingColor,
//         param.progress);

//     for (size_t i = 0; i < 4; i++)
//         strip.SetPixelColor(players[1].GarageTiles[i], updatedColor);
// }
// void ShowPlayerAnimation2(const AnimationParam &param)
// {
//     RgbwColor updatedColor = RgbwColor::LinearBlend(
//         animationState[param.index].StartingColor,
//         animationState[param.index].EndingColor,
//         param.progress);

//     for (size_t i = 0; i < 4; i++)
//         strip.SetPixelColor(players[2].GarageTiles[i], updatedColor);
// }
// void ShowPlayerAnimation3(const AnimationParam &param)
// {
//     RgbwColor updatedColor = RgbwColor::LinearBlend(
//         animationState[param.index].StartingColor,
//         animationState[param.index].EndingColor,
//         param.progress);

//     for (size_t i = 0; i < 4; i++)
//         strip.SetPixelColor(players[3].GarageTiles[i], updatedColor);
// }
// void ShowPlayerAnimation4(const AnimationParam &param)
// {
//     RgbwColor updatedColor = RgbwColor::LinearBlend(
//         animationState[param.index].StartingColor,
//         animationState[param.index].EndingColor,
//         param.progress);

//     for (size_t i = 0; i < 4; i++)
//         strip.SetPixelColor(players[4].GarageTiles[i], updatedColor);
// }
// void ShowPlayerAnimation5(const AnimationParam &param)
// {
//     RgbwColor updatedColor = RgbwColor::LinearBlend(
//         animationState[param.index].StartingColor,
//         animationState[param.index].EndingColor,
//         param.progress);

//     for (size_t i = 0; i < 4; i++)
//         strip.SetPixelColor(players[5].GarageTiles[i], updatedColor);
// }



// // AnimUpdateCallback animUpdate = [=](const AnimationParam& param)
// // {
// //     // progress will start at 0.0 and end at 1.0
// //     // we convert to the curve we want
// //     float progress = easing(param.progress);

// //     // use the curve value to apply to the animation
// //     RgbColor updatedColor = RgbColor::LinearBlend(originalColor, targetColor, progress);
// //     strip.SetPixelColor(pixel, updatedColor);
// // };

// // void ShowPlayerAnimation(const AnimationParam &param)
// // {
// //     RgbwColor updatedColor = RgbwColor::LinearBlend(
// //         animationState[param.index].StartingColor,
// //         animationState[param.index].EndingColor,
// //         param.progress);

// //     for (size_t i = 0; i < 4; i++)
// //         strip.SetPixelColor(players[0].GarageTiles[i], updatedColor);
// // }




// void ShowPlayer(uint8_t playerID, bool show)
// {
//     uint16_t indexAnim;
//     if (animations.NextAvailableAnimation(&indexAnim, 1))
//     {
//         if (show)
//         {
//             animationState[indexAnim].StartingColor = RgbwColor(0, 0, 0, 0);
//             animationState[indexAnim].EndingColor = players[playerID].Color;
//         }
//         else
//         {
//             animationState[indexAnim].StartingColor = players[playerID].Color;
//             animationState[indexAnim].EndingColor = RgbwColor(0, 0, 0, 0);
//         }
//         switch (playerID)
//         {
//         case 0:
//             animations.StartAnimation(indexAnim, speed, ShowPlayerAnimation0);
//             break;
//         case 1:
//             animations.StartAnimation(indexAnim, speed, ShowPlayerAnimation1);
//             break;
//         case 2:
//             animations.StartAnimation(indexAnim, speed, ShowPlayerAnimation2);
//             break;
//         case 3:
//             animations.StartAnimation(indexAnim, speed, ShowPlayerAnimation3);
//             break;
//         case 4:
//             animations.StartAnimation(indexAnim, speed, ShowPlayerAnimation4);
//             break;
//         case 5:
//             animations.StartAnimation(indexAnim, speed, ShowPlayerAnimation5);
//             break;

//         default:
//             break;
//         }
//     }
// }

// void ClearBoard()
// {
//     strip.ClearTo(RgbwColor(0, 0, 0, 0));
//     for (size_t i = 0; i < sizeof(FieldLookup) / sizeof(FieldLookup[0]); i++)
//         strip.SetPixelColor(FieldLookup[i], DefaultCellBackgroundColor);
// }

// void setupBoard()
// {

//     strip.Begin();
//     ClearBoard();

//     strip.Show();
// }

// void loopBoard()
// {
//     if (animations.IsAnimating())
//     {
//         animations.UpdateAnimations();
//         strip.Show();
//     }
//     // else
//     // {
//     //     indexPixel++;
//     //     if (indexPixel > PIXEL_COUNT - 1)
//     //         indexPixel = 0;

//     //     nextPixel = indexPixel + 1;
//     //     if (nextPixel > PIXEL_COUNT - 1)
//     //         nextPixel = 0;

//     //     MovePiece(players[0].Color);
//     // }
// }
