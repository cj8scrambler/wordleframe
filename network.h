#ifndef __NETWORK_H
#define __NETWORK_H

#include <WiFi.h>

class Network
{
  public:
    Network();
    bool isConnected();
    bool connectWifi(const char *ssid, const char *passwd);
    void disconnect(void);
    time_t getTime();
  private:
    bool _init;
};

#endif // __NETWORK_H
