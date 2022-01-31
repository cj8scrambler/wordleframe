#ifndef __CONFIG_H
#define __CONFIG_H

#define MAX_CREDS_LEN                  40   // Max string length for adafruit.io/wifi credentials

struct config_data {
  uint16_t version;
  char ssid[MAX_CREDS_LEN];
  char pass[MAX_CREDS_LEN];
  char adafruit_user[MAX_CREDS_LEN];
  char adafruit_key[MAX_CREDS_LEN];
  char adafruit_feed[MAX_CREDS_LEN];
};

class Config
{
  public:
    Config(void);
    bool begin(void);
    void reconfig(void);
    void dump(void);
    const char *ssid(void);
    const char *pass(void);
    const char *adafruit_user(void);
    const char *adafruit_key(void);
    const char *adafruit_feed(void);
#ifdef DEBUG
    void dumpConfig(void);
#endif
  private:
    struct config_data *config;
    void save(void);
    bool readFromSerial(char * prompt, char * buf, int maxLen, int timeout, bool hide);
};
#endif // __CONFIG_H
