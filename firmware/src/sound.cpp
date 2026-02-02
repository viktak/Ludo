#include <Arduino.h>

#include "sound.h"
#include "main.h"

Buzzer::Melody_t mStartup{.nbNotes = 3, .duration = {200, 200, 200}, .frequency = {E4_NOTE_FREQ, G4_NOTE_FREQ, C5_NOTE_FREQ}};
Buzzer::Melody_t mPlayerActivated{.nbNotes = 2, .duration = {100, 100}, .frequency = {G4_NOTE_FREQ, C5_NOTE_FREQ}};
Buzzer::Melody_t mPlayerDeactivated{.nbNotes = 2, .duration = {100, 100}, .frequency = {C5_NOTE_FREQ, G4_NOTE_FREQ}};
Buzzer::Melody_t mStartGame{.nbNotes = 6, .duration = {200, 200, 200, 400, 200, 600}, .frequency = {C4_NOTE_FREQ, E4_NOTE_FREQ, G4_NOTE_FREQ, C5_NOTE_FREQ, G4_NOTE_FREQ, C5_NOTE_FREQ}};
Buzzer::Melody_t mSelectMove{.nbNotes = 1, .duration = {50}, .frequency = {C7_NOTE_FREQ}};
Buzzer::Melody_t mPieceMoves{.nbNotes = 1, .duration = {50}, .frequency = {C6_NOTE_FREQ}};
Buzzer::Melody_t mPlayerFinished{.nbNotes = 8, .duration = {100, 100, 100, 100, 100, 100, 100, 100}, .frequency = {C4_NOTE_FREQ, D4_NOTE_FREQ, E5_NOTE_FREQ, F5_NOTE_FREQ, G5_NOTE_FREQ, A5_NOTE_FREQ, A5_NOTE_FREQ, C6_NOTE_FREQ}};
Buzzer::Melody_t mPieceHit{.nbNotes = 3, .duration = {100, 100, 100}, .frequency = {C6_NOTE_FREQ, G5_NOTE_FREQ, C5_NOTE_FREQ}};
// Buzzer::Melody_t mPlayer{.nbNotes = 3, .duration = {200, 200, 200}, .frequency = {E4_NOTE_FREQ, G4_NOTE_FREQ, C5_NOTE_FREQ}};
// Buzzer::Melody_t mPlayer{.nbNotes = 3, .duration = {200, 200, 200}, .frequency = {E4_NOTE_FREQ, G4_NOTE_FREQ, C5_NOTE_FREQ}};
// Buzzer::Melody_t mPlayer{.nbNotes = 3, .duration = {200, 200, 200}, .frequency = {E4_NOTE_FREQ, G4_NOTE_FREQ, C5_NOTE_FREQ}};
// Buzzer::Melody_t mPlayer{.nbNotes = 3, .duration = {200, 200, 200}, .frequency = {E4_NOTE_FREQ, G4_NOTE_FREQ, C5_NOTE_FREQ}};

Buzzer myBuzzer;

void PlayMelody(const Melodies melody)
{
    if (appSettings.useBeeper)
    {
        switch (melody)
        {
        case Melodies::PlayerActivated:
            myBuzzer.setMelody(&mPlayerActivated);
            break;
        case Melodies::PlayerDeactivaed:
            myBuzzer.setMelody(&mPlayerDeactivated);
            break;
        case Melodies::StartGame:
            myBuzzer.setMelody(&mStartGame);
            break;
        case Melodies::SelectMove:
            myBuzzer.setMelody(&mSelectMove);
            break;
        case Melodies::PieceMoves:
            myBuzzer.setMelody(&mPieceMoves);
            break;
        case Melodies::PieceHit:
            myBuzzer.setMelody(&mPieceHit);
            break;
        case Melodies::PlayerFinished:
            myBuzzer.setMelody(&mPlayerFinished);
            break;

        default:
            break;
        }
    }
}

void setupSound()
{
    myBuzzer.init(BUZZER_GPIO);
    myBuzzer.setMelody(&mStartup);
}

void loopSound()
{
    myBuzzer.step();
}