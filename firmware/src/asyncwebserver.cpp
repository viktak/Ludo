#include <Arduino.h>

#include <DNSServer.h>

#include <ArduinoJson.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

#include <AceSorting.h>

#include "asyncwebserver.h"
#include "common.h"
#include "version.h"
#include "settings.h"
#include "main.h"
#include "leds.h"
#include "game.h"
#include "mqtt.h"

#define OTA_PROGRESS_UPDATE_INTERVAL 30

const char *ADMIN_USERNAME = "admin";

AsyncWebServer server(80);

size_t updateSize = 0;
size_t lastCurrent = 0;
bool shouldReboot = false;
unsigned long ota_progress_millis = 0;

//  Helper functions - not used in production
void PrintParameters(AsyncWebServerRequest *request)
{
    int params = request->params();
    for (int i = 0; i < params; i++)
    {
        const AsyncWebParameter *p = request->getParam(i);
        if (p->isFile())
        { // p->isPost() is also true
            SerialMon.printf("FILE[%s]: %s, size: %u\r\n", p->name().c_str(), p->value().c_str(), p->size());
        }
        else if (p->isPost())
        {
            SerialMon.printf("POST[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
        }
        else
        {
            SerialMon.printf("GET[%s]: %s\r\n", p->name().c_str(), p->value().c_str());
        }
    }
}

void PrintHeaders(AsyncWebServerRequest *request)
{
    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++)
    {
        const AsyncWebHeader *h = request->getHeader(i);
        SerialMon.printf("HEADER[%s]: %s\r\n", h->name().c_str(), h->value().c_str());
    }
}

// Root/Status
String StatusTemplateProcessor(const String &var)
{
    //  System information
    if (var == "sitename")
        return (String)appSettings.friendlyName;
    if (var == "chipid")
        return (String)GetChipID();
    if (var == "hardwareid")
        return HARDWARE_ID;
    if (var == "hardwareversion")
        return HARDWARE_VERSION;
    if (var == "firmwareid")
        return FIRMWARE_ID;
    if (var == "firmwareversion")
        return FIRMWARE_VERSION;
    if (var == "timezone")
        return timechangerules::tzDescriptions[appSettings.timeZone];
    if (var == "currenttime")
    {
        time_t localTime = timechangerules::timezones[appSettings.timeZone]->toLocal(now(), &tcr);
        return DateTimeToString(localTime);
    }
    if (var == "uptime")
        return TimeIntervalToString(millis() / 1000);
    if (var == "lastresetreason0")
        return GetResetReasonString(rtc_get_reset_reason(0));
    if (var == "lastresetreason1")
        return GetResetReasonString(rtc_get_reset_reason(1));
    if (var == "flashchipsize")
        return String(ESP.getFlashChipSize());
    if (var == "flashchipspeed")
        return String(ESP.getFlashChipSpeed());
    if (var == "freeheapsize")
        return String(ESP.getFreeHeap());
    if (var == "freesketchspace")
        return String(ESP.getFreeSketchSpace());

    //  Network

    switch (WiFi.getMode())
    {
    case WIFI_AP:
        if (var == "wifimode")
            return "Access Point";
        if (var == "ssid")
            return "-";
        if (var == "channel")
            return String(WiFi.channel());
        if (var == "hostname")
            return appSettings.hostName;
        if (var == "macaddress")
            return "-";
        if (var == "ipaddress")
            return WiFi.softAPIP().toString();
        if (var == "subnetmask")
            return WiFi.softAPSubnetMask().toString();
        if (var == "apmacaddress")
            return String(WiFi.softAPmacAddress());
        if (var == "gateway")
            return "-";
        break;

    case WIFI_STA:
        if (var == "wifimode")
            return "Station";
        if (var == "ssid")
            return String(WiFi.SSID());
        if (var == "channel")
            return String(WiFi.channel());
        if (var == "hostname")
            return appSettings.hostName;
        if (var == "macaddress")
            return String(WiFi.macAddress());
        if (var == "ipaddress")
            return WiFi.localIP().toString();
        if (var == "subnetmask")
            return WiFi.subnetMask().toString();
        if (var == "apssid")
            return "Not active!";
        if (var == "apmacaddress")
            return "Not active!";
        if (var == "gateway")
            return WiFi.gatewayIP().toString();

        break;
    default:
        //  This should not happen...
        break;
    }

    return String();
}

//  General
String GeneralSettingsTemplateProcessor(const String &var)
{

    if (var == "sitename")
        return (String)appSettings.friendlyName;
    if (var == "hostname")
        return appSettings.hostName;
    if (var == "friendlyname")
        return appSettings.friendlyName;
    if (var == "mqtt-servername")
        return appSettings.mqttServer;
    if (var == "mqtt-port")
        return ((String)appSettings.mqttPort).c_str();
    if (var == "mqtt-username")
        return ((String)appSettings.mqttUserName).c_str();
    if (var == "mqtt-password")
        return ((String)appSettings.mqttPassword).c_str();
    if (var == "mqtt-topic")
        return appSettings.mqttTopic;
    if (var == "ntpserver")
        return ((String)appSettings.ntpServer).c_str();
    if (var == "mqtt-topic")
        return appSettings.mqttTopic;
    if (var == "HeartbeatInterval")
        return ((String)appSettings.heartbeatInterval).c_str();

    if (var == "timezoneslist")
    {
        char ss[4];
        String timezoneslist = "";

        for (unsigned int i = 0; i < sizeof(timechangerules::tzDescriptions) / sizeof(timechangerules::tzDescriptions[0]); i++)
        {
            itoa(i, ss, DEC);
            timezoneslist += "<option ";
            if ((unsigned int)appSettings.timeZone == i)
                timezoneslist += "selected ";

            timezoneslist += "value=\"";
            timezoneslist += ss;
            timezoneslist += "\">";

            timezoneslist += timechangerules::tzDescriptions[i];

            timezoneslist += "</option>";
        }

        return timezoneslist;
    }

    return String();
}

void POSTGeneralSettings(AsyncWebServerRequest *request)
{
    const AsyncWebParameter *p;

    PrintParameters(request);

    if (request->hasParam("txtFriendlyName", true))
    {
        p = request->getParam("txtFriendlyName", true);
        sprintf(appSettings.friendlyName, "%s", p->value().c_str());
    }

    if (request->hasParam("txtHostname", true))
    {
        p = request->getParam("txtHostname", true);
        sprintf(appSettings.hostName, "%s", p->value().c_str());
    }

    if (request->hasParam("txtServerName", true))
    {
        p = request->getParam("txtServerName", true);
        sprintf(appSettings.mqttServer, "%s", p->value().c_str());
    }

    if (request->hasParam("numPort", true))
    {
        p = request->getParam("numPort", true);
        appSettings.mqttPort = atoi(p->value().c_str());
    }

    if (request->hasParam("txtUsername", true))
    {
        p = request->getParam("txtUsername", true);
        sprintf(appSettings.mqttUserName, "%s", p->value().c_str());
    }

    if (request->hasParam("txtPassword", true))
    {
        p = request->getParam("txtPassword", true);
        sprintf(appSettings.mqttPassword, "%s", p->value().c_str());
    }

    if (request->hasParam("txtTopic", true))
    {
        p = request->getParam("txtTopic", true);
        sprintf(appSettings.mqttTopic, "%s", p->value().c_str());
    }

    if (request->hasParam("txtNTPServer", true))
    {
        p = request->getParam("txtNTPServer", true);
        sprintf(appSettings.ntpServer, "%s", p->value().c_str());
    }

    if (request->hasParam("lstTimeZones", true))
    {
        p = request->getParam("lstTimeZones", true);
        appSettings.timeZone = atoi(p->value().c_str());
    }

    if (request->hasParam("numHeartbeatInterval", true))
    {
        p = request->getParam("numHeartbeatInterval", true);
        appSettings.heartbeatInterval = atoi(p->value().c_str());
    }
}

//  Players settings
String PlayersSettingsTemplateProcessor(const String &var)
{
    if (var == "sitename")
        return (String)appSettings.friendlyName;

    if (var == "txtPlayer0Color")
    {
        char myColor[8];
        sprintf(myColor, "#%02x%02x%02x", Players[0].Color.R, Players[0].Color.G, Players[0].Color.B);
        return (String)myColor;
    }

    if (var == "txtPlayer1Color")
    {
        char myColor[8];
        sprintf(myColor, "#%02x%02x%02x", Players[1].Color.R, Players[1].Color.G, Players[1].Color.B);
        return (String)myColor;
    }

    if (var == "txtPlayer2Color")
    {
        char myColor[8];
        sprintf(myColor, "#%02x%02x%02x", Players[2].Color.R, Players[2].Color.G, Players[2].Color.B);
        return (String)myColor;
    }

    if (var == "txtPlayer3Color")
    {
        char myColor[8];
        sprintf(myColor, "#%02x%02x%02x", Players[3].Color.R, Players[3].Color.G, Players[3].Color.B);
        return (String)myColor;
    }

    if (var == "txtPlayer4Color")
    {
        char myColor[8];
        sprintf(myColor, "#%02x%02x%02x", Players[4].Color.R, Players[4].Color.G, Players[4].Color.B);
        return (String)myColor;
    }

    if (var == "txtPlayer5Color")
    {
        char myColor[8];
        sprintf(myColor, "#%02x%02x%02x", Players[5].Color.R, Players[5].Color.G, Players[5].Color.B);
        return (String)myColor;
    }

    return String();
}

void POSTPlayersSettings(AsyncWebServerRequest *request)
{
    const AsyncWebParameter *p;

    // PrintParameters(request);

    if (request->hasParam("txtPlayer0Color", true))
    {
        //  Get parameter
        p = request->getParam("txtPlayer0Color", true);
        //  Put parameter in a buffer
        char buf[7];
        sprintf(buf, "%s", p->value().c_str());
        //  Convert it to decimal
        uint32_t decimalColor = hexColorToDecimal(buf);
        //  Set new colors
        Players[0].Color = RgbwColor((decimalColor >> 16) & 0xFF, (decimalColor >> 8) & 0xFF, decimalColor & 0xFF, 0);
        RefreshPieces(0);
    }
    if (request->hasParam("txtPlayer1Color", true))
    {
        p = request->getParam("txtPlayer1Color", true);
        char buf[7];
        sprintf(buf, "%s", p->value().c_str());
        uint32_t decimalColor = hexColorToDecimal(buf);
        Players[1].Color = RgbwColor((decimalColor >> 16) & 0xFF, (decimalColor >> 8) & 0xFF, decimalColor & 0xFF, 0);
        RefreshPieces(1);
    }
    if (request->hasParam("txtPlayer2Color", true))
    {
        p = request->getParam("txtPlayer2Color", true);
        char buf[7];
        sprintf(buf, "%s", p->value().c_str());
        uint32_t decimalColor = hexColorToDecimal(buf);
        Players[2].Color = RgbwColor((decimalColor >> 16) & 0xFF, (decimalColor >> 8) & 0xFF, decimalColor & 0xFF, 0);
        RefreshPieces(2);
    }
    if (request->hasParam("txtPlayer3Color", true))
    {
        p = request->getParam("txtPlayer3Color", true);
        char buf[7];
        sprintf(buf, "%s", p->value().c_str());
        uint32_t decimalColor = hexColorToDecimal(buf);
        Players[3].Color = RgbwColor((decimalColor >> 16) & 0xFF, (decimalColor >> 8) & 0xFF, decimalColor & 0xFF, 0);
        RefreshPieces(3);
    }
    if (request->hasParam("txtPlayer4Color", true))
    {
        p = request->getParam("txtPlayer4Color", true);
        char buf[7];
        sprintf(buf, "%s", p->value().c_str());
        uint32_t decimalColor = hexColorToDecimal(buf);
        Players[4].Color = RgbwColor((decimalColor >> 16) & 0xFF, (decimalColor >> 8) & 0xFF, decimalColor & 0xFF, 0);
        RefreshPieces(4);
    }
    if (request->hasParam("txtPlayer5Color", true))
    {
        p = request->getParam("txtPlayer5Color", true);
        char buf[7];
        sprintf(buf, "%s", p->value().c_str());
        uint32_t decimalColor = hexColorToDecimal(buf);
        Players[5].Color = RgbwColor((decimalColor >> 16) & 0xFF, (decimalColor >> 8) & 0xFF, decimalColor & 0xFF, 0);
        RefreshPieces(5);
    }
}

//  Rules settings
String RulesSettingsTemplateProcessor(const String &var)
{
    if (var == "sitename")
        return (String)appSettings.friendlyName;
    if (var == "chkusebeeper")
        return (appSettings.useBeeper ? "checked" : "");
    if (var == "chkHomeSafe")
        return appSettings.rules.IsHomeSafe == true ? "checked" : "";
    if (var == "chkAssistedMode")
        return appSettings.rules.AssistedMode == true ? "checked" : "";

    return String();
}

void POSTRulesSettings(AsyncWebServerRequest *request)
{
    AsyncWebParameter *p;

    PrintParameters(request);

    if (request->hasParam("chkHomeSafe", true))
        appSettings.rules.IsHomeSafe = true;
    else
        appSettings.rules.IsHomeSafe = false;

    if (request->hasParam("chkAssistedMode", true))
        appSettings.rules.AssistedMode = true;
    else
        appSettings.rules.AssistedMode = false;

    if (request->hasParam("chkUseBeeper", true))
        appSettings.useBeeper = true;
    else
        appSettings.useBeeper = false;
}

//  Save/Load game
String SaveLoadGameTemplateProcessor(const String &var)
{
    if (var == "sitename")
        return (String)appSettings.friendlyName;

    if (var.startsWith("loaddisabled"))
    {
        char slotName[13];
        sprintf(slotName, "ludo-save-%u", atoi((var.substring(var.length() - 1)).c_str()));

        Preferences saveGames;
        saveGames.begin(slotName, RO_MODE);

        uint8_t numberOfPlayers = 0;
        for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
        {
            char myKey[15];
            sprintf(myKey, "PL%uIsActive", j);
            if (saveGames.getBool(myKey, false))
                numberOfPlayers++;
        }

        saveGames.end();
        return numberOfPlayers > 0 ? "" : "disabled";
    }

    if (var.startsWith("savedisabled"))
        return Stage == GameStages::PlayerRolling ? "" : "disabled";

    if (var.startsWith("game"))
    {
        char slotName[13];
        sprintf(slotName, "ludo-save-%s", (String)var[4]);

        Preferences saveGames;
        saveGames.begin(slotName, RO_MODE);

        char myLine[24];
        if (var.endsWith("line0"))
        {
            uint8_t numberOfPlayers = 0;
            for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
            {
                char myKey[15];
                sprintf(myKey, "PL%uIsActive", j);
                if (saveGames.getBool(myKey, false))
                    numberOfPlayers++;
            }

            if (numberOfPlayers > 0)
                sprintf(myLine, "%u player game", numberOfPlayers);
            else
                sprintf(myLine, "[Empty]");
        }
        if (var.endsWith("line1"))
            if (saveGames.isKey("CurrentRound"))
                sprintf(myLine, "Round: %u", saveGames.getUInt("CurrentRound", 0));
            else
                sprintf(myLine, "");

        if (var.endsWith("line2"))
            if (saveGames.isKey("IsHomeSafe"))
                sprintf(myLine, "Home tile %s safe.", saveGames.getBool("IsHomeSafe", false) == true ? "IS" : "is NOT");
            else
                sprintf(myLine, "");
        if (var.endsWith("line3"))
            if (saveGames.isKey("AssistedMode"))
                sprintf(myLine, "Assisted mode is %s.", saveGames.getBool("AssistedMode", false) == true ? "ON" : "OFF");
            else
                sprintf(myLine, "");

        saveGames.end();
        return myLine;
    }

    return String();
}

void POSTSaveLoadGame(AsyncWebServerRequest *request)
{
    PrintParameters(request);

    int8_t slot = -1;
    enum POST_MODES
    {
        Undefined,
        Save,
        Load,
        Download
    } mode = POST_MODES::Undefined;

    if (request->hasParam("deleteAll", true))
    {
        char slotName[15];
        for (size_t i = 0; i < 4; i++)
        {
            Preferences saveGames;
            char slotName[15];
            sprintf(slotName, "ludo-save-%i", i);
            saveGames.begin(slotName, RW_MODE);
            saveGames.clear();
            saveGames.end();
        }
        request->redirect("/saveloadgame.html");
    }

    if (request->hasParam("save0", true))
    {
        slot = 0;
        mode = POST_MODES::Save;
    }
    if (request->hasParam("save1", true))
    {
        slot = 1;
        mode = POST_MODES::Save;
    }
    if (request->hasParam("save2", true))
    {
        slot = 2;
        mode = POST_MODES::Save;
    }
    if (request->hasParam("save3", true))
    {
        slot = 3;
        mode = POST_MODES::Save;
    }

    if (request->hasParam("load0", true))
    {
        slot = 0;
        mode = POST_MODES::Load;
    }
    if (request->hasParam("load1", true))
    {
        slot = 1;
        mode = POST_MODES::Load;
    }
    if (request->hasParam("load2", true))
    {
        slot = 2;
        mode = POST_MODES::Load;
    }
    if (request->hasParam("load3", true))
    {
        slot = 3;
        mode = POST_MODES::Load;
    }

    if (request->hasParam("download0", true))
    {
        slot = 0;
        mode = POST_MODES::Download;
    }

    if (request->hasParam("download1", true))
    {
        slot = 1;
        mode = POST_MODES::Download;
    }

    if (request->hasParam("download2", true))
    {
        slot = 2;
        mode = POST_MODES::Download;
    }

    if (request->hasParam("download3", true))
    {
        slot = 3;
        mode = POST_MODES::Download;
    }

    if (request->hasParam("uploadCurrentButton", true))
    {
        if (!request->_tempObject)
        {
            return request->redirect("/saveloadgame.html");
        }
        SerialMon.printf("Uploading game...\r\n");

        StreamString *buffer = reinterpret_cast<StreamString *>(request->_tempObject);

        StartNewGame();

        JsonDocument doc;

        DeserializationError error = deserializeJson(doc, buffer->c_str());

        if (error)
        {
            SerialMon.print("deserializeJson() failed: ");
            SerialMon.println(error.c_str());
        }
        else
        {
            JsonObject General = doc["General"];
            const char *myGameID = General["GameID"];
            sprintf(gameID, "%s", myGameID);

            Statistics.Round = (uint16_t)General["numberOfRounds"];
            CurrentPlayerID = (uint8_t)General["currentPlayer"];
            appSettings.rules.IsHomeSafe = doc["Rules"]["SafeHomeTile"];
            appSettings.rules.AssistedMode = doc["Rules"]["AssistedMode"];

            for (JsonObject Player : doc["Players"].as<JsonArray>())
            {
                int Player_id = Player["id"];
                Players[Player_id].IsActive = Player["isActive"];
                Players[Player_id].IsFinished = Player["isFinished"];

                String myColor = Player["color"];
                uint8_t r, g, b, w;
                parseRGBW(myColor.c_str(), r, g, b, w);
                Players[Player_id].Color = RgbwColor(r, g, b, w);

                JsonArray Player_Pieces = Player["Pieces"];
                for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
                    Players[Player_id].Pieces[j].SetPosition(Player_Pieces[j]);

                RefreshPieces(Player_id);

                Statistics.Player[Player_id].TotalPips = Player["totalPips"];
                Statistics.Player[Player_id].EmptyPips = Player["emptyPips"];

                JsonObject Player_rolls = Player["rolls"];
                for (size_t j = 0; j < 6; j++)
                    Statistics.Player[Player_id].Rolls[j] = Player_rolls[(String)(j + 1)];

                JsonObject Player_emptyRolls = Player["emptyRolls"];
                for (size_t j = 0; j < 6; j++)
                    Statistics.Player[Player_id].EmptyRolls[j] = Player_emptyRolls[(String)(j + 1)];

                JsonObject Player_kickedOut = Player["kickedOut"];
                for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
                    Statistics.Player[Player_id].Kicked[j] = Player_kickedOut[(String)j];
            }
            SerialMon.printf("Game (%s) uploaded successfully.\r\n", gameID);
        }

        delete buffer;
        request->_tempObject = nullptr;
        return request->redirect("/saveloadgame.html");
    }

    //  Download current game
    if (request->hasParam("downloadCurrent", true))
    {
        JsonDocument doc;

        //  General
        doc["General"]["DeviceID"] = GetChipID();
        doc["General"]["GameID"] = gameID;
        doc["General"]["numberOfRounds"] = Statistics.Round;
        doc["General"]["currentPlayer"] = CurrentPlayerID;

        //  Players
        JsonArray jaPlayers = doc["Players"].to<JsonArray>();
        for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
        {
            JsonObject joPlayer = jaPlayers.add<JsonObject>();
            joPlayer["id"] = Players[i].GetId();
            joPlayer["isActive"] = Players[i].IsActive;
            joPlayer["isFinished"] = Players[i].IsFinished;
            joPlayer["totalPips"] = Statistics.Player[i].TotalPips;
            joPlayer["emptyPips"] = Statistics.Player[i].EmptyPips;

            char myColor[10];
            sprintf(myColor, "#%02x%02x%02x%02x\0", Players[i].Color.R, Players[i].Color.G, Players[i].Color.B, Players[i].Color.W);
            joPlayer["color"] = myColor;
            JsonArray jaPieces = joPlayer["Pieces"].to<JsonArray>();
            for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
                jaPieces.add(Players[i].Pieces[j].GetPosition());

            //  Statistics
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

        //  Rules
        doc["Rules"]["SafeHomeTile"] = appSettings.rules.IsHomeSafe;
        doc["Rules"]["AssistedMode"] = appSettings.rules.AssistedMode;

        doc.shrinkToFit();

        char output[MQTT_BUFFER_SIZE];
        serializeJson(doc, output);

        char fileName[48];
        sprintf(fileName, "ludo-%s-%u.json", gameID, Statistics.Round);

        // Content-Disposition header
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);

        // "Save As" dialog
        response->addHeader("Content-Disposition", "attachment; filename=\"" + (String)fileName + "\"");

        // Optional: Cache control headers
        response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        response->addHeader("Pragma", "no-cache");
        response->addHeader("Expires", "0");

        request->send(response);

        return;
    }

    //  Save game to selected slot
    if (mode == POST_MODES::Save)
        if (slot > -1)
        {
            SerialMon.println("Saving game...");

            char slotName[15];
            sprintf(slotName, "ludo-save-%i", slot);

            Preferences saveGames;
            saveGames.begin(slotName, RW_MODE);

            saveGames.putUInt("DeviceID", GetChipID());
            saveGames.putString("GameID", (String)gameID);
            saveGames.putUInt("CurrentRound", Statistics.Round);
            saveGames.putUInt("CurrentPlayerID", CurrentPlayerID);
            saveGames.putBool("IsHomeSafe", appSettings.rules.IsHomeSafe);
            saveGames.putBool("AssistedMode", appSettings.rules.AssistedMode);

            for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
            {
                char myKey[15];
                sprintf(myKey, "PL%uIsActive", i);
                saveGames.putBool(myKey, Players[i].IsActive);

                sprintf(myKey, "PL%uIsFinished", i);
                saveGames.putBool(myKey, Players[i].IsFinished);

                char myColor[10];
                sprintf(myColor, "#%02x%02x%02x%02x\0", Players[i].Color.R, Players[i].Color.G, Players[i].Color.B, Players[i].Color.W);
                sprintf(myKey, "PL%uColor", Players[i].GetId());
                saveGames.putString(myKey, myColor);

                //  Pieces
                for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
                {
                    sprintf(myKey, "PL%uPIECE%u", i, j);
                    saveGames.putUInt(myKey, Players[i].Pieces[j].GetPosition());
                }

                //  Rolls/Pips
                for (size_t j = 0; j < 6; j++)
                {
                    sprintf(myKey, "PL%uRoll%u", i, j);
                    saveGames.putUInt(myKey, Statistics.Player[i].Rolls[j]);
                    sprintf(myKey, "PL%uEmptyRoll%u", i, j);
                    saveGames.putUInt(myKey, Statistics.Player[i].EmptyRolls[j]);
                }
                sprintf(myKey, "PL%uPips", i);
                saveGames.putUInt(myKey, Statistics.Player[i].TotalPips);
                sprintf(myKey, "PL%uEmptyPips", i);
                saveGames.putUInt(myKey, Statistics.Player[i].EmptyPips);

                //  Hits
                for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
                {
                    sprintf(myKey, "PL%uKicked%u", i, j);
                    saveGames.putUInt(myKey, Statistics.Player[i].Kicked[j]);
                }
            }

            saveGames.end();

            SerialMon.println("Game saved.");
            request->redirect("/saveloadgame.html");
        }

    //  Load game from selected slot
    if (mode == POST_MODES::Load)
        if (slot > -1)
        {
            StartNewGame();

            char slotName[15];
            sprintf(slotName, "ludo-save-%i", slot);

            SerialMon.printf("Loading game from slot #%i...\r\n", slot);

            Preferences saveGames;
            saveGames.begin(slotName, RO_MODE);

            sprintf(gameID, "%s", saveGames.getString("GameID", "00000000").c_str());

            CurrentPlayerID = saveGames.getUInt("CurrentPlayerID");
            Statistics.Round = saveGames.getUInt("CurrentRound");
            appSettings.rules.AssistedMode = saveGames.getBool("AssistedMode");
            appSettings.rules.IsHomeSafe = saveGames.getBool("IsHomeSafe");

            // SerialMon.printf("Round:\t\t%u\r\n", Statistics.Round);
            // SerialMon.printf("Player:\t\t%u\r\n", CurrentPlayerID);
            // SerialMon.printf("Safe home:\t%s\r\n", appSettings.rules.IsHomeSafe ? "yes" : "no");
            // SerialMon.printf("Assisted mode:\t%s\r\n", appSettings.rules.AssistedMode ? "yes" : "no");

            for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
            {
                char myKey[15];

                sprintf(myKey, "PL%uIsActive", i);
                Players[i].IsActive = saveGames.getBool(myKey);

                sprintf(myKey, "PL%uIsFinished", i);
                Players[i].IsFinished = saveGames.getBool(myKey);

                sprintf(myKey, "PL%uColor", i);
                String myColor = saveGames.getString(myKey);

                uint8_t r, g, b, w;
                parseRGBW(myColor.c_str(), r, g, b, w);
                Players[i].Color = RgbwColor(r, g, b, w);

                // SerialMon.printf("PlayerID:\t%u - %u\r\n", i, Players[i].GetId());
                // SerialMon.printf("Active:\t\t%s\r\n", Players[i].IsActive ? "yes" : "no");
                // SerialMon.printf("Finished:\t%s\r\n", Players[i].IsFinished ? "yes" : "no");
                // SerialMon.printf("ColorString:\t%\r\n", myColor);
                // SerialMon.printf("Color:\t\tR: %u\tG: %u\tB: %u\tW: %u\r\n", Players[i].Color.R, Players[i].Color.G, Players[i].Color.B, Players[i].Color.W);

                //  Pieces
                for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
                {
                    sprintf(myKey, "PL%uPIECE%u", i, j);
                    Players[i].Pieces[j].SetPosition(saveGames.getUInt(myKey));
                }

                //  Rolls/Pips
                for (size_t j = 0; j < 6; j++)
                {
                    sprintf(myKey, "PL%uRoll%u", i, j);
                    Statistics.Player[i].Rolls[j] = saveGames.getUInt(myKey, 0);
                    sprintf(myKey, "PL%uEmptyRoll%u", i, j);
                    Statistics.Player[i].EmptyRolls[j] = saveGames.getUInt(myKey, 0);
                }
                sprintf(myKey, "PL%uPips", i);
                Statistics.Player[i].TotalPips = saveGames.getUInt(myKey, 0);
                sprintf(myKey, "PL%uEmptyPips", i);
                Statistics.Player[i].EmptyPips = saveGames.getUInt(myKey, 0);

                //  Hits
                for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
                {
                    sprintf(myKey, "PL%uKicked%u", i, j);
                    Statistics.Player[i].Kicked[j] = saveGames.getUInt(myKey, 0);
                }

                RefreshPieces(i);
            }

            saveGames.end();
            SerialMon.printf("Game (ID: %s) loaded.\r\n", gameID);
            Stage = GameStages::PlayerRolling;
            request->redirect("/saveloadgame.html");
        }

    //  Download game
    if (mode == POST_MODES::Download)
        if (slot > -1)
        {

            char slotName[15];
            sprintf(slotName, "ludo-save-%i", slot);

            SerialMon.printf("Downloading game from slot #%i...\r\n", slot);

            Preferences saveGames;
            saveGames.begin(slotName, RO_MODE);

            char savedGameID[25];
            sprintf(savedGameID, "%s", saveGames.getString("GameID", "00000000").c_str());
            SerialMon.printf("GameID: %s\r\n", savedGameID);

            uint32_t myRound = saveGames.getUInt("CurrentRound");

            JsonDocument doc;

            //  General
            doc["General"]["DeviceID"] = GetChipID();
            doc["General"]["GameID"] = (String)savedGameID;
            doc["General"]["numberOfRounds"] = myRound;
            doc["General"]["currentPlayer"] = saveGames.getUInt("CurrentPlayerID");

            //  Players
            JsonArray jaPlayers = doc["Players"].to<JsonArray>();
            for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
            {
                JsonObject joPlayer = jaPlayers.add<JsonObject>();
                char myKey[15];

                joPlayer["id"] = i;

                sprintf(myKey, "PL%uIsActive", i);
                joPlayer["isActive"] = saveGames.getBool(myKey);

                sprintf(myKey, "PL%uIsFinished", i);
                joPlayer["isFinished"] = saveGames.getBool(myKey);

                sprintf(myKey, "PL%uColor", i);
                joPlayer["color"] = saveGames.getString(myKey);

                JsonArray jaPieces = joPlayer["Pieces"].to<JsonArray>();
                for (size_t j = 0; j < NUMBEROFPIECESPERPLAYER; j++)
                {
                    sprintf(myKey, "PL%uPIECE%u", i, j);
                    jaPieces.add(saveGames.getUInt(myKey));
                }
            }

            //  Rules
            doc["Rules"]["SafeHomeTile"] = saveGames.getBool("IsHomeSafe");
            doc["Rules"]["AssistedMode"] = saveGames.getBool("AssistedMode");

            saveGames.end();

            doc.shrinkToFit();

            char output[MQTT_BUFFER_SIZE];
            serializeJson(doc, output);

            char fileName[48];
            sprintf(fileName, "ludo-%s-%u.json", savedGameID, myRound);

            // Content-Disposition header
            AsyncWebServerResponse *response = request->beginResponse(200, "application/json", output);

            // "Save As" dialog
            response->addHeader("Content-Disposition", "attachment; filename=\"" + (String)fileName + "\"");

            // Optional: Cache control headers
            response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            response->addHeader("Pragma", "no-cache");
            response->addHeader("Expires", "0");

            request->send(response);

            return;
        }
}

void POSTSaveLoadGame(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    SerialMon.printf("Upload[%s]: start=%u, len=%u, final=%d\r\n", filename.c_str(), index, len, final);

    if (!index)
    {
        // first pass
        StreamString *buffer = new StreamString();
        size_t size = std::max(4094l, request->header("Content-Length").toInt());
        SerialMon.printf("Allocating string buffer of %u bytes\r\n", size);
        if (!buffer->reserve(size))
        {
            delete buffer;
            request->abort();
        }
        request->_tempObject = buffer;
    }

    if (len)
    {
        reinterpret_cast<StreamString *>(request->_tempObject)->write(data, len);
    }
    return request->redirect("/saveloadgame.html");
}

//  Statistics
String StatisticsTemplateProcessor(const String &var)
{
    if (var == "sitename")
        return (String)appSettings.friendlyName;

    if (var == "numberofplayers")
        return (String)GetNumberOfActivePlayers();

    if (var == "round")
        return (String)Statistics.Round;

    for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
    {
        char myVar[20];

        sprintf(myVar, "player%uhidden", i);
        if (var == (String)myVar)
        {
            if (Players[i].IsActive)
                return "";
            else
                return "hidden";
        }

        sprintf(myVar, "player%ucolor", i);
        if (var == (String)myVar)
        {
            char myColor[8];
            sprintf(myColor, "#%02x%02x%02x", Players[i].Color.R, Players[i].Color.G, Players[i].Color.B);
            return (String)myColor;
        }

        sprintf(myVar, "totalpips%u", i);
        if (var == (String)myVar)
            return (String)Statistics.Player[i].TotalPips;

        sprintf(myVar, "emptypips%u", i);
        if (var == (String)myVar)
            return (String)Statistics.Player[i].EmptyPips;

        for (size_t j = 0; j < 6; j++)
        {
            sprintf(myVar, "rolls%u%u", j + 1, i);
            if (var == (String)myVar)
                return (String)Statistics.Player[i].Rolls[j];

            sprintf(myVar, "emptyrolls%u%u", j + 1, i);
            if (var == (String)myVar)
                return (String)Statistics.Player[i].EmptyRolls[j];
        }

        for (size_t j = 0; j < NUMBEROFPLAYERS; j++)
        {
            sprintf(myVar, "hit%u%u", j, i);
            if (var == (String)myVar)
                return (String)Statistics.Player[i].Kicked[j];
        }
    }

    return String();
}

//  Network
String NetworkSettingsTemplateProcessor(const String &var)
{
    if (var == "sitename")
        return (String)appSettings.friendlyName;

    if (var == "networklist")
    {
        int numberOfNetworks = WiFi.scanComplete();
        if (numberOfNetworks == -2)
        {
            WiFi.scanNetworks(true);
        }
        else if (numberOfNetworks)
        {
            String aSSIDs[numberOfNetworks];
            for (int i = 0; i < numberOfNetworks; ++i)
            {
                aSSIDs[i] = WiFi.SSID(i);
            }

            ace_sorting::insertionSort(aSSIDs, numberOfNetworks);

            String wifiList;
            for (size_t i = 0; i < numberOfNetworks; i++)
            {
                wifiList += "<option value='";
                wifiList += aSSIDs[i];
                wifiList += "'";

                if (!strcmp(aSSIDs[i].c_str(), (appSettings.ssid)))
                {
                    wifiList += " selected ";
                }

                wifiList += ">";
                wifiList += aSSIDs[i] + " (" + (String)WiFi.RSSI(i) + ")";
                wifiList += "</option>";
            }
            WiFi.scanDelete();
            if (WiFi.scanComplete() == -2)
            {
                WiFi.scanNetworks(true);
            }
            return wifiList;
        }
    }

    if (var == "password")
        return "";

    return String();
}

void POSTNetworkSettings(AsyncWebServerRequest *request)
{
    const AsyncWebParameter *p;

    if (request->hasParam("lstNetworks", true))
    {
        p = request->getParam("lstNetworks", true);
        sprintf(appSettings.ssid, "%s", p->value().c_str());
    }

    if (request->hasParam("txtPassword", true))
    {
        p = request->getParam("txtPassword", true);
        sprintf(appSettings.password, "%s", p->value().c_str());
    }
}

//  Tools
String ToolsTemplateProcessor(const String &var)
{
    if (var == "sitename")
        return (String)appSettings.friendlyName;

    return String();
}

// Bad request
String BadRequestTemplateProcessor(const String &var)
{
    //  System information
    if (var == "sitename")
        return (String)appSettings.friendlyName;

    return String();
}

//  OTA
void onOTAStart()
{
    SerialMon.println("OTA update started!");
}

void onOTAProgress(size_t current, size_t final)
{
    //  Progress in serial monitor
    if (millis() - ota_progress_millis > 500)
    {
        SerialMon.printf("OTA: %3.0f%%\tTransfered %7u of %7u bytes\tSpeed: %4.3fkB/s\r\n", ((float)current / (float) final) * 100, current, final, ((float)(current - lastCurrent)) / 1024);
        lastCurrent = current;
        ota_progress_millis = millis();
    }
}

void onOTAEnd(bool success)
{
    SerialMon.println("OTA: 100% uploaded.\r\n");

    if (success)
    {
        SerialMon.println("OTA update finished successfully! Restarting...");
    }
    else
    {
        SerialMon.println("There was an error during OTA update!");
    }
}

/// Init
void InitAsyncWebServer()
{
    //  Bootstrap
    server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/bootstrap.bundle.min.js", "text/javascript"); });

    server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/bootstrap.min.css", "text/css"); });

    //  Images
    server.on("/favico.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/favico.ico", "image/x-icon"); });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/favico.icon", "image/x-icon"); });

    server.on("/menu.png", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/menu.png", "image/png"); });

    //  Logout
    server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                  request->requestAuthentication();
                  request->redirect("/"); });

    //  Status
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/status.html", String(), false, StatusTemplateProcessor); });

    server.on("/status.html", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/status.html", String(), false, StatusTemplateProcessor); });

    //  General
    server.on("/general-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/general-settings.html", String(), false, GeneralSettingsTemplateProcessor); });

    server.on("/general-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                  POSTGeneralSettings(request);

                  appSettings.opMode = OPERATION_MODES::NORMAL;
                  appSettings.Save();

                  ESP.restart();

                  request->redirect("/general-settings.html"); });

    //  Players
    server.on("/players-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/players-settings.html", String(), false, PlayersSettingsTemplateProcessor); });

    server.on("/players-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                POSTPlayersSettings(request);

                for (size_t i = 0; i < NUMBEROFPLAYERS; i++)
                    Players[i].Save();

                request->redirect("/players-settings.html"); });

    //  Rules
    server.on("/rules-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/rules-settings.html", String(), false, RulesSettingsTemplateProcessor); });

    server.on("/rules-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                POSTRulesSettings(request);

                appSettings.Save();

                  request->redirect("/rules-settings.html"); });

    //  Save/Load game
    server.on("/saveloadgame.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/saveloadgame.html", String(), false, SaveLoadGameTemplateProcessor); });

    server.on("/saveloadgame.html", HTTP_POST, [](AsyncWebServerRequest *request)
              { POSTSaveLoadGame(request); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
              { POSTSaveLoadGame(request, filename, index, data, len, final); });

    //  Statistics
    server.on("/statistics.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                    return request->requestAuthentication();
                request->send(LittleFS, "/statistics.html", String(), false, StatisticsTemplateProcessor); });

    //  Network
    server.on("/network-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
            return request->requestAuthentication();

        request->send(LittleFS, "/network-settings.html", String(), false, NetworkSettingsTemplateProcessor); });

    server.on("/network-settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        POSTNetworkSettings(request);

        appSettings.opMode = OPERATION_MODES::NORMAL;
        appSettings.Save();

        ESP.restart();

        request->redirect("/network-settings.html"); });

    //  Tools
    server.on("/tools.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
            return request->requestAuthentication();
        request->send(LittleFS, "/tools.html", String(), false, ToolsTemplateProcessor); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
            return request->requestAuthentication();
        ESP.restart(); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
            return request->requestAuthentication();
        appSettings.ClearNVS();
        ESP.restart(); });

    //  Everything else (error)
    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(LittleFS, "/badrequest.html", String(), false, BadRequestTemplateProcessor); });

    //  OTA
    ElegantOTA.setAutoReboot(true);
    ElegantOTA.begin(&server, ADMIN_USERNAME, appSettings.adminPassword);
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);

    server.begin();
    SerialMon.println("AsyncWebServer started.");
}

void loopAsyncWebserver()
{
    ElegantOTA.loop();
}
