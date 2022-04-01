#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"

// increment to force reconfig in case of EEPROM data layout change
#define EEPROM_VERSION            0x0102

#define MAX_LINE_SIZE             80
#define BLANK_EEPROM_BYTE         0xff

Config::Config(void)
{
}

// Reads config data from EEPROM.
// Returns true if valid config loaded
// Returns false if a manual reconfig is needed
bool Config::begin(void)
{
  EEPROM.begin(sizeof(struct config_data));
  config = (struct config_data *)EEPROM.getDataPtr();

  /* Make sure the strings are sane */
  if (config->ssid[0] == BLANK_EEPROM_BYTE)
    config->ssid[0] = '\0';
  else
    config->ssid[MAX_CREDS_LEN-1] = '\0';

  if (config->pass[0] == BLANK_EEPROM_BYTE)
    config->pass[0] = '\0';
  else
    config->pass[MAX_CREDS_LEN-1] = '\0';

  if (config->adafruit_user[0] == BLANK_EEPROM_BYTE)
    config->adafruit_user[0] = '\0';
  else
    config->adafruit_user[MAX_CREDS_LEN-1] = '\0';

  if (config->adafruit_key[0] == BLANK_EEPROM_BYTE)
    config->adafruit_key[0] = '\0';
  else
    config->adafruit_key[MAX_CREDS_LEN-1] = '\0';

  if (config->adafruit_feed[0] == BLANK_EEPROM_BYTE)
    config->adafruit_feed[0] = '\0';
  else
    config->adafruit_feed[MAX_CREDS_LEN-1] = '\0';


  Serial.printf("Loaded %d bytes of data from EEPROM\r\n", sizeof(struct config_data));
#ifdef DEBUG
  dumpConfig();
#endif

  if ((config->ssid[0] == '\0') ||
      (config->pass[0] == '\0') ||
      (config->adafruit_user[0] == '\0') ||
      (config->adafruit_key[0] == '\0') ||
      (config->adafruit_feed[0] == '\0') ||
      (config->version != EEPROM_VERSION) )
  {
    Serial.println("Configuration invalid");
    return false;
  }
  return true;
}

#ifdef DEBUG
void Config::dumpConfig(void)
{
#if 1
  Serial.printf("config raw data (%d bytes):\r\n", sizeof(struct config_data));
  for(int i=0; i < sizeof(struct config_data); i++) {
    Serial.printf("%02x ", ((uint8_t *)config)[i]);
    if (i%16 == 15)
      Serial.println();
  }
  Serial.println();
#endif
  Serial.printf(" ver: [0x%x] (offset %d)\r\n", config->version, (((uint8_t *)&(config->version))-(uint8_t *)config));
  Serial.printf("ssid: [%s] (offset %d)\r\n", config->ssid, (((uint8_t *)&(config->ssid))-(uint8_t *)config));
  Serial.printf("pass: [%s] (offset %d)\r\n", config->pass, (((uint8_t *)&(config->pass))-(uint8_t *)config));
  Serial.printf("user: [%s] (offset %d)\r\n", config->adafruit_user, (((uint8_t *)&(config->adafruit_user))-(uint8_t *)config));
  Serial.printf(" key: [%s] (offset %d)\r\n", config->adafruit_key, (((uint8_t *)&(config->adafruit_key))-(uint8_t *)config));
  Serial.printf("feed: [%s] (offset %d)\r\n", config->adafruit_feed, (((uint8_t *)&(config->adafruit_feed))-(uint8_t *)config));
}
#endif


void Config::reconfig()
{
  int i;
  char buff[MAX_LINE_SIZE];

  snprintf(buff, MAX_LINE_SIZE, "SSID [%s]: ", config->ssid);
  readFromSerial(buff, buff, MAX_CREDS_LEN, 0, false);
  if (strlen(buff))
  {
    strcpy(config->ssid, buff);
  }

  strcpy(buff, "Password [");
  for (i = 0; i < strlen(config->pass); i++)
  {
    strcat(buff, "*");
  }
  strcat(buff, "]: ");
  readFromSerial(buff, buff, MAX_CREDS_LEN, 0, true);
  if (strlen(buff))
  {
    strcpy(config->pass, buff);
  }

  snprintf(buff, MAX_LINE_SIZE, "adafruit.io username [%s]: ", config->adafruit_user);
  readFromSerial(buff, buff, MAX_CREDS_LEN, 0, false);
  if (strlen(buff))
  {
    strcpy(config->adafruit_user, buff);
  }

  snprintf(buff, MAX_LINE_SIZE, "adafruit.io key [%s]: ", config->adafruit_key);
  readFromSerial(buff, buff, MAX_CREDS_LEN, 0, false);
  if (strlen(buff))
  {
    strcpy(config->adafruit_key, buff);
  }

  char *lastslash = config->adafruit_feed;
  for (i = 0; i < strlen(config->adafruit_feed); i++)
  {
    if (config->adafruit_feed[i] == '/') 
    {
      lastslash = &(config->adafruit_feed[i+1]);
    }
  }
  snprintf(buff, MAX_LINE_SIZE, "adafruit.io feed name [%s]: ", lastslash);
  readFromSerial(buff, buff, MAX_CREDS_LEN, 0, false);
  if (strlen(buff))
  {
    char buff2[MAX_LINE_SIZE];
    snprintf(buff2, MAX_LINE_SIZE, "%s/feeds/%s", config->adafruit_user, buff);
    strcpy(config->adafruit_feed, buff2);
  }

  config->version = EEPROM_VERSION;

  save();
}

void Config::save()
{
#ifdef DEBUG
  Serial.printf("Saving %d bytes of data to EEPROM:\r\n", sizeof(struct config_data));
  dumpConfig();
#endif
  EEPROM.commit();
}

const char *Config::ssid()
{
  return config->ssid;
}

const char *Config::pass()
{
  return config->pass;
}

const char *Config::adafruit_user()
{
  return config->adafruit_user;
}

const char *Config::adafruit_key()
{
  return config->adafruit_key;
}

const char *Config::adafruit_feed()
{
  return config->adafruit_feed;
}


/* Promt the user, then read a string back.  Reading ends when max limit
 * reached, timeout occurs or CR encountered.  Newline is printed at the end
 *
 *    prompt   - message to present to user
 *    buf      - location to store user input
 *    maxLen   - buf length
 *    timeout  - timeout waiting for user input (returns data entered so far)
 *    hide     - 1: show '*'; 0: show user input
 *
 *    returns true if value is read
 */
bool Config::readFromSerial(char * prompt, char * buf, int maxLen, int timeout, bool hide)
{
    unsigned long begintime = millis();
    bool timedout = false;
    int loc=0;
    char newchar;

    if(maxLen <= 0)
    {
        // nothing can be read
        return false;
    }

    /* consume all the pending serial data first */
    while (0xFF != (newchar = Serial.read())) {
      delay(5);
    };

    Serial.print(prompt);
    do {
        while (0xFF == (newchar = Serial.read())) {
            delay(10);
            if ((timeout > 0) && ((millis() - begintime) >= timeout)) {
              break;
            }
        }
        buf[loc++] = newchar;
        if (hide)
            Serial.print('*');
        else
            Serial.print((char)buf[loc-1]);

        if (timeout > 0)
            timedout = ((millis() - begintime) >= timeout);
      
      /* stop at max length, CR or timeout */
    } while ((loc < maxLen) && (buf[loc-1] != '\r') && !timedout);

    /* If carriage return was cause of the break, then erase it */
    if ((loc > 0) && (buf[loc-1] == '\r')) {
        loc--;
    }

    /* NULL terminate if there's room, but sometimes 1 single char is passed */
    if (loc < maxLen)
        buf[loc] = '\0';

    Serial.println("");
    return true;
}
