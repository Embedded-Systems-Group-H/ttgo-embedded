#ifndef __APP_H__
#define __APP_H__

#include "button.hpp"
#include "sprite.hpp"
#include "watch_interface.hpp"

class App{
public:
    static App& Get();
    
    void Init();
    void Update();

private:
    void clearScreen();

    int buttonCount_;
    Button** buttons_;

    int spriteCount_;
    Sprite** sprites_;

    Sprite* spriteTime_ = nullptr;
    Sprite* spriteLocation_ = nullptr;
    Sprite* spriteCoordinate_ = nullptr;
    Sprite* spriteStepCount_ = nullptr;
    Sprite* spriteWiFi_ = nullptr;

    WatchInterface* watch_ = nullptr;

private:
    void TouchCallback(uint16_t x, uint16_t y);
    void ButtonCallback();
    void StepCountCallback(uint32_t stepCount);
    void GpsCallback(bool isValid, double lat, double lng);
    void WifiStatusCallback(bool isConnected);
    void TimeUpdatedCallback(GpsTime gpsTime);

    bool StartSession();
    bool EndSession();
    bool SendGps(double lat, double lng);
    bool SendStepCount(uint32_t stepCount);

private:
    App();
    App(App&&) = delete;
    App(const App&) = delete;
    App& operator=(App&&) = delete;
    App& operator=(const App&) = delete;

};

#endif // __APP_H__