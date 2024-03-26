#include "wireless.hpp"

Wireless::Wireless() :
    enabled_(false)
{
}

void Wireless::Enable() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    enabled_ = true;
}

void Wireless::Disable() {
    enabled_ = false;
    // WiFi.kill ?
}

bool Wireless::IsConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

bool Wireless::HttpPost(const char* url) {
    if(!IsConnected())
        return false;

    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.POST("");

    bool success = httpResponseCode > 0;
    if(success){
        // Serial.print("HTTP Response code:");
        // Serial.println(httpResponseCode);
        // String payload = http.getString();
        // Serial.println(payload);
    }
    else{
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    http.end();

    return success;
}
