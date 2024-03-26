#ifndef __WIRELESS_H__
#define __WIRELESS_H__

#include <WiFi.h>
#include <HTTPClient.h>
#include "lilygo.hpp"
#include "config.hpp"

class Wireless{
public:
    Wireless();
    ~Wireless();

    void Enable();
    void Disable();

    bool IsConnected() const;
    bool HttpPost(const char* url);

private:
    bool enabled_;

};

#endif // __WIRELESS_H__