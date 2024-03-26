#include "config.hpp"
#include "button.hpp"
#include "sprite.hpp"
#include "watch_interface.hpp"

static constexpr uint8_t spriteCount = 5;
static Sprite* g_Sprites[spriteCount];

Sprite* sprite_Time = nullptr;
Sprite* sprite_Location = nullptr;
Sprite* sprite_Coordinate = nullptr;
Sprite* sprite_StepCount = nullptr;
Sprite* sprite_WiFi = nullptr;

static const int buttonCount = 2;
static Button* buttons[2] = {nullptr, nullptr};

static char buffer[128];

static String base_url = String("http://") + String(SERVER_IP) + String(":") + String(SERVER_PORT) + String("/api/");

struct{
    String sessionId;
    int stepCount = 0;
} session;

WatchInterface* watch = nullptr;

bool start_session(){
    session.sessionId = String(now());
    String url = base_url + String("session_start/") + session.sessionId;
    return watch->GetWirelessInterface()->HttpPost(url.c_str());
}

bool end_session(){
    String url = base_url + String("session_end/") + session.sessionId;
    session.sessionId = "";
    return watch->GetWirelessInterface()->HttpPost(url.c_str());
}

bool send_gps(double lat, double lng){
    if(session.sessionId.length() == 0)
        return false;
    String payload = String("ts=") + now();
    String latitude = String("lat=") + String(lat);
    String longitude = String("long=") + String(lng);
    payload = payload + "&" + latitude + "&" + longitude;
    String url = base_url + String("gps/") + session.sessionId;
    url += String("?") + payload;
    return watch->GetWirelessInterface()->HttpPost(url.c_str());
}

bool send_step_count(){
    if(session.sessionId.length() == 0)
        return false;
    String payload = String("ts=") + now();
    String stepCount = String("count=") + String(session.stepCount);
    payload = payload + "&" + stepCount;
    String url = base_url + String("step_count/") + session.sessionId;
    url += String("?") + payload;
    return watch->GetWirelessInterface()->HttpPost(url.c_str());
}

void clearScreen(){
    watch->GetTFT()->fillScreen(TFT_BLACK);
    watch->GetTFT()->setCursor(0, 0);

    for(int i=0; i<buttonCount; i++){
        auto b = buttons[i];
        b->Render();
    }

    sprintf(buffer, "Location: INVALID");
    sprite_Location->Render(buffer);
}

void handleTouch(uint16_t x, uint16_t y){
    bool result = false;
    for(int i=0; i<buttonCount && !result; i++){
        result = buttons[i]->HandleEvent(x, y);
    }

    sprintf(buffer, "x=%03d  y=%03d", x, y);
    sprite_Coordinate->Render(buffer);
    
    watch->VibrateOnce();
}

void buttonCallback(){
    watch->VibrateOnce();

    if(watch->IsScreenOn()) {
        watch->SetScreen(false);
    }
    else{
        watch->SetScreen(true);
        clearScreen();
    }
}

void stepCountCallback(uint32_t stepCount){
    sprintf(buffer, "Step Count: %05d", stepCount);
    sprite_StepCount->Render(buffer);
    send_step_count();
}

void gpsCallback(double lat, double lng){
    sprintf(buffer, "Location: lat %.6f long %.6f", lat, lng);
    sprite_Location->Render(buffer);
    send_gps(lat, lng);
}

void wifiStatusCallback(bool isConnected){
    sprintf(buffer, "Wifi status");
    sprite_WiFi->Render(buffer);
}

void timeUpdateCallback(GpsTime gpsTime){
    sprintf(buffer, "Time: %02d:%02d:%02d", hour(), minute(), second());
    sprite_Time->Render(buffer);
}

void setup(){
    watch = &WatchInterface::Get();
    watch->Init();
    watch->SetTouchCallback(handleTouch);
    watch->SetButtonCallback(buttonCallback);
    watch->SetStepCountCallback(stepCountCallback);
    watch->SetGpsCallback(gpsCallback);

    auto tft = watch->GetTFT();
    g_Sprites[0] = new Sprite(0, 0*24, 240, 1*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);
    g_Sprites[1] = new Sprite(0, 1*24, 240, 2*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);
    g_Sprites[2] = new Sprite(0, 2*24, 240, 3*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);
    g_Sprites[3] = new Sprite(0, 3*24, 240, 4*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);
    g_Sprites[4] = new Sprite(0, 4*24, 240, 5*24, new TFT_eSprite(tft), TFT_WHITE, TFT_BLACK, 2);

    sprite_Time = g_Sprites[0];
    sprite_Location = g_Sprites[1];
    sprite_Coordinate = g_Sprites[2];
    sprite_StepCount = g_Sprites[3];
    sprite_WiFi = g_Sprites[4];

    buttons[0] = new Button(10, 120, 240/3 - 3, 180, new TFT_eSprite(tft), "Start", TFT_WHITE, TFT_BLUE, start_session);
    buttons[1] = new Button(240/3 + 3, 120, 2*240/3 - 3, 180, new TFT_eSprite(tft), "End", TFT_WHITE, TFT_BLUE, end_session);

    clearScreen();

    delay(2000);
}

void loop(){
    watch->Update();

    // delay(5);
}
