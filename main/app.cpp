#include "app.hpp"
#include "config.hpp"

static App* g_App;

static String base_url = String("http://") + String(SERVER_IP) + String(":") + String(SERVER_PORT) + String("/api/");
static char buffer[128];

struct{
    String sessionId;
    int latestStepCount = 0;
    int startStepCount = 0;
} static session;

bool App::StartSession(){
    session.sessionId = String(now());
    String url = base_url + String("session_start/") + session.sessionId;
    session.startStepCount = session.latestStepCount;
    StepCountCallback(session.latestStepCount);
    buttons_[0]->SetBackgroundColor(TFT_GREEN);
    buttons_[0]->Render();

    return watch_->GetWirelessInterface()->HttpPost(url.c_str());
}

bool App::EndSession(){
    String url = base_url + String("session_end/") + session.sessionId;
    session.sessionId = "";
    buttons_[0]->SetBackgroundColor(TFT_BLUE);
    buttons_[0]->Render();
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
    watch_ = &WatchInterface::Get();
    if(watch_->Init()){
        g_App = this;
        watch_->SetTouchCallback([](uint16_t x, uint16_t y){ g_App->TouchCallback(x, y); });
        watch_->SetButtonCallback([](){ g_App->ButtonCallback(); });
        watch_->SetStepCountCallback([](uint32_t stepCount){ g_App->StepCountCallback(stepCount); });
        watch_->SetGpsCallback([](bool isValid, double lat, double lng){ g_App->GpsCallback(isValid, lat, lng); });
        watch_->SetWifiStatusCallback([](bool isConnected){ g_App->WifiStatusCallback(isConnected); });
        watch_->SetTimeUpdatedCallback([](GpsTime gpsTime){ g_App->TimeUpdatedCallback(gpsTime); });
    }

    auto tft = watch_->GetTFT();
    spriteCount_ = 5;
    sprites_ = new Sprite*[spriteCount_];

    spriteTime_ =           sprites_[0] = new Sprite(0, 2*24, 120, 3*24, new TFT_eSprite(tft), TFT_BLACK, TFT_WHITE, 2);
    spriteWiFi_ =           sprites_[4] = new Sprite(120, 2*24, 240, 3*24, new TFT_eSprite(tft), TFT_BLACK, TFT_WHITE, 2);
    spriteLocation_ =       sprites_[1] = new Sprite(0, 3*24, 240, 4*24, new TFT_eSprite(tft), TFT_BLACK, TFT_WHITE, 2);
    spriteLogo_ =           sprites_[2] = new Sprite(70, 0*24, 240, 2*24, new TFT_eSprite(tft), TFT_BLACK, TFT_WHITE, 4);
    spriteStepCount_ =      sprites_[3] = new Sprite(40, 5*24, 240, 7*24, new TFT_eSprite(tft), TFT_BLACK, TFT_WHITE, 4);

    spriteTime_->SetText("Time: ", 0);
    spriteLocation_->SetText("Location: ", 0);
    spriteLogo_->SetText("Lily run", 0);
    spriteStepCount_->SetText("Steps: ", 0);
    spriteWiFi_->SetText("WiFi: ", 0);

    spriteTime_->AddTextPiece("", TFT_WHITE, TFT_BLACK);
    spriteLocation_->AddTextPiece("NO SIGNAL", TFT_RED, TFT_WHITE);
    spriteStepCount_->AddTextPiece("00000", TFT_BLUE, TFT_WHITE);
    spriteWiFi_->AddTextPiece("Disconnected", TFT_GREEN, TFT_BLACK);

    buttonCount_ = 3;
    buttons_ = new Button*[buttonCount_];

    buttons_[0] = new Button(10, 170, 240/3 - 3, 230, new TFT_eSprite(tft), "Start", TFT_WHITE, TFT_BLUE, 4, []() -> bool { return g_App->StartSession(); });
    buttons_[1] = new Button(2*240/3 + 3, 170, 240 - 3, 230, new TFT_eSprite(tft), "End", TFT_WHITE, TFT_BLUE, 4, []() -> bool { return g_App->EndSession(); });
    buttons_[2] = new Button(240/3 + 3, 170, 2*240/3 - 3, 230, new TFT_eSprite(tft), "Pause", TFT_WHITE, TFT_BLUE, 2, []() -> bool { return g_App->PauseSession(); }, 15);

    ClearScreen();
}

App::App() {
}

void App::TouchCallback(uint16_t x, uint16_t y) {
    watch_->VibrateOnce();

    bool result = false;
    for(int i=0; i<buttonCount_ && !result; i++){
        result = buttons_[i]->HandleEvent(x, y);
    }

    // sprintf(buffer, "x=%03d  y=%03d", x, y);
    // spriteLogo_->SetText(buffer, 1);
    // spriteLogo_->Render();
}

void App::ButtonCallback() {
    watch_->VibrateOnce();

    if(watch_->IsScreenOn()) {
        watch_->SetScreen(false);
    }
    else{
        watch_->SetScreen(true);
        ClearScreen();
    }
}

void App::StepCountCallback(uint32_t stepCount) {
    sprintf(buffer, "%05d", stepCount - session.startStepCount);
    spriteStepCount_->SetText(buffer, 1);
    spriteStepCount_->Render();
    session.latestStepCount = stepCount;
    if(!paused_)
        SendStepCount(stepCount - session.startStepCount);
}

void App::GpsCallback(bool isValid, double lat, double lng) {
    if(isValid){
        sprintf(buffer, "lat %.6f long %.6f", lat, lng);
    }
    else{
        sprintf(buffer, "No Signal");
    }
    spriteLocation_->SetText(buffer, 1);
    spriteLocation_->Render();

    if(isValid && !paused_){
        SendGps(lat, lng);
    }
}

void App::WifiStatusCallback(bool isConnected) {
    sprintf(buffer, "%s", (isConnected ? "Connected" : "Disconnected"));
    spriteWiFi_->SetText(buffer, 1);
    spriteWiFi_->Render();
}

void App::TimeUpdatedCallback(GpsTime gpsTime) {
    sprintf(buffer, "%02d:%02d:%02d", hour(), minute(), second());
    spriteTime_->SetText(buffer, 1);
    spriteTime_->Render();
}

void App::ClearScreen() {
    watch_->GetTFT()->fillScreen(TFT_WHITE);
    watch_->GetTFT()->setCursor(0, 0);

    for(int i=0; i<buttonCount_; i++){
        auto b = buttons_[i];
        b->Render();
    }

    for(int i=0; i<spriteCount_; i++){
        auto s = sprites_[i];
        s->Render();
    }

    // sprintf(buffer, "INVALID");
    // spriteLocation_->Render();
}

void App::Update() {
    watch_->Update();    
}

bool App::PauseSession() {
    paused_ = !paused_;
    buttons_[2]->SetText(paused_ ? "Resume" : "Pause");
    buttons_[2]->Render();
    return true;
}
