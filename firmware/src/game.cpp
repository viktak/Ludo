#include "game.h"
#include "main.h"
#include "mqtt.h"
#include "sound.h"

#include <TimeLib.h>

#define ANIMATIONCHANNELS NUMBEROFPLAYERS *NUMBEROFPIECESPERPLAYER

#define DARKENING_FACTOR .6

NeoPixelBus<NeoGrbwFeature, Neo800KbpsMethod> strip(PIXEL_COUNT, LEDSTRIP_GPIO);

NeoPixelAnimator animations(ANIMATIONCHANNELS);
NeoGamma<NeoGammaTableMethod> colorGamma;

Button2 Buttons[NUMBEROFPLAYERS];

GameStages Stage;
GameStatistics Statistics;

char gameID[25];

Player Players[6] = {
    Player(0),
    Player(1),
    Player(2),
    Player(3),
    Player(4),
    Player(5)};

struct MyAnimationState
{
    RgbwColor StartingColor;
    RgbwColor EndingColor;
    uint8_t PlayerID;
    uint8_t PieceID;
    bool dir;
};

MyAnimationState animationStates[ANIMATIONCHANNELS];

RgbwColor oldColor[NUMBEROFPIECESPERPLAYER] = {RgbwColor(0, 0, 0, 0), RgbwColor(0, 0, 0, 0), RgbwColor(0, 0, 0, 0), RgbwColor(0, 0, 0, 0)}; //  Holds the color of a tile before any flashing animation started so that we can return to the same color any time.
RgbwColor OldColor;

unsigned long oldMillis = 0;
unsigned long DelayBeforeRolling = 0;

uint8_t MyCounter = 0;
uint8_t DieValue = 0;
uint8_t CurrentPlayerID = 0;
int8_t SelectedPieceID = -1;
uint8_t garagePos = 0;

int16_t attackedTiles[NUMBEROFPIECESPERPLAYER]; //  This holds the positions attacked

bool animDir = true;
bool JustStarting = false;
bool PlayerFinishedRolling = false;

void PrintColor(const RgbwColor col)
{
    SerialMon.printf("RGBW color: %u:%u:%u:%u\r\n", col.R, col.G, col.B, col.W);
}

void PrintPlayerPositions(const uint8_t playerID)
{
    SerialMon.printf("Player: %u\r\n", playerID);
    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
        SerialMon.printf("Piece: %u\tPosition: %u\r\n", i, Players[playerID].Pieces[i].GetPosition());
}

void PrintAttackedTiles()
{
    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
        SerialMon.printf("%i\t", attackedTiles[i]);
    SerialMon.println();
}

void PrintPossibleMoves()
{
    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
        SerialMon.printf("Piece: %u\tPosition: %u\tAttacking: %i\r\n", i, Players[CurrentPlayerID].Pieces[i].GetPosition(), attackedTiles[i]);
}

void PrintPlayerProperties(const uint8_t playerID)
{
    SerialMon.printf("Player: %u\tActive: %s\tFinished: %s\r\n", playerID, Players[playerID].IsActive ? "yes" : "no", Players[playerID].IsFinished ? "yes" : "no");
}

void DoStartupAnimation()
{
    bool isFinished = false;
    static uint8_t v = 0;

    AnimUpdateCallback FadeAnimation = [=](const AnimationParam &param)
    {
        RgbwColor updatedColor = RgbwColor::LinearBlend(animationStates[param.index].StartingColor, animationStates[param.index].EndingColor, param.progress);

        for (size_t i = 0; i < DIE_LENGTH; i++)
            if (DieLookup[((v - 1) / 2) * DIE_LENGTH + i] == 1)
                strip.SetPixelColor(i, updatedColor);
    };

    RgbwColor randomColor;

    while (!isFinished)
    {
        if (animations.IsAnimating())
        {
            animations.UpdateAnimations();
            strip.Show();
        }
        else
        {
            if (v < 12)
            {
                uint16_t indexAnim;

                if (animations.NextAvailableAnimation(&indexAnim, 0))
                {
                    if (v % 2 == 0)
                    {
                        randomColor = RgbwColor(random(255), random(255), random(255), 0);
                        animationStates[indexAnim].StartingColor = RgbwColor(0, 0, 0, 0);
                        animationStates[indexAnim].EndingColor = randomColor;
                    }
                    else
                    {
                        animationStates[indexAnim].StartingColor = randomColor;
                        animationStates[indexAnim].EndingColor = RgbwColor(0, 0, 0, 0);
                    }
                    animations.StartAnimation(indexAnim, 200, FadeAnimation);
                }
                v++;
            }
            else
                isFinished = true;
        }
    }
}

void DoPlayerFinishedAnimation(const uint8_t playerID)
{
    bool isFinished = false;
    uint8_t v = 0;

    AnimUpdateCallback FadeAnimation = [=](const AnimationParam &param)
    {
        RgbwColor updatedColor = RgbwColor::LinearBlend(animationStates[param.index].StartingColor, animationStates[param.index].EndingColor, param.progress);

        strip.SetPixelColor(Players[playerID].Pieces[param.index].GetPosition() + DIE_LENGTH, updatedColor);
    };

    while (!isFinished)
    {
        if (animations.IsAnimating())
        {
            animations.UpdateAnimations();
            strip.Show();
        }
        else
        {
            if (v < 16)
            {
                for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
                {
                    HslColor myColor = (RgbColor)Players[playerID].Color;
                    myColor.L *= (float)random(2, 10) / 10;

                    animationStates[i].StartingColor = strip.GetPixelColor(Players[playerID].Pieces[i].GetPosition() + DIE_LENGTH);
                    animationStates[i].EndingColor = myColor;
                    animations.StartAnimation(i, random(100, 200), FadeAnimation);
                }
                v++;
            }
            else
            {
                RefreshPieces(playerID);
                isFinished = true;
            }
        }
    }
}

bool IsPositionInGarage(const uint8_t playerID, const uint16_t pos)
{
    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
        if (GaragePositions[NUMBEROFPIECESPERPLAYER * playerID + i] == pos)
            return true;
    return false;
}

bool IsPositionInFinish(const uint8_t playerID, const uint16_t pos)
{
    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
        if (FinishPositions[NUMBEROFPIECESPERPLAYER * playerID + i] == pos)
            return true;
    return false;
}

//  Returns the number of (active) players in the current game
uint8_t GetNumberOfActivePlayers()
{
    uint8_t numberOfPlayers = 0;
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        if (Players[i].IsActive)
            numberOfPlayers++;

    return numberOfPlayers;
}

//  Returns the ID of a player who has a piece at a specified position or -1, if the position is empty
int8_t GetPlayerByPosition(const uint16_t pos)
{
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        // if (Players[i].IsActive)
        for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
            if (Players[i].Pieces[j].GetPosition() == pos)
                return (int8_t)i;
    return -1;
}

//  Returns the ID of a piece at a specified position or -1, if the position is empty
int8_t GetPieceByPosition(const uint16_t pos)
{
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
            if (Players[i].Pieces[j].GetPosition() == pos)
                return (int8_t)j;
    return -1;
}

//  Gets the next position of a specific position based on the player
int16_t GetNextPosition(const uint8_t playerID, const int16_t pos)
{
    // strip.SetPixelColor(pos, RgbwColor(0, 0, 0, 255));
    int16_t newPos = -1;

    if (IsPositionInGarage(playerID, pos))
        newPos = PLAYER_SLICE_OFFSET * playerID + PLAYER_HOME_POSITION;
    else if (IsPositionInFinish(playerID, pos))
        if (pos > PLAYER_SLICE_OFFSET * playerID + 10)
            newPos = pos - 1;
        else
            newPos = -1;
    else if (pos == PLAYER_SLICE_OFFSET * playerID + NUMBEROFPIECESPERPLAYER)
        newPos = FinishPositions[playerID * NUMBEROFPIECESPERPLAYER + 3];
    else if (pos % PLAYER_SLICE_OFFSET == 9)
        newPos = (pos + 9) % (NUMBEROFPLAYERS * PLAYER_SLICE_OFFSET);
    else
        newPos = (pos + 1) % (NUMBEROFPLAYERS * PLAYER_SLICE_OFFSET);

    return newPos;
}

//  Gets the next position of a specific piece of a player
int16_t GetNextPositionByPiece(const uint8_t playerID, const uint16_t piece)
{
    uint16_t newPos = -1;

    if (IsPositionInGarage(playerID, Players[playerID].Pieces[piece].GetPosition()))
        newPos = PLAYER_SLICE_OFFSET * playerID + PLAYER_HOME_POSITION;
    else if (IsPositionInFinish(playerID, Players[playerID].Pieces[piece].GetPosition()))
        if (Players[playerID].Pieces[piece].GetPosition() > PLAYER_SLICE_OFFSET * playerID + 10)
            newPos = Players[playerID].Pieces[piece].GetPosition() - 1;
        else
            newPos = -1;
    else if (Players[playerID].Pieces[piece].GetPosition() == PLAYER_SLICE_OFFSET * playerID + NUMBEROFPIECESPERPLAYER)
        newPos = FinishPositions[playerID * NUMBEROFPIECESPERPLAYER + 3];
    else if (Players[playerID].Pieces[piece].GetPosition() % PLAYER_SLICE_OFFSET == 9)
        newPos = (Players[playerID].Pieces[piece].GetPosition() + 9) % (NUMBEROFPLAYERS * PLAYER_SLICE_OFFSET);
    else
        newPos = (Players[playerID].Pieces[piece].GetPosition() + 1) % (NUMBEROFPLAYERS * PLAYER_SLICE_OFFSET);

    // SerialMon.printf("GetNextPositionByPiece: Player: %u\tPiece: %u\tNewPos: %i\r\n", playerID, piece, newPos);

    return newPos;
}

//  Gets the next available piece in garage - returns -1 if no pieces are in the garage
int8_t GetLastPieceInGarage(const uint8_t playerID)
{
    int8_t lastPiece = -1;
    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
    {
        int8_t myPos = GetPieceByPosition(GaragePositions[playerID * NUMBEROFPIECESPERPLAYER + i]);
        // SerialMon.printf("Position: %u\tPiece: %i\r\n", GaragePositions[playerID * NUMBEROFPIECESPERPLAYER + i], myPos);
        if (myPos > -1)
            lastPiece = myPos;
    }
    return lastPiece;
}

//  Gets the next available space in garage - returns -1 if garage is fulle
int8_t GetNextSpaceInGarage(const uint8_t playerID)
{
    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
        if (GetPlayerByPosition(GaragePositions[playerID * NUMBEROFPIECESPERPLAYER + i]) < 0)
            return GaragePositions[playerID * NUMBEROFPIECESPERPLAYER + i];
    return -1;
}

//  Calcultes the target tile after the die rolls
uint8_t CalculatePossibleMoves(const uint8_t playerID, const uint8_t die)
{
    uint8_t numberOfPossibleMoves = 0;
    DieValue = die;

    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
    {
        attackedTiles[i] = -1;
        int16_t pos = Players[playerID].Pieces[i].GetPosition();

        if (IsPositionInGarage(playerID, pos)) //  Pieces in the garage can only enter the field if the player rolled 6.
            if (die == 6 && i == GetLastPieceInGarage(playerID))
                pos = PLAYER_HOME_POSITION + playerID * PLAYER_SLICE_OFFSET;
            else
                pos = -1;
        else
        {
            bool isInFinish = IsPositionInFinish(playerID, pos);
            for (size_t i = 0; i < die; i++)
            {
                pos = GetNextPosition(playerID, (uint16_t)pos);
                isInFinish = isInFinish || IsPositionInFinish(playerID, pos);
            }
            if (isInFinish && !IsPositionInFinish(playerID, pos))
                pos = -1;
        }

        //  If the new tile already has the player's piece, it cannot move there.
        for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
            if (Players[playerID].Pieces[j].GetPosition() == pos)
                pos = -1;

        //  RULE:   If the piece is in it's home position (i.e. where it enters the game) it cannot be hit by enyone
        if (appSettings.rules.IsHomeSafe)
            if (pos == PLAYER_HOME_POSITION + GetPlayerByPosition(pos) * PLAYER_SLICE_OFFSET)
                pos = -1;

        attackedTiles[i] = pos;
        if (pos > -1)
            numberOfPossibleMoves++;
    }
    // PrintPossibleMoves();
    return numberOfPossibleMoves;
}

//  Show pieces that can move
void ShowAttackingPieces()
{
    SerialMon.println("Attacking");
}

//  Clears all tiles and sets the main course tiles to their default background color
void ClearBoard()
{
    strip.ClearTo(RgbwColor(0, 0, 0, 0));

    //  Main field
    for (size_t i = 0; i < sizeof(FieldLookup) / sizeof(FieldLookup[0]); i++)
        strip.SetPixelColor(FieldLookup[i] + DIE_LENGTH, DefaultCellBackgroundColor);

    //  Finish positions
    for (size_t i = 0; i < sizeof(FinishPositions) / sizeof(FinishPositions[0]); i++)
        strip.SetPixelColor(FinishPositions[i] + DIE_LENGTH, DefaultCellBackgroundColor);
}

//  Show/Hide player's pieces
void TogglePlayerVisibility(const uint8_t playerID)
{
    AnimUpdateCallback ShowPlayerAnimation = [=](const AnimationParam &param)
    {
        RgbwColor updatedColor = RgbwColor::LinearBlend(
            animationStates[param.index].StartingColor,
            animationStates[param.index].EndingColor,
            param.progress);

        for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
            strip.SetPixelColor(Players[playerID].Pieces[i].GetPosition() + DIE_LENGTH, updatedColor);
    };

    uint16_t indexAnim;
    if (animations.NextAvailableAnimation(&indexAnim, 0))
    {
        if (Players[playerID].IsActive)
        {
            animationStates[indexAnim].StartingColor = RgbwColor(0, 0, 0, 0);
            animationStates[indexAnim].EndingColor = Players[playerID].Color;
        }
        else
        {
            animationStates[indexAnim].StartingColor = Players[playerID].Color;
            animationStates[indexAnim].EndingColor = RgbwColor(0, 0, 0, 0);
        }
        animations.StartAnimation(indexAnim, STARTUPANIMATIONSPEED, ShowPlayerAnimation);
    }
}

//  Returns true if all pieces of a player are in the finish positions
bool IsPlayerFinished(const uint8_t playerID)
{
    bool isFinished = true;
    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
        isFinished &= IsPositionInFinish(playerID, Players[playerID].Pieces[i].GetPosition());

    if (isFinished)
    {
        SerialMon.printf("Player %u is finished! Well done.\r\n", playerID);

        PlayMelody(Melodies::PlayerFinished);
        Statistics.Player[playerID].FinishedMillis = millis();
        DoPlayerFinishedAnimation(playerID);
    }

    return isFinished;
}

//  Get the next possible move or -1 if there is none
int8_t GetNextPossibleMove()
{
    size_t i = 0;
    while (i < NUMBEROFPIECESPERPLAYER)
    {
        SelectedPieceID = (SelectedPieceID + 1) % NUMBEROFPIECESPERPLAYER;
        if (attackedTiles[SelectedPieceID] > -1)
            return SelectedPieceID;
        i++;
    }
    return -1;
}

//  Moves a pieces by [die] amount forward
void MovePieceByNumber(const uint8_t playerID, const uint16_t piece, const uint8_t die)
{
    // SerialMon.printf("Player: %u\tPiece: %u\tDie: %u\tCurrentPos: %u\tAttackedPos: %i\r\n", playerID, piece, die, Players[playerID].Pieces[piece].GetPosition(), attackedTiles[piece]);

    SelectedPieceID = piece;
    CurrentPlayerID = playerID;
    animDir = true;
    DieValue = die;
    MyCounter = 0;
    Stage = GameStages::MovePiece;
}

//  Gets the number of players staill in the game (Active AND not Finished)
uint8_t GetNumberOfPlayersStillPlaying()
{
    uint8_t myCounter = 0;
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        if (Players[i].IsActive && !Players[i].IsFinished)
            myCounter++;
    return myCounter;
}

//  Advances the current player to the next active player
void SetNextPlayer(const bool randomPlayerNeeded)
{
    if (randomPlayerNeeded)
    {
        do
        {
            uint8_t p = random(NUMBEROFPLAYERS);
            if (Players[p].IsActive)
            {
                CurrentPlayerID = Players[p].GetId();
                return;
            }
        } while (true);
    }
    else
        do
        {
            CurrentPlayerID = (CurrentPlayerID + 1) % NUMBEROFPLAYERS;
            if (Players[CurrentPlayerID].IsActive && !Players[CurrentPlayerID].IsFinished)
                return;
        } while (true);
}

//  Redraws all the pieces of the given player
void RefreshPieces(const uint8_t playerID)
{
    if (Players[playerID].IsActive)
    {
        // SerialMon.printf("Player: #%u\r\n", playerID);
        for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
            strip.SetPixelColor(Players[playerID].Pieces[i].GetPosition() + DIE_LENGTH, Players[playerID].Color);
    }
    strip.Show();
}

void StartNewGame()
{
    ClearBoard();
    Statistics.Reset();
    Stage = GameStages::PlayerRolling;
}

/// ##################################################################################################
///     Button handlers
/// ##################################################################################################

void buttonClickHandler(Button2 &btn)
{
    SerialMon.printf("%u clicked\r\n", btn.getID());
    if (Stage == GameStages::SelectPlayers)
    {
        uint8_t playerID = (uint8_t)btn.getID();
        Players[playerID].IsActive = !Players[playerID].IsActive;
        TogglePlayerVisibility(playerID);
        PlayMelody(Players[playerID].IsActive ? Melodies::PlayerActivated : Melodies::PlayerDeactivaed);
    }

    if (Stage == GameStages::PlayerRolling)
    {
        if (((uint8_t)btn.getID() == CurrentPlayerID) && millis() > DelayBeforeRolling + 100) //  The delay is needed to avoid a bug where if a player stops the die prematurely, it wn't roll at all
            PlayerFinishedRolling = true;
    }

    if (Stage == GameStages::ShowAttack)
    {
        if (btn.getID() == CurrentPlayerID)
        {
            animations.StopAll();
            if (appSettings.rules.AssistedMode)
                for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
                    strip.SetPixelColor(attackedTiles[i] + DIE_LENGTH, oldColor[i]);
            else
                for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
                    strip.SetPixelColor(Players[CurrentPlayerID].Pieces[i].GetPosition() + DIE_LENGTH, Players[CurrentPlayerID].Color);
            strip.Show();
            animDir = true;

            Stage = GameStages::BrowseThreatenedTiles;

            //  Play tune
            PlayMelody(Melodies::SelectMove);
        }
    }

    if (Stage == GameStages::BrowseThreatenedTiles)
    {
        if (btn.getID() == CurrentPlayerID)
        {
            //  Stop any ongoing animation and restore the pixel color
            animations.StopAll();
            if (appSettings.rules.AssistedMode)
                strip.SetPixelColor(attackedTiles[SelectedPieceID] + DIE_LENGTH, oldColor[SelectedPieceID]);
            else
                strip.SetPixelColor(Players[CurrentPlayerID].Pieces[SelectedPieceID].GetPosition() + DIE_LENGTH, Players[CurrentPlayerID].Color);
            strip.Show();

            SelectedPieceID = GetNextPossibleMove();
            animDir = true;

            //  Play tune
            PlayMelody(Melodies::SelectMove);
        }
    }
}

void buttonDoubleClickHandler(Button2 &btn)
{
    SerialMon.printf("%u double clicked\r\n", btn.getID());
}

void buttonTripleClickHandler(Button2 &btn)
{
    SerialMon.printf("%u triple clicked\r\n", btn.getID());
}

void buttonLongClickHandler(Button2 &btn)
{
    SerialMon.printf("%i long clicked\r\n", btn.getID());

    if (Stage == GameStages::SelectPlayers)
    {
        bool arePlayersReady = true;
        uint8_t pc = 0;

        //  Count how many players are activated
        for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
            if (Players[i].IsActive)
                pc++;

        //  The game can only start, if all activated players confirm
        for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
            if (Players[i].IsActive && !Buttons[i].isPressed())
                arePlayersReady = false;

        if (arePlayersReady)
        {
            SerialMon.printf("Number of players selected: %u\r\n", pc);
            if (pc > 1)
            {
                SerialMon.printf("\r\nTotal: %u Players in this game.\r\n", pc);
                MyCounter = 0;
                Stage = GameStages::ConfirmPlayers;
                Statistics.Reset();
            }
            else
                SerialMon.println("Add more Players. You need at least 2 players for a game.");
        }
        else
            SerialMon.println("All selected players must press their buttons at the same time!");
    }

    if (Stage == GameStages::BrowseThreatenedTiles)
    {
        if (btn.getID() == CurrentPlayerID)
        {
            //  Stop any ongoing animation and restore the pixel color
            animations.StopAll();
            if (appSettings.rules.AssistedMode)
                strip.SetPixelColor(attackedTiles[SelectedPieceID] + DIE_LENGTH, oldColor[SelectedPieceID]);
            strip.Show();

            //  Move to the next stage of the game
            animDir = true;
            MyCounter = 0;
            JustStarting = true;
            Stage = GameStages::MovePiece;
        }
    }
}

/// ##################################################################################################
/// ##################################################################################################
/// ##################################################################################################

void InitButtons()
{
    Buttons[0].begin(BUTTON0_GPIO);
    Buttons[1].begin(BUTTON1_GPIO);
    Buttons[2].begin(BUTTON2_GPIO);
    Buttons[3].begin(BUTTON3_GPIO);
    Buttons[4].begin(BUTTON4_GPIO);
    Buttons[5].begin(BUTTON5_GPIO);

    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
    {
        Buttons[i].setClickHandler(buttonClickHandler);
        Buttons[i].setDoubleClickHandler(buttonDoubleClickHandler);
        Buttons[i].setTripleClickHandler(buttonTripleClickHandler);
        Buttons[i].setLongClickDetectedHandler(buttonLongClickHandler);
    }
}

void InitLEDStrip()
{
    strip.Begin();
}

void InitPlayers()
{
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        Players[i].Load();

    //  Move all pieces to the garage
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
            Players[i].Pieces[j].SetPosition(Players[i].GarageTiles[j]);
}

void setupGame()
{
    InitButtons();
    InitLEDStrip();
    InitPlayers();

    DoStartupAnimation();

    ClearBoard();
    delay(10); //  This is needed to avoid some garbage appear on the strip
    strip.Show();

    //  Create a new game
    StartNewGame();

    //  Start player activation
    Stage = GameStages::SelectPlayers;
}

void loopGame()
{

    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        Buttons[i].loop();

    if (animations.IsAnimating())
    {
        animations.UpdateAnimations();
        strip.Show();
    }
    else
    {
        switch (Stage)
        {
        case GameStages::SelectPlayers:
        {
            break;
        }

        case GameStages::ConfirmPlayers:
        {
            //  Play tune
            PlayMelody(Melodies::StartGame);

            SerialMon.println("Let the game begin!");
            SetNextPlayer(true); //  A random player will start rolling
            SerialMon.printf("Player #%u begins the game.\r\n", CurrentPlayerID);
            PlayerFinishedRolling = false;
            Statistics.StartMillis = millis();
            Stage = GameStages::PlayerRolling;
            break;
        }

        case GameStages::PlayerRolling:
        {
            if (Statistics.Round < 1)
                sprintf(gameID, "%u%02u%02u%02u%02u%02u-%lu", year(), month(), day(), hour(), minute(), second(), millis());

            if (PlayerFinishedRolling)
            {
                SerialMon.printf("Player #%u rolled %u.\r\n", CurrentPlayerID, DieValue);

                MyCounter = 0;
                Stage = GameStages::PlayerRolled;
            }
            else
            {
                if (millis() - oldMillis > 100)
                {
                    uint8_t c = SerialMon.read() - 48;
                    if ((c > 0) && (c < 7))
                    {
                        DieValue = c;
                        PlayerFinishedRolling = true;
                    }
                    else
                        DieValue = (uint8_t)random(6) + 1;
                    for (size_t i = 0; i < DIE_LENGTH; i++)
                    {
                        if (DieLookup[(DieValue - 1) * DIE_LENGTH + i] == 1)
                            strip.SetPixelColor(i, Players[CurrentPlayerID].Color);
                        else
                            strip.SetPixelColor(i, DarkCellBackgroundColor);
                        strip.Show();
                    }
                    oldMillis = millis();
                }
            }
            break;
        }

        case GameStages::PlayerRolled:
        {
            uint8_t possibleMoves = CalculatePossibleMoves(CurrentPlayerID, DieValue);
            if (possibleMoves > 0)
            {
                //  Update statistics
                Statistics.Player[CurrentPlayerID].Rolls[DieValue - 1]++;
                Statistics.Player[CurrentPlayerID].TotalPips += DieValue;

                if (appSettings.rules.AssistedMode)
                {
                    //  Save colors of attacked tiles for later use
                    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
                        oldColor[i] = strip.GetPixelColor(attackedTiles[i] + DIE_LENGTH);

                    // PrintAttackedTiles();
                }
                else
                {
                    //  Save colors of attacked tiles for later use
                    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
                    {
                        if (IsPositionInFinish)
                            oldColor[i] = FinishBackgroundColor;
                        else if (IsPositionInGarage)
                            oldColor[i] = GarageBackgroundColor;
                        else
                            oldColor[i] = DefaultCellBackgroundColor;
                    }
                }

                animDir = true;
                SelectedPieceID = GetNextPossibleMove();

                if (possibleMoves > 1)
                    Stage = GameStages::ShowAttack;
                else
                    Stage = GameStages::BrowseThreatenedTiles;
            }
            else
            {
                if (millis() - oldMillis > 100)
                {
                    if (MyCounter % 2 == 0)
                        for (size_t i = 0; i < DIE_LENGTH; i++)
                        {
                            if (DieLookup[(DieValue - 1) * DIE_LENGTH + i] == 1)
                                strip.SetPixelColor(i, Players[CurrentPlayerID].Color);
                            else
                                strip.SetPixelColor(i, DarkCellBackgroundColor);
                        }
                    else
                        for (size_t i = 0; i < DIE_LENGTH; i++)
                            strip.SetPixelColor(i, DarkCellBackgroundColor);
                    strip.Show();
                    MyCounter++;

                    oldMillis = millis();
                }

                if (MyCounter > 16) //  Move on
                {
                    //  Update statistics
                    Statistics.Player[CurrentPlayerID].Rolls[DieValue - 1]++;
                    Statistics.Player[CurrentPlayerID].EmptyRolls[DieValue - 1]++;

                    Statistics.Player[CurrentPlayerID].TotalPips += DieValue;
                    Statistics.Player[CurrentPlayerID].EmptyPips += DieValue;

                    Stage = GameStages::EvaluateRound;
                }
            }
            break;
        }

        case GameStages::ShowAttack:
        {
            AnimUpdateCallback attackAnimation = [=](const AnimationParam &param)
            {
                RgbwColor updatedColor = RgbwColor::LinearBlend(animationStates[param.index].StartingColor, animationStates[param.index].EndingColor, param.progress);
                if (appSettings.rules.AssistedMode)
                    strip.SetPixelColor((uint16_t)attackedTiles[param.index] + DIE_LENGTH, updatedColor);
                else
                    for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
                        if (attackedTiles[i] > -1)
                            strip.SetPixelColor(Players[CurrentPlayerID].Pieces[i].GetPosition() + DIE_LENGTH, updatedColor);
            };

            for (size_t i = 0; i < NUMBEROFPIECESPERPLAYER; i++)
                if (attackedTiles[i] > -1)
                {
                    uint16_t indexAnim;
                    if (animations.NextAvailableAnimation(&indexAnim, 0))
                    {
                        if (appSettings.rules.AssistedMode)
                        {
                            if (animDir)
                            {
                                animationStates[i].StartingColor = oldColor[i];
                                animationStates[i].EndingColor = RgbwColor(strip.GetPixelColor(attackedTiles[i] + DIE_LENGTH).R, strip.GetPixelColor(attackedTiles[i] + DIE_LENGTH).G, strip.GetPixelColor(attackedTiles[i] + DIE_LENGTH).B, ATTACKHIGHLIGHT_W);
                            }
                            else
                            {
                                animationStates[i].StartingColor = strip.GetPixelColor(attackedTiles[i] + DIE_LENGTH);
                                animationStates[i].EndingColor = oldColor[i];
                            }
                        }
                        else
                        {
                            if (animDir)
                            {
                                animationStates[i].StartingColor = Players[CurrentPlayerID].Color;
                                animationStates[i].EndingColor = oldColor[i];
                            }
                            else
                            {
                                animationStates[i].StartingColor = oldColor[i];
                                animationStates[i].EndingColor = Players[CurrentPlayerID].Color;
                            }
                        }
                        animations.StartAnimation(i, ATTACKANIMATIONSPEED, attackAnimation);
                    }
                }
            animDir = !animDir;
            break;
        }

        case GameStages::BrowseThreatenedTiles:
        {
            AnimUpdateCallback attackAnimation = [=](const AnimationParam &param)
            {
                RgbwColor updatedColor = RgbwColor::LinearBlend(animationStates[param.index].StartingColor, animationStates[param.index].EndingColor, param.progress);
                if (appSettings.rules.AssistedMode)
                    strip.SetPixelColor((uint16_t)attackedTiles[param.index] + DIE_LENGTH, updatedColor);
                else
                    strip.SetPixelColor(Players[CurrentPlayerID].Pieces[SelectedPieceID].GetPosition() + DIE_LENGTH, updatedColor);
            };

            uint16_t indexAnim;
            if (animations.NextAvailableAnimation(&indexAnim, 0))
            {
                if (appSettings.rules.AssistedMode)
                {
                    if (animDir)
                    {
                        animationStates[SelectedPieceID].StartingColor = oldColor[SelectedPieceID];
                        animationStates[SelectedPieceID].EndingColor = RgbwColor(strip.GetPixelColor(attackedTiles[SelectedPieceID] + DIE_LENGTH).R, strip.GetPixelColor(attackedTiles[SelectedPieceID] + DIE_LENGTH).G, strip.GetPixelColor(attackedTiles[SelectedPieceID] + DIE_LENGTH).B, ATTACKHIGHLIGHT_W);
                    }
                    else
                    {
                        animationStates[SelectedPieceID].StartingColor = strip.GetPixelColor(attackedTiles[SelectedPieceID] + DIE_LENGTH);
                        animationStates[SelectedPieceID].EndingColor = oldColor[SelectedPieceID];
                    }
                }
                else
                {
                    if (animDir)
                    {
                        animationStates[SelectedPieceID].StartingColor = Players[CurrentPlayerID].Color;
                        animationStates[SelectedPieceID].EndingColor = oldColor[SelectedPieceID];
                    }
                    else
                    {
                        animationStates[SelectedPieceID].StartingColor = oldColor[SelectedPieceID];
                        animationStates[SelectedPieceID].EndingColor = Players[CurrentPlayerID].Color;
                    }
                }
                animations.StartAnimation(SelectedPieceID, ATTACKANIMATIONSPEED, attackAnimation);
            }
            animDir = !animDir;
            break;
        }

        case GameStages::MovePiece:
        {
            uint16_t oldPos = Players[CurrentPlayerID].Pieces[SelectedPieceID].GetPosition();
            uint16_t newPos = (uint16_t)GetNextPositionByPiece(CurrentPlayerID, SelectedPieceID);
            bool isPieceHit = false;

            AnimUpdateCallback GarageFadesInAnimation = [=](const AnimationParam &param)
            {
                RgbwColor updatedColor = RgbwColor::LinearBlend(animationStates[param.index].StartingColor, animationStates[param.index].EndingColor, param.progress);
                strip.SetPixelColor(garagePos + DIE_LENGTH, updatedColor);
            };

            AnimUpdateCallback PieceFadesInAnimation = [=](const AnimationParam &param)
            {
                RgbwColor updatedColor = RgbwColor::LinearBlend(animationStates[param.index].StartingColor, animationStates[param.index].EndingColor, param.progress);
                strip.SetPixelColor(newPos + DIE_LENGTH, updatedColor);
            };

            AnimUpdateCallback PieceFadesOutAnimation = [=](const AnimationParam &param)
            {
                RgbwColor updatedColor = RgbwColor::LinearBlend(animationStates[param.index].StartingColor, animationStates[param.index].EndingColor, param.progress);
                strip.SetPixelColor(oldPos + DIE_LENGTH, updatedColor);
            };

            if (IsPositionInGarage(CurrentPlayerID, Players[CurrentPlayerID].Pieces[SelectedPieceID].GetPosition()) && DieValue == 6)
                MyCounter += DieValue - 1;

            if (MyCounter < DieValue)
            {
                uint16_t indexAnim;

                //  Fading current tile OUT
                if (animations.NextAvailableAnimation(&indexAnim, 0))
                {
                    animationStates[indexAnim].StartingColor = Players[CurrentPlayerID].Color;
                    if (JustStarting)
                    {
                        if (IsPositionInGarage(CurrentPlayerID, Players[CurrentPlayerID].Pieces[SelectedPieceID].GetPosition()))
                            animationStates[indexAnim].EndingColor = DarkCellBackgroundColor;
                        else
                            animationStates[indexAnim].EndingColor = DefaultCellBackgroundColor;
                        JustStarting = false;
                    }
                    else
                        animationStates[indexAnim].EndingColor = OldColor;
                    animations.StartAnimation(indexAnim, PIECEMOVESANIMATIONSPEED, PieceFadesOutAnimation);
                }

                //  Fading next tile IN
                if (animations.NextAvailableAnimation(&indexAnim, 0))
                {
                    OldColor = strip.GetPixelColor(GetNextPositionByPiece(CurrentPlayerID, SelectedPieceID) + DIE_LENGTH);

                    animationStates[indexAnim].StartingColor = strip.GetPixelColor(GetNextPositionByPiece(CurrentPlayerID, SelectedPieceID) + DIE_LENGTH);
                    animationStates[indexAnim].EndingColor = Players[CurrentPlayerID].Color;
                    animations.StartAnimation(indexAnim, PIECEMOVESANIMATIONSPEED, PieceFadesInAnimation);
                }

                //  If it is the last step and there is another player's piece on it, send it back to his garage.
                int8_t hitPlayerID = GetPlayerByPosition(newPos);
                if (++MyCounter == DieValue && hitPlayerID > -1)
                {
                    int8_t hitPieceID = GetPieceByPosition(newPos);
                    garagePos = GetNextSpaceInGarage(hitPlayerID);

                    //  Fading garage tile IN
                    if (animations.NextAvailableAnimation(&indexAnim, 0))
                    {
                        animationStates[indexAnim].StartingColor = GarageBackgroundColor;
                        animationStates[indexAnim].EndingColor = Players[hitPlayerID].Color;
                        animations.StartAnimation(indexAnim, PIECEMOVESANIMATIONSPEED, GarageFadesInAnimation);
                    }
                    Players[hitPlayerID].Pieces[hitPieceID].SetPosition(garagePos);
                    isPieceHit = true;

                    //  Update statistics
                    Statistics.Player[CurrentPlayerID].Kicked[(uint8_t)hitPlayerID]++;
                }

                //  Update piece's position
                Players[CurrentPlayerID].Pieces[SelectedPieceID].SetPosition(GetNextPositionByPiece(CurrentPlayerID, SelectedPieceID));

                //  Play tune
                if (isPieceHit)
                    PlayMelody(Melodies::PieceHit);
                else
                    PlayMelody(Melodies::PieceMoves);
            }
            else
                Stage = GameStages::EvaluateRound;
            break;
        }

        case GameStages::EvaluateRound:
        {
            Players[CurrentPlayerID].IsFinished = IsPlayerFinished(CurrentPlayerID);

            //  Update statistics
            Statistics.Round++;
            // Statistics.Print();

            SendGameStatus();
            SendGameStatistics();

            if (GetNumberOfPlayersStillPlaying() > 0)
            {
                //  If player rolled a 6, he can have another turn, if he is not finished
                if (DieValue < 6 || Players[CurrentPlayerID].IsFinished)
                    SetNextPlayer(false);

                SerialMon.printf("Next up: #%u\r\n", CurrentPlayerID);
                PlayerFinishedRolling = false;
                DelayBeforeRolling = millis();
                Stage = GameStages::PlayerRolling;
            }
            else
            {
                SerialMon.println("Finito!");

                //  Clear die
                for (size_t i = 0; i < DIE_LENGTH; i++)
                    strip.SetPixelColor(i, DarkCellBackgroundColor);
                strip.Show();

                animDir = true;
                Stage = GameStages::Finished;
            }
            break;
        }

        case GameStages::Finished:
        {
            AnimUpdateCallback FadeAnimation = [=](const AnimationParam &param)
            {
                RgbwColor updatedColor = RgbwColor::LinearBlend(animationStates[param.index].StartingColor, animationStates[param.index].EndingColor, param.progress);

                strip.SetPixelColor(Players[animationStates[param.index].PlayerID].Pieces[animationStates[param.index].PieceID].GetPosition() + DIE_LENGTH, updatedColor);
            };

            for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
            {
                if (Players[i].IsActive)
                {
                    for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
                    {
                        uint16_t indexAnim = NUMBEROFPIECESPERPLAYER * i + j;

                        HslColor myColor = (RgbColor)Players[i].Color;
                        myColor.L *= (float)random(2, 10) / 10;

                        animationStates[indexAnim].StartingColor = strip.GetPixelColor(Players[i].Pieces[j].GetPosition() + DIE_LENGTH);
                        animationStates[indexAnim].EndingColor = myColor;

                        animationStates[indexAnim].PlayerID = i;
                        animationStates[indexAnim].PieceID = j;

                        animations.StartAnimation(indexAnim, random(100, 200), FadeAnimation);
                    }
                }
            }

            animDir = !animDir;
            break;
        }

        default:
            break;
        }
    }
}
