#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "TimeChangeRules.h"
#include "mqtt.h"
#include "common.h"
#include "version.h"
#include "settings.h"
#include "main.h"
#include "game.h"
#include "gamestatistics.h"



WiFiClient client;
PubSubClient PSclient(client);

unsigned long heartbeatMillis = 0;

void ConnectToMQTTBroker()
{
    if (!PSclient.connected())
    {
        SerialMon.printf("Connecting to MQTT broker %s... ", appSettings.mqttServer);

        char msg[100];
        sprintf(msg, "{\"System\":{\"Hostname\":\"%s\",\"Status\":\"offline\"}}", appSettings.hostName);

        char myTopic[MAX_MQTT_TOPIC_LENGTH];
        sprintf(myTopic, "%s/%s", appSettings.mqttTopic, MQTT_STATE_TOPIC);

        if (PSclient.connect(appSettings.hostName, appSettings.mqttUserName, appSettings.mqttPassword, myTopic, 0, true, msg))
        {
            SerialMon.println(" Success.");

            //  Send state
            sprintf(myTopic, "%s/%s", appSettings.mqttTopic, MQTT_STATE_TOPIC);
            sprintf(msg, "{\"System\":{\"Hostname\":\"%s\",\"Status\":\"online\"}}", appSettings.hostName);
            PSclient.publish(myTopic, msg, true);

            //  Send heartbeat
            SendHeartbeat();

            //  Listening to commands
            sprintf(myTopic, "%s/%s/#", appSettings.mqttTopic, MQTT_COMMAND_TOPIC);
            PSclient.subscribe(myTopic, 0);
        }
        else
        {
            SerialMon.println(" failed. Check broker settings, credentials, access.");
        }
    }
}

void SendDataToBroker(const char *topic, const char data[], bool retained)
{
    if (strlen(appSettings.mqttServer) == 0)
        return;

    if (PSclient.connected())
    {
        PSclient.publish(topic, data, retained);
    }
}

void SendGameStatus()
{
    JsonDocument doc;

    //  General
    doc["General"]["DeviceID"] = GetChipID();
    doc["General"]["GameID"] = (String)gameID;
    doc["General"]["numberOfRounds"] = Statistics.Round;
    doc["General"]["currentPlayer"] = CurrentPlayerID;
    doc["General"]["dieValue"] = DieValue;

    //  Players
    JsonArray jaPlayers = doc["Players"].to<JsonArray>();
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
    {
        JsonObject joPlayer = jaPlayers.add<JsonObject>();
        joPlayer["id"] = Players[i].GetId();
        joPlayer["isActive"] = Players[i].IsActive;
        joPlayer["isFinished"] = Players[i].IsFinished;
        char myColor[10];
        sprintf(myColor, "#%02x%02x%02x%02x\0", Players[i].Color.R, Players[i].Color.G, Players[i].Color.B, Players[i].Color.W);
        joPlayer["color"] = myColor;
        JsonArray jaPieces = joPlayer["Pieces"].to<JsonArray>();
        for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
            jaPieces.add(Players[i].Pieces[j].GetPosition());
    }

    //  Rules
    doc["Rules"]["SafeHomeTile"] = appSettings.rules.IsHomeSafe;
    doc["Rules"]["AssistedMode"] = appSettings.rules.AssistedMode;

    doc.shrinkToFit();

    char output[MQTT_BUFFER_SIZE];
    serializeJson(doc, output);

    char myTopic[MAX_MQTT_TOPIC_LENGTH];
    sprintf(myTopic, "%s/%s", appSettings.mqttTopic, MQTT_GAME_TOPIC);
    SendDataToBroker(myTopic, output, false);
}

void SendGameStatistics()
{
    JsonDocument doc;

    //  Game
    doc["Game"]["DeviceID"] = GetChipID();
    doc["Game"]["GameID"] = (String)gameID;
    doc["Game"]["numberOfRounds"] = Statistics.Round;

    //  Players
    JsonArray jaPlayers = doc["Players"].to<JsonArray>();
    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
    {
        JsonObject joPlayer = jaPlayers.add<JsonObject>();
        joPlayer["id"] = Players[i].GetId();
        joPlayer["totalPips"] = Statistics.Player[i].TotalPips;
        joPlayer["emptyPips"] = Statistics.Player[i].EmptyPips;

        char num[2];

        //  Number of rolls by die value
        JsonObject joRolls = joPlayer["rolls"].to<JsonObject>();
        for (size_t j = 0; j < 6; j++)
        {
            sprintf(num, "%u", j + 1); //  +1 needed because the die values are stored as 0..5 instead of 1..6
            joRolls[num] = Statistics.Player[i].Rolls[j];
        }

        //  Number of empty rolls by die value
        JsonObject joEmptyRolls = joPlayer["emptyRolls"].to<JsonObject>();
        for (size_t j = 0; j < 6; j++)
        {
            sprintf(num, "%u", j + 1); //  +1 needed because the die values are stored as 0..5 instead of 1..6
            joEmptyRolls[num] = Statistics.Player[i].EmptyRolls[j];
        }

        //  Players kicked out
        JsonObject joKickedOut = joPlayer["kickedOut"].to<JsonObject>();
        for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
        {
            sprintf(num, "%u", j);
            joKickedOut[num] = Statistics.Player[i].Kicked[j];
        }

    }

    doc.shrinkToFit();

    char output[MQTT_BUFFER_SIZE];
    serializeJson(doc, output);

    char myTopic[MAX_MQTT_TOPIC_LENGTH];
    sprintf(myTopic, "%s/%s", appSettings.mqttTopic, MQTT_STATISTICS_TOPIC);
    SendDataToBroker(myTopic, output, false);
}

void SendHeartbeat()
{
    char buffer[MQTT_BUFFER_SIZE];
    JsonDocument doc;

    JsonObject sysDetails = doc["System"].to<JsonObject>();

    sysDetails["ChipID"] = GetChipID();
    TimeChangeRule *tcr;
    time_t localTime = timechangerules::timezones[appSettings.timeZone]->toLocal(now(), &tcr);
    sysDetails["Time"] = DateTimeToString(localTime);
    sysDetails["FriendlyName"] = appSettings.friendlyName;
    sysDetails["Freeheap"] = ESP.getFreeHeap();

    sysDetails["HardwareID"] = HARDWARE_ID;
    sysDetails["HardwareVersion"] = HARDWARE_VERSION;
    sysDetails["FirmwareID"] = FIRMWARE_ID;
    sysDetails["FirmwareVersion"] = FIRMWARE_VERSION;
    sysDetails["UpTime"] = TimeIntervalToString(millis() / 1000);
    sysDetails["CPU0_ResetReason"] = GetResetReasonString(rtc_get_reset_reason(0));
    sysDetails["CPU1_ResetReason"] = GetResetReasonString(rtc_get_reset_reason(1));
    sysDetails["TIMEZONE"] = appSettings.timeZone;
    sysDetails["NTPServer"] = appSettings.ntpServer;
    sysDetails["UseBeeper"] = appSettings.useBeeper;

    JsonObject mqttDetails = doc["MQTT"].to<JsonObject>();

    mqttDetails["Server"] = appSettings.mqttServer;
    mqttDetails["Port"] = appSettings.mqttPort;
    mqttDetails["Topic"] = appSettings.mqttTopic;
    mqttDetails["User"] = appSettings.mqttUserName;

    JsonObject wifiDetails = doc["WiFi"].to<JsonObject>();
    wifiDetails["SSID"] = appSettings.ssid;
    wifiDetails["Channel"] = WiFi.channel();
    wifiDetails["HostName"] = appSettings.hostName;
    wifiDetails["IP_Address"] = WiFi.localIP().toString();
    wifiDetails["MAC_Address"] = WiFi.macAddress();

    JsonObject gameDetails = doc["Game"].to<JsonObject>();
    gameDetails["SafeHome"] = appSettings.rules.IsHomeSafe;
    gameDetails["AssistedMode"] = appSettings.rules.AssistedMode;

    serializeJson(doc, buffer);

    char myTopic[MAX_MQTT_TOPIC_LENGTH];
    sprintf(myTopic, "%s/%s", appSettings.mqttTopic, MQTT_HEARTBEAT_TOPIC);
    SendDataToBroker(myTopic, buffer, false);
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload, len);

    if (error)
    {
        Serial.print("Error decoding JSON data: ");
        Serial.println(error.c_str());
        return;
    }

#ifdef __debugSettings
    SerialMon.print("Message received in ");
    SerialMon.print(topic);
    SerialMon.println(": ");

    serializeJsonPretty(doc, SerialMon);
    SerialMon.println();
#endif

    if (doc["command"].isNull())
        return;

    const char *command = doc["command"];

    if (strcmp(command, "loadGame") == 0)
    {
        // Sample json:
        // {
        //   "command": "loadGame",
        //   "params": {
        //     "General": {
        //       "currentPlayer": 2,
        //       "dieValue": 6
        //     },
        //     "Players": [
        //       {
        //         "id": 0,
        //         "isActive": false,
        //         "isFinished": false,
        //         "color": "#ff000000",
        //         "Pieces": [
        //           17,
        //           16,
        //           15,
        //           14
        //         ]
        //       },
        //       {
        //         "id": 1,
        //         "isActive": false,
        //         "isFinished": false,
        //         "color": "#00ff0000",
        //         "Pieces": [
        //           35,
        //           34,
        //           33,
        //           32
        //         ]
        //       },
        //       {
        //         "id": 2,
        //         "isActive": true,
        //         "isFinished": false,
        //         "color": "#0000ff00",
        //         "Pieces": [
        //           91,
        //           76,
        //           55,
        //           50
        //         ]
        //       },
        //       {
        //         "id": 3,
        //         "isActive": true,
        //         "isFinished": false,
        //         "color": "#b2160a00",
        //         "Pieces": [
        //           75,
        //           74,
        //           59,
        //           68
        //         ]
        //       },
        //       {
        //         "id": 4,
        //         "isActive": true,
        //         "isFinished": false,
        //         "color": "#ff00ff00",
        //         "Pieces": [
        //           95,
        //           94,
        //           77,
        //           86
        //         ]
        //       },
        //       {
        //         "id": 5,
        //         "isActive": false,
        //         "isFinished": false,
        //         "color": "#ffff0000",
        //         "Pieces": [
        //           107,
        //           106,
        //           105,
        //           104
        //         ]
        //       }
        //     ],
        //     "Rules": {
        //       "SafeHomeTile": true,
        //       "AssistedMode": true
        //     }
        //   }
        // }

        StartNewGame();

        JsonObject joParams = doc["params"];

        //  General
        CurrentPlayerID = (uint8_t)joParams["General"]["currentPlayer"];
        DieValue = (uint8_t)joParams["General"]["dieValue"];

        for (JsonObject joPlayer : joParams["Players"].as<JsonArray>())
        {
            uint8_t playerID = (uint8_t)joPlayer["id"];
            Players[playerID].IsActive = (bool)joPlayer["isActive"];
            Players[playerID].IsFinished = (bool)joPlayer["isFinished"];

            uint8_t r, g, b, w;
            parseRGBW(joPlayer["color"], r, g, b, w);
            Players[playerID].Color = RgbwColor(r, g, b, w);

            for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
                Players[playerID].Pieces[j].SetPosition((uint16_t)joPlayer["Pieces"][j]);

            if (Players[playerID].IsActive)
                TogglePlayerVisibility(playerID);
        }

        //  Rules
        appSettings.rules.IsHomeSafe = (bool)joParams["Rules"]["SafeHomeTile"];
        appSettings.rules.AssistedMode = (bool)joParams["Rules"]["AssistedMode"];

        Stage = GameStages::EvaluateRound;

        return;
    }

    if (strcmp(command, "getNextPosition") == 0)
    {
        //  Sample json:
        //  {
        //   "command": "getNextPosition",
        //   "params": {
        //     "player": 1,
        //     "pos": 9
        //   }
        // }

        uint8_t playerID = doc["params"]["player"];
        uint16_t pos = doc["params"]["pos"];

        SerialMon.println(GetNextPosition(playerID, pos));
        return;
    }

    if (strcmp(command, "setNextPlayer") == 0)
    {
        //  Sample json:
        //  {
        //   "command": "setNextPlayer",
        //   "params": {
        //     "isRandom": false
        //   }
        // }

        for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
            SerialMon.printf("Player: %u\tActive: %s\tFinished: %s\r\n", i, Players[i].IsActive ? "yes" : "no", Players[i].IsFinished ? "yes" : "no");

        bool isRandom = doc["params"]["isRandom"];

        SerialMon.printf("Before:\t%u\r\n", CurrentPlayerID);
        SetNextPlayer(isRandom);
        SerialMon.printf("After:\t%u\r\n", CurrentPlayerID);
    }

    if (strcmp(command, "getNextPositionByPiece") == 0)
    {
        //  Sample json:
        // {
        //   "command": "getNextPositionByPiece",
        //   "params": {
        //     "player": 0,
        //     "piece": 0
        //   }
        // }

        uint8_t playerID = doc["params"]["player"];
        uint8_t pieceID = doc["params"]["piece"];
        SerialMon.printf("Next position is: %i\r\n", GetNextPositionByPiece(playerID, pieceID));
        return;
    }

    if (strcmp(command, "getPlayerByPosition") == 0)
    {
        //  Sample json:
        // {
        //   "command": "getPlayerByPosition",
        //   "params": {
        //     "pos": 18
        //   }
        // }

        uint8_t pos = doc["params"]["pos"];
        SerialMon.printf("Tile #%u is occupied by player %i.\r\n", pos, GetPlayerByPosition(pos));
        return;
    }

    if (strcmp(command, "getPieceByPosition") == 0)
    {
        //  Sample json:
        // {
        //   "command": "getPieceByPosition",
        //   "params": {
        //     "pos": 0
        //   }
        // }

        int16_t pos = doc["params"]["pos"];
        SerialMon.printf("Position #%u\tPlayer: %i\tPiece: %i\r\n", pos, GetPlayerByPosition(pos), GetPieceByPosition(pos));
        return;
    }

    if (strcmp(command, "getLastPieceInGarage") == 0)
    {
        //  Sample json:
        //  {
        //   "command": "getLastPieceInGarage",
        //   "params": {
        //     "player": 3
        //   }
        // }

        uint8_t playerID = doc["params"]["player"];

        SerialMon.println(GetLastPieceInGarage(playerID));
        return;
    }

    if (strcmp(command, "isPlayerFinished") == 0)
    {
        //  Sample json:
        //  {
        //   "command": "isPlayerFinished",
        //   "params": {
        //     "player": 3
        //   }
        // }

        uint8_t playerID = doc["params"]["player"];

        SerialMon.printf("Player %u is %s.\r\n", playerID, IsPlayerFinished(playerID) ? "finished" : "not finished");
        return;
    }

    if (strcmp(command, "togglePlayerVisibility") == 0)
    {
        //  Sample json:
        //  {
        //   "command": "togglePlayerVisibility",
        //   "params": {
        //     "player": 1
        //   }
        // }

        uint8_t playerID = doc["params"]["player"];

        TogglePlayerVisibility(playerID);
        return;
    }

    if (strcmp(command, "movePieceByNumber") == 0)
    {
        //  Sample json:
        // {
        //   "command": "movePieceByNumber",
        //   "params": {
        //     "player": 0,
        //     "piece": 0,
        //     "die": 3
        //   }
        // }

        uint8_t playerID = doc["params"]["player"];
        uint8_t pieceID = doc["params"]["piece"];
        uint16_t die = doc["params"]["die"];
        MovePieceByNumber(playerID, pieceID, die);
        return;
    }

    if (strcmp(command, "getNextPossibleMove") == 0)
    {
        //  Only makes sense in GameStages::SelectMove
        //  Sample json:
        // {
        //   "command": "getNextPossibleMove",
        //   "params": {
        //   }
        // }

        SerialMon.printf("Next possible move: %i\r\n", GetNextPossibleMove());
        return;
    }
}

void setupMQTT()
{
    if (strlen(appSettings.mqttServer) == 0)
        return;
    PSclient.setServer(appSettings.mqttServer, appSettings.mqttPort);
    PSclient.setKeepAlive(MQTT_KEEP_ALIVE_PERIOD);
    PSclient.setCallback(mqttCallback);
    if (!PSclient.setBufferSize(MQTT_BUFFER_SIZE))
    {
        SerialMon.printf("Could not resize MQTT buffer to %u.\r\n", MQTT_BUFFER_SIZE);
    }
    heartbeatMillis = millis();
}

void loopMQTT()
{
    if (strlen(appSettings.mqttServer) == 0)
        return;

    if (PSclient.connected())
    {
        PSclient.loop();
        if (millis() - heartbeatMillis > appSettings.heartbeatInterval * 1000)
        {
            SendHeartbeat();
            heartbeatMillis = millis();
        }
    }
    else
    {
        ConnectToMQTTBroker();
    }
}