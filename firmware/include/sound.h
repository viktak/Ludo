#ifndef SOUND_H
#define SOUND_H

#include "Buzzer.h"

#define BUZZER_GPIO 13

enum Melodies
{
    PlayerActivated = 0,
    PlayerDeactivaed,
    StartGame,
    SelectMove,
    PieceMoves,
    PieceHit,
    PlayerFinished,
    GameFinished
};


void PlayMelody(const Melodies melody);

void setupSound();
void loopSound();

#endif