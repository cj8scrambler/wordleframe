#include <time.h>
#include <TimeLib.h>
#include "network.h"

#define WIFI_WAIT_SEC                   15    // Wait time for wifi
#define TIME_WAIT_SEC                   5     // Wait time for time sync
#define MIN_EPOCH     (49 * 365 * 24 * 3600)  // 1/1/2019

Network::Network()
{
    _init = false;
}

bool Network::isConnected()
{
    return (_init && (WiFi.status() == WL_CONNECTED));
}

void Network::disconnect()
{
    WiFi.mode(WIFI_OFF);
}

bool Network::connectWifi(const char *ssid, const char *pass)
{
    int attempts = 0;
    time_t epochTime = 0;

    if (_init && WiFi.status() == WL_CONNECTED)
    {
        return true;
    }
#ifdef DEBUG
    Serial.printf("Connecting to SSID: %s", ssid);
//    Serial.setDebugOutput(true);
#endif

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    delay(500);
    while ((attempts++ < (WIFI_WAIT_SEC * 2)) && (WiFi.status() != WL_CONNECTED))
    {
#ifdef DEBUG
        Serial.print(".");
#endif
        delay(500);
    }
#ifdef DEBUG
//    Serial.setDebugOutput(false);
    Serial.println("");
#endif

    if (attempts >= (WIFI_WAIT_SEC * 2)) {
        Serial.printf("Unable to connect to wifi %s.\r\n", ssid);
        return false;
    }

#ifdef DEBUG
    Serial.printf("Connected to: %s\r\n", ssid);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
#endif

#ifdef DEBUG
    Serial.println("Updating time");
#endif
    attempts = 0;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    while ((attempts++ < (TIME_WAIT_SEC * 2)) && (epochTime < MIN_EPOCH))
    {
#ifdef DEBUG
        Serial.println("");
#endif
        delay(500);
        epochTime = time(NULL);
    }

    if (attempts >= (TIME_WAIT_SEC * 2)) {
        Serial.printf("Unable to get time\r\n");
        return false;
    }

    setTime(epochTime);

#ifdef DEBUG
    Serial.printf("Time found: %d\r\n", epochTime);
#endif
    _init = true;
    return true;
}

time_t Network::getTime(void)
{
    return (time(NULL));
}
