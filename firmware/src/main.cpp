#include <WiFi.h>
#include <ESPmDNS.h>

#include "main.h"
#include "version.h"
#include "common.h"
#include "asyncwebserver.h"
#include "mqtt.h"
#include "leds.h"
#include "ntp.h"
#include "settings.h"
#include "sound.h"

#include "game.h"

settings appSettings;

bool isAccessPointCreated = false;
bool needsRestart = false;
bool ntpInitialized = false;

unsigned long WifiMillis = 0;
uint8_t oldSeconds = -1;

enum CONNECTION_STATE
{
    STATE_CHECK_WIFI_CONNECTION,
    STATE_WIFI_CONNECT,
    STATE_CHECK_INTERNET_CONNECTION,
    STATE_INTERNET_CONNECTED
};

enum CONNECTION_STATE connectionState;

void setup()
{
    SerialMon.begin(DEBUG_SPEED);

    SerialMon.printf("\r\n\n\nBooting ESP node %u...\r\n", GetChipID());
    SerialMon.println("Hardware ID:      " + String(HARDWARE_ID));
    SerialMon.println("Hardware version: " + String(HARDWARE_VERSION));
    SerialMon.println("Software ID:      " + String(FIRMWARE_ID));
    SerialMon.println("Software version: " + String(FIRMWARE_VERSION));
    SerialMon.println();

    appSettings.Load();

    setupLEDs();
    connectionLED_OFF();
    setupMQTT();
    setupGame();
    setupSound();

    connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;

    //  Finished setup()
    SerialMon.println("Setup finished successfully.");
}

void loop()
{
    if (appSettings.opMode == OPERATION_MODES::WIFI_SETUP)
    {
        if (millis() - WifiMillis > MAX_WIFI_INACTIVITY * 1000)
        {
            SerialMon.println("WiFi mode is now over. Restarting in Normal mode....");
            appSettings.opMode = OPERATION_MODES::NORMAL;
            appSettings.Save();
            ESP.restart();
        }

        if (!isAccessPointCreated)
        {
            SerialMon.println("Creating Acces Point...");

            delay(500);

            WiFi.mode(WIFI_AP);
            WiFi.softAP(appSettings.AccessPointSSID, appSettings.AccessPointPassword);
            WiFi.setSleep(false);

            IPAddress myIP = WiFi.softAPIP();
            isAccessPointCreated = true;

            SerialMon.println("Access point created. Use the following information to connect to the ESP device:");

            SerialMon.printf("SSID:\t\t\t%s\r\nPassword:\t\t%s\r\nAccess point address:\t%s\r\n",
                             appSettings.AccessPointSSID, appSettings.AccessPointPassword, myIP.toString().c_str());

            InitAsyncWebServer();

            WifiMillis = millis();
        }
    }
    else if (appSettings.opMode == OPERATION_MODES::NORMAL)
    {
        switch (connectionState)
        {

        // Check the WiFi connection
        case STATE_CHECK_WIFI_CONNECTION:

            // Are we connected ?
            if (WiFi.status() != WL_CONNECTED)
            {
                // Wifi is NOT connected
                connectionLED_OFF();
                connectionState = CONNECTION_STATE::STATE_WIFI_CONNECT;
            }
            else
            {
                // Wifi is connected so check Internet
                connectionLED_ON();
                connectionState = CONNECTION_STATE::STATE_CHECK_INTERNET_CONNECTION;
            }
            break;

        // No Wifi so attempt WiFi connection
        case STATE_WIFI_CONNECT:
        {
            // Indicate NTP no yet initialized
            ntpInitialized = false;

            connectionLED_OFF();
            SerialMon.printf("Trying to connect to WIFI network: %s", appSettings.ssid);

            // Set station mode
            WiFi.mode(WIFI_MODE_NULL);

            // Start connection process
            WiFi.setHostname(appSettings.hostName);
            WiFi.mode(WIFI_STA);

            WiFi.begin(appSettings.ssid, appSettings.password);
            WiFi.setSleep(false);

            // Initialize iteration counter
            uint8_t attempt = 0;

            while ((WiFi.status() != WL_CONNECTED) && (attempt++ < WIFI_CONNECTION_TIMEOUT))
            {
                connectionLED_ON();
                SerialMon.printf(".");
                delay(50);
                connectionLED_OFF();
                delay(950);
            }

            if (attempt >= WIFI_CONNECTION_TIMEOUT)
            {
                SerialMon.println("\r\nCould not connect to WiFi.");
                delay(100);

                appSettings.opMode = OPERATION_MODES::WIFI_SETUP;

                appSettings.Save();
                ESP.restart();

                break;
            }

            SerialMon.printf(" Success.\r\n");
            SerialMon.print("IP address: ");
            SerialMon.println(WiFi.localIP());
            SerialMon.printf("Hostname: %s\r\n", appSettings.hostName);

            if (MDNS.begin(appSettings.hostName))
                SerialMon.println("MDNS responder started.");

            InitAsyncWebServer();

            connectionState = CONNECTION_STATE::STATE_CHECK_INTERNET_CONNECTION;
        }
        break;

        case STATE_CHECK_INTERNET_CONNECTION:

            // Do we have a connection to the Internet ?
            if (checkInternetConnection())
            {
                // We have an Internet connection

                if (!ntpInitialized)
                {
                    // We are connected to the Internet for the first time so set NTP provider
                    setupNTP();

                    ntpInitialized = true;

                    SerialMon.println("Connected to the Internet.");
                }

                connectionState = CONNECTION_STATE::STATE_INTERNET_CONNECTED;
            }
            else
            {
                connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;
            }
            break;

        case STATE_INTERNET_CONNECTED:

            loopNTP();
            loopMQTT();
            loopAsyncWebserver();
            loopSound();
            loopGame();

            // Set next connection state
            connectionState = CONNECTION_STATE::STATE_CHECK_WIFI_CONNECTION;
            break;
        }
    }
}
