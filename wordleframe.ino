#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <Adafruit_MQTT_Client.h>
#include <Adafruit_ThinkInk.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <TimeAlarms.h>
#include <regex.h>
#include "wordleDisplay.h"
#include "wordleRecords.h"
#include "audio.h"
#include "config.h"
#include "network.h"

// Audio sample data
#include "WordleNotice2.h"
#define NOTICE_DATA     WordleNotice2
#define NOTICE_LEN      WordleNotice2_len
#define NOTICE_HZ       9000
#define NOTICE_PCM_PIN  17
#define AUDIO_EN_GPIO   16

// Score Rotate Delay
#define SCORE_ROTATE_DELAY_MS        15000

// Time to give user to enter config mode at boot (0 to disable)
#define STARTUP_CONFIG_DELAY_MS      1500

// Adafruit.io settings
#define AIO_SERVER                   "io.adafruit.com"
#define AIO_SERVERPORT               8883

// Neopixel grid settings
#define NEOPIXEL_PIN                 10
#define NEOPIXEL_NOT_ENABLE_PIN      21

// E-ink settings
#define EPD_DC      7  // can be any pin, but required!
#define EPD_CS      8  // can be any pin, but required!
#define EPD_BUSY    5  // can set to -1 to not use a pin (will wait a fixed delay)
#define SRAM_CS     -1 // can set to -1 to not use a pin (uses a lot of RAM!)
#define EPD_RESET   6  // can set to -1 and share with chip Reset (can't deep sleep)
#define DISPLAY_W   296
#define DISPLAY_H   128

// root CA cert for io.adafruit.com:883
static const char *ssl_cert PROGMEM = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
"-----END CERTIFICATE-----\n";

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define ARRAY_SIZE(a)                               \
  ((sizeof(a) / sizeof(*(a))) /                     \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

typedef enum disp_state {NORMAL, SHOW_SETUP, SHOW_WIFI, SHOW_MQTT} disp_state;
disp_state g_state = SHOW_SETUP;

// Config data stored in EEPROM
Config config;

// Handle wifi
Network wifi;

// WiFiFlientSecure for SSL/TLS support
WiFiClientSecure client;

// Create the mqtt client, username/key will be supplied later
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT);
Adafruit_MQTT_Subscribe *mqtt_feed;

// Parsed data
WordleRecords records;

// LED matrix
WordleDisplay leds(NEOPIXEL_PIN);

// E-Ink Display
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

void setup(void)
{
  int i, in;
  unsigned long begintime;

  leds.begin();

  display.begin(THINKINK_GRAYSCALE4);
  display.setTextColor(EPD_BLACK);
  display.setTextWrap(false);
  display.clearBuffer();
  display.display();

  // Enable NeoPixels
  pinMode(NEOPIXEL_NOT_ENABLE_PIN, OUTPUT);
  digitalWrite(NEOPIXEL_NOT_ENABLE_PIN, LOW);

  // Enable audio
  pinMode(AUDIO_EN_GPIO, OUTPUT);
//  digitalWrite(AUDIO_EN_GPIO, LOW);
  digitalWrite(AUDIO_EN_GPIO, LOW);

  Serial.begin(115200);

  if (!config.begin()) {
    UpdateDisplayState(SHOW_SETUP);
    config.reconfig();
  } else {
    if (STARTUP_CONFIG_DELAY_MS) {
      Serial.println("");
      Serial.print("Press any key to force reconfig");
      begintime = millis();
      while (!Serial.available()) {
        if (((millis() - begintime) >= STARTUP_CONFIG_DELAY_MS)) {
          break;
        }
        Serial.print(".");
        delay(80);
      }
      Serial.println("");
      if (Serial.available()){
        UpdateDisplayState(SHOW_SETUP);
        config.reconfig();
      }
    }
  }

  mqtt_feed = new Adafruit_MQTT_Subscribe(&mqtt, config.adafruit_feed(), MQTT_QOS_1);
  mqtt_feed->setCallback(incoming_callback);
  mqtt.subscribe(mqtt_feed);
}

/* Have to wrap the function due to the differing return types */
void incoming_callback(char *data, uint16_t len) {
  records.add(data, len);
}

void loop(void)
{
  int scale = 0;
  bool new_record = false;
  static unsigned long last_score_time = 0;
  bool wifi_up = wifi.isConnected();

  if (!wifi_up) {
    g_state = SHOW_WIFI;
    wifi.disconnect();
    Serial.println("Wifi is down; try to bring it up:");
    if (wifi.connectWifi(config.ssid(), config.pass())) {
      client.setCACert(ssl_cert);
      wifi_up = true;
      g_state = SHOW_MQTT;
      Alarm.alarmRepeat(0,0,0, ResetBoard);  // 12:00am every day
      Serial.println("Wifi is up");
    }
  }

  if (wifi_up && MQTT_connect()) {
    g_state = NORMAL;
    mqtt.processPackets(100);
    // ping the server to keep the mqtt connection alive
    if(! mqtt.ping()) {
      Serial.println("MQTT is down");
      mqtt.disconnect();
      g_state = SHOW_WIFI;
    }
  } else {
    Serial.printf("Unable to connect to MQTT (wifi: %d)\r\n", wifi_up);
  }

  UpdateDisplayState(g_state);

  new_record = records.isNewRecordAvail();
  if (new_record ||
      (((millis() - last_score_time) >= SCORE_ROTATE_DELAY_MS) && records.shouldRefresh()))
  {
    record current = records.get();
    if (current.score)
    {
      leds.clear();
      UpdateDisplay(current);           // Update the e-ink display
      if (new_record)
      {
        digitalWrite(AUDIO_EN_GPIO, HIGH);
        records.printRecord(current);
        Play(NOTICE_DATA, NOTICE_LEN, NOTICE_HZ, NOTICE_PCM_PIN, false);
      }
      leds.show(current.results, current.score, true); // Update the LEDs
      /* Sound should be done playing by now */
      if (new_record)
      {
        digitalWrite(AUDIO_EN_GPIO, LOW);
      }
    }
    last_score_time = millis();
  }
}

void UpdateDisplayState(disp_state state)
{
  static disp_state old_state = NORMAL;

  if (state != old_state)
  {
    switch(state) {
      case SHOW_SETUP:
        UpdateDisplay("Setup required");
        break;
      case SHOW_WIFI:
        UpdateDisplay("Connecting to wifi...");
        break;
      case SHOW_MQTT:
        UpdateDisplay("Connecting to backend...");
        break;
      default:
        // fallthrough and let regular display update happen
        break;
    }
  }
}

void UpdateDisplay(const char *string)
{
  display.setFont(&FreeSansBold12pt7b);
  display.clearBuffer();
  display.setCursor(5, 20);

  display.print(string);
  display.display();
}

void UpdateDisplay(record r)
{
  int16_t user_x, wordle_x, y, x, x1, y1;
  bool user_smallfont = false;
  uint16_t w, h;
  char buf[80];

  // See if user will fit at larger font
  display.setFont(&FreeSansBold18pt7b);
  display.getTextBounds(r.user, 0, 0, &x1, &y1, &w, &h);
  if (w > (DISPLAY_W - 30)) {
    // If not, recalculate width at smaller font
    display.setFont(&FreeSansBold12pt7b);
    display.getTextBounds(r.user, 0, 0, &x1, &y1, &w, &h);
    user_smallfont = true;
  }
  user_x = (DISPLAY_W - w) / 2;

  // Figure out center point of Wordle string
  snprintf(buf, 80, "Wordle %d %d/%d%c", r.gameno,
           r.score, GUESSES, r.hard?'*':'\0');
  display.setFont(&FreeSansBold18pt7b);
  display.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  wordle_x = (DISPLAY_W - w) / 2;

  // Center based on the wider of the two
  x = MIN(user_x, wordle_x);
  
  display.clearBuffer();
  if (user_smallfont)
    display.setFont(&FreeSansBold12pt7b);
  display.setCursor(x, 40);
  display.print(r.user);
  display.setFont(&FreeSansBold18pt7b);
  y = DISPLAY_H - h - 10;
  display.setCursor(x, y);
  display.print(buf);
  display.display();
}

void ResetBoard(void) {
  Serial.println("Midnight: reset board");
  records.reset();
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
bool MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return true;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 2;
  while (retries &&
         (ret = mqtt.connect(config.adafruit_user(), config.adafruit_key())) != 0) {
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
  }

  if (ret == 0) {
    Serial.println("MQTT Connected!");
    return true;
  }

  Serial.println("Failed to connect to MQTT");
  return false;
}
