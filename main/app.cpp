#include "app.hpp"
#include "config.hpp"

static String base_url = String("http://") + String(SERVER_IP) + String(":") + String(SERVER_PORT) + String("/api/");
static char buffer[128];

struct{
    String sessionId;
} static session;

bool App::StartSession(){
    session.sessionId = String(now());
    String url = base_url + String("session_start/") + sessionId;
    return watch_->GetWirelessInterface()->HttpPost(url.c_str());
}

bool App::EndSession(){
    String url = base_url + String("session_end/") + sessionId;
    session.sessionId = "";
    return watch_->GetWirelessInterface()->HttpPost(url.c_str());
}

bool App::SendGps(double lat, double lng){
    if(session.sessionId.length() == 0)
        return false;
    String payload = String("ts=") + now();
    String latitude = String("lat=") + String(lat);
    String longitude = String("long=") + String(lng);
    payload = payload + "&" + latitude + "&" + longitude;
    String url = base_url + String("gps/") + session.sessionId;
    url += String("?") + payload;
    return watch_->GetWirelessInterface()->HttpPost(url.c_str());
}

bool App::SendStepCount(uint32_t stepCount){
    if(session.sessionId.length() == 0)
        return false;
    String payload = String("ts=") + now();
    String stepCountStr = String("count=") + String(stepCount);
    payload = payload + "&" + stepCountStr;
    String url = base_url + String("step_count/") + session.sessionId;
    url += String("?") + payload;
    return watch_->GetWirelessInterface()->HttpPost(url.c_str());
}

App& App::Get() {
    static App app;
    return app;    
}

void App::Init() {
    watch = &WatchInterface::Get();
    watch_->Init();

    watch_->SetTouchCallback([this](uint16_t x, uint16_t y){ this->TouchCallback(x, y); });
    watch_->SetButtonCallback([this](){ this->ButtonCallback(); });
    watch_->SetStepCountCallback([this](uint32_t stepCount){ this->StepCountCallback(stepCount); });
    watch_->SetGpsCallback([this](double lat, double lng){ this->GpsCallback(lat, lng); });
    watch_->SetWifiStatusCallback([this](bool isConnected){ this->WifiStatusCallback(isConnected); });
    watch_->SetTimeUpdatedCallback([this](GpsTime gpsTime){ this->TimeUpdatedCallback(gpsTime); });

    auto tft = watch_->GetTFT();
    spriteCount_ = 5;
    sprites_ = new Sprite[spriteCount_];
    sprites_[0] = new Sprite(0, 0*24, 240, 1*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);
    sprites_[1] = new Sprite(0, 1*24, 240, 2*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);
    sprites_[2] = new Sprite(0, 2*24, 240, 3*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);
    sprites_[3] = new Sprite(0, 3*24, 240, 4*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);
    sprites_[4] = new Sprite(0, 4*24, 240, 5*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);

    spriteTime_ = g_Sprites[0];
    spriteLocation_ = g_Sprites[1];
    spriteCoordinate_ = g_Sprites[2];
    spriteStepCount_ = g_Sprites[3];
    spriteWiFi_ = g_Sprites[4];

    buttonCount_ = 2;
    buttons_ = new Button[buttonCount_];

    buttons_[0] = new Button(10, 120, 240/3 - 3, 180, new TFT_eSprite(tft), "Start", TFT_WHITE, TFT_BLUE, StartSession);
    buttons_[1] = new Button(240/3 + 3, 120, 2*240/3 - 3, 180, new TFT_eSprite(tft), "End", TFT_WHITE, TFT_BLUE, EndSession);
}

App::App() {
    
}

void App::TouchCallback(uint16_t x, uint16_t y) {
    bool result = false;
    for(int i=0; i<buttonCount && !result; i++){
        result = buttons[i]->HandleEvent(x, y);
    }

    sprintf(buffer, "x=%03d  y=%03d", x, y);
    sprite_Coordinate->Render(buffer);

    watch_->VibrateOnce();
}

void App::ButtonCallback() {
    watch_->VibrateOnce();

    if(watch_->IsScreenOn()) {
        watch_->SetScreen(false);
    }
    else{
        watch_->SetScreen(true);
        clearScreen();
    }
}

void App::StepCountCallback(uint32_t stepCount) {
    sprintf(buffer, "Step Count: %05d", stepCount);
    sprite_StepCount->Render(buffer);
    send_step_count();
}

void App::GpsCallback(double lat, double lng) {
    sprintf(buffer, "Location: lat %.6f long %.6f", lat, lng);
    sprite_Location->Render(buffer);
    send_gps(lat, lng);
}

void App::WifiStatusCallback(bool isConnected) {
    sprintf(buffer, "Wifi status: %s", (isConnected ? "Connected" : "Disconnected"));
    spriteWiFi_->Render(buffer);
}

void App::TimeUpdatedCallback(GpsTime gpsTime) {
    sprintf(buffer, "Time: %02d:%02d:%02d", hour(), minute(), second());
    spriteTime_->Render(buffer);
}

void App::clearScreen() {
    watch_->GetTFT()->fillScreen(TFT_BLACK);
    watch_->GetTFT()->setCursor(0, 0);

    for(int i=0; i<buttonCount; i++){
        auto b = buttons[i];
        b->Render();
    }

    sprintf(buffer, "Location: INVALID");
    sprite_Location->Render(buffer);
}

void App::Update() {
    watch_->Update();    
}