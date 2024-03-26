#ifndef __WATCH_INTERFACE_H__
#define __WATCH_INTERFACE_H__

#include "lilygo.hpp"
#include "wireless.hpp"
#include <TimeLib.h>

struct GpsTime{
    int year=2000, month=1, day=1, hour=0, minute=0, second=0;
};

class WatchInterface{
public:
    static WatchInterface& Get();

    void Init();
    void Update();

    bool IsScreenOn() const;
    Wireless* GetWirelessInterface();
    TFT_eSPI* GetTFT();

    void VibrateOnce();
    void SetScreen(bool turnOn);

    void SetTouchCallback(void(*touchCallback)(uint16_t, uint16_t));
    void SetButtonCallback(void(*buttonCallback)());
    void SetStepCountCallback(void(*stepCountCallback)(uint32_t));
    void SetGpsCallback(void(*gpsCallback)(double, double));
    void SetWifiStatusCallback(void(*wifiStatusCallback)(bool));
    void SetTimeUpdatedCallback(void(*timeUpdatedCallback)(GpsTime));
    
private:
    ~WatchInterface();

    void InitializeHardwareInterfaces();
    bool Quectel_L76X_Probe();

private:
    TTGOClass* ttgo_;
    TFT_eSPI* tft_;
    Buzzer* buzzer_;
    Adafruit_DRV2605* drv_;
    BMA* sensor_;
    TinyGPSPlus* gps_;
    HardwareSerial* gnss_;
    Wireless wireless_;

    bool touched_;
    bool touchedJustNow_;
    bool lastWifiStatus_;
    GpsTime gpsTime_;

    void(*touchCallback_)(uint16_t, uint16_t);
    void(*buttonCallback_)();
    void(*stepCountCallback_)(uint32_t);
    void(*gpsCallback_)(double, double);
    void(*wifiStatusCallback_)(bool);
    void(*timeUpdatedCallback_)(GpsTime);

    inline static bool pmu_irq_ = 0;
    inline static bool step_irq_ = 0;

private:
    WatchInterface();
    WatchInterface(WatchInterface&&) = delete;
    WatchInterface(const WatchInterface&) = delete;
    WatchInterface& operator=(WatchInterface&&) = delete;
    WatchInterface& operator=(const WatchInterface&) = delete;

};

#endif // __WATCH_INTERFACE_H__