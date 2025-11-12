#include <Arduino.h>
#include <Update.h>

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
        AsyncWebParameter *p = request->getParam(i);
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
        AsyncWebHeader *h = request->getHeader(i);
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
    AsyncWebParameter *p;

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

    if (request->hasParam("txtTopic", true))
    {
        p = request->getParam("txtTopic", true);
        sprintf(appSettings.mqttTopic, "%s", p->value().c_str());
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
    AsyncWebParameter *p;

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
    AsyncWebParameter *p;

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
    Serial.println("OTA update started!");
}

void onOTAProgress(size_t current, size_t final)
{
    //  Progress in serial monitor
    if (millis() - ota_progress_millis > 500)
    {
        Serial.printf("OTA: %3.0f%%\tTransfered %7u of %7u bytes\tSpeed: %4.3fkB/s\r\n", ((float)current / (float) final) * 100, current, final, ((float)(current - lastCurrent)) / 1024);
        lastCurrent = current;
        ota_progress_millis = millis();
    }
}

void onOTAEnd(bool success)
{
    Serial.println("OTA: 100% uploaded.\r\n");

    if (success)
    {
        Serial.println("OTA update finished successfully! Restarting...");
    }
    else
    {
        Serial.println("There was an error during OTA update!");
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

    //  Network
    server.on("/network-settings.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
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
                    if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                        return request->requestAuthentication();
                    request->send(LittleFS, "/tools.html", String(), false, ToolsTemplateProcessor); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                        return request->requestAuthentication();
                    ESP.restart(); });

    server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                    if(!request->authenticate(ADMIN_USERNAME, appSettings.adminPassword))
                        return request->requestAuthentication();
                    appSettings.Load(true);
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