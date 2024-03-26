#include "lilygo.hpp"
#include "config.hpp"

#include "button.hpp"
#include "sprite.hpp"
#include "wireless.hpp"
#include <TimeLib.h>

static TTGOClass* ttgo = nullptr;
static TFT_eSPI* tft = nullptr;
static Buzzer* buzzer = nullptr;
static TinyGPSPlus* gps = nullptr;
static Adafruit_DRV2605* drv = nullptr;
static BMA* sensor = nullptr;
static HardwareSerial* GNSS = nullptr;

Sprite* sprite_Time = nullptr;
Sprite* sprite_Location = nullptr;
Sprite* sprite_Coordinate = nullptr;
Sprite* sprite_StepCount = nullptr;
Sprite* sprite_WiFi = nullptr;

static constexpr uint8_t spriteCount = 5;
static Sprite* g_Sprites[spriteCount];

static bool pmu_irq = false;
static bool step_irq = false;
static bool doClearScreen = false;
static bool touched = false;
static bool touchedJustNow = false;
static char buf[128];
static char gpsBuf[128];
static char gpsTimeBuf[128];
static char stepBuf[128];
static char wifiBuf[128];

const int UTC_offset = 2;

struct{
    int stepCount = 0;
} session;

time_t unixTimestamp() {
    return now();
}


static String sessionId;
static String base_url = String("http://") + String(SERVER_IP) + String(":") + String(SERVER_PORT) + String("/api/");
static Wireless wireless;

bool start_session(){
    sessionId = String(unixTimestamp());
    String url = base_url + String("session_start/") + sessionId;
    return wireless.HttpPost(url.c_str());
}

bool end_session(){
    String url = base_url + String("session_end/") + sessionId;
    sessionId = "";
    return wireless.HttpPost(url.c_str());
}

bool send_gps(){
    if(sessionId.length() == 0)
        return false;
    String payload = String("ts=") + unixTimestamp();
    String latitude = String("lat=") + String(gps->location.lat());
    String longitude = String("long=") + String(gps->location.lng());
    payload = payload + "&" + latitude + "&" + longitude;
    String url = base_url + String("gps/") + sessionId;
    url += String("?") + payload;
    return wireless.HttpPost(url.c_str());
}

bool send_step_count(){
    if(sessionId.length() == 0)
        return false;
    String payload = String("ts=") + unixTimestamp();
    String stepCount = String("count=") + String(session.stepCount);
    payload = payload + "&" + stepCount;
    String url = base_url + String("step_count/") + sessionId;
    url += String("?") + payload;
    return wireless.HttpPost(url.c_str());
}

static const int buttonCount = 2;
static Button* buttons[2] = {nullptr, nullptr};
static int lastWifiStatus = 999;

void clearScreen(){
    tft->fillScreen(TFT_BLACK);
    tft->setCursor(0, 0);

    for(int i=0; i<buttonCount; i++){
        auto b = buttons[i];
        b->Render();
    }

    sprintf(gpsBuf, "Location: INVALID");
    // sprite_Location->fillSprite(TFT_BLACK);
    // sprite_Location->setTextColor(TFT_GREEN);
    // sprite_Location->setCursor(0, 0);
    // sprite_Location->print(gpsBuf);
    // sprite_Location->pushSprite(0, 43 + 25);
    // sprite_Location->SetText(gpsBuf);
    sprite_Location->Render(gpsBuf);
}

bool handleEvent(uint16_t x, uint16_t y){
    bool result = false;
    for(int i=0; i<buttonCount && !result; i++){
        result |= buttons[i]->HandleEvent(x, y);
    }
    return result;
}

bool Quectel_L76X_Probe(){
    bool result = false;
    GNSS->write("$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n");
    delay(5);
    // Get version information
    GNSS->write("$PCAS06,0*1B\r\n");
    uint32_t startTimeout = millis() + 500;
    while (millis() < startTimeout) {
        if (GNSS->available()) {
            String ver = GNSS->readStringUntil('\r');
            // Get module info , If the correct header is returned,
            // it can be determined that it is the MTK chip
            int index = ver.indexOf("$");
            if (index != -1) {
                ver = ver.substring(index);
                if (ver.startsWith("$GPTXT,01,01,02")) {
                    Serial.println("L76K GNSS init succeeded, using L76K GNSS Module");
                    result = true;
                }
            }
        }
    }
    // Initialize the L76K Chip, use GPS + GLONASS
    GNSS->write("$PCAS04,5*1C\r\n");
    delay(250);
    // only ask for RMC and GGA
    GNSS->write("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    delay(250);
    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    GNSS->write("$PCAS11,3*1E\r\n");
    return result;
}

void setup(){
    Serial.begin(115200);
    ttgo = TTGOClass::getWatch();
    ttgo->begin();
    ttgo->openBL();

    tft = ttgo->tft;
    tft->setTextFont(2);
    tft->println("Starting program...");

    drv = ttgo->drv;
    ttgo->enableDrv2650();

    ttgo->trunOnGPS();
    ttgo->gps_begin();
    gps =  ttgo->gps;
    GNSS = ttgo->hwSerial;

    if(!Quectel_L76X_Probe()) {
        tft->setCursor(0, 0);
        tft->setTextColor(TFT_RED);
        tft->println("GNSS Probe failed!");
        while (1);
    }

    tft->fillScreen(TFT_RED);
    tft->setCursor(0, 0);

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

    pinMode(AXP202_INT, INPUT);
    attachInterrupt(AXP202_INT, []{
        pmu_irq = true;
    }, FALLING);

    ttgo->power->adc1Enable(AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1 |
                            AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1,
                            true);
    ttgo->power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ |
                            AXP202_BATT_REMOVED_IRQ | AXP202_BATT_CONNECT_IRQ |
                            AXP202_PEK_LONGPRESS_IRQ | AXP202_PEK_SHORTPRESS_IRQ,
                            true);
    ttgo->power->clearIRQ();
    
    sensor = ttgo->bma;

    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    sensor->accelConfig(cfg);
    sensor->enableAccel();
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] {
        step_irq = 1;
    }, RISING);
    sensor->enableFeature(BMA423_STEP_CNTR, true);
    sensor->resetStepCounter();
    sensor->enableStepCountInterrupt();

    wireless.Enable();
    clearScreen();

    delay(2000);
}

struct{
  int year=2000, month=1, day=1, hour=0, minute=0, second=0;
} gpsTime;

void loop(){
    while(GNSS->available()) {
        int r = GNSS->read();
        gps->encode(r);
    }
    
    int16_t x, y;
    bool touched2 = ttgo->getTouch(x, y);
    touchedJustNow = !touched && touched2;
    touched = touched2;

    if(doClearScreen){
        clearScreen();
        doClearScreen = false;
    }

    if (gps->location.isUpdated()) {
        sprintf(gpsBuf, "Location: lat %.6f long %.6f", gps->location.lat(), gps->location.lng());
        
        sprite_Location->Render(gpsBuf);
        // sprite_Location->pushSprite(0, 43 + 25);
        send_gps();
    }

    bool dateUpdated = gps->date.isUpdated();
    bool timeUpdated = gps->date.isUpdated();
    bool datetimeUpdated = dateUpdated || timeUpdated;

    if(dateUpdated){
        gpsTime.year = gps->date.year();
        gpsTime.month = gps->date.month();
        gpsTime.day = gps->date.day();
    }

    if(timeUpdated){
        gpsTime.hour = gps->time.hour();
        gpsTime.minute = gps->time.minute();
        gpsTime.second = gps->time.second();
    }

    if(datetimeUpdated){
        setTime(gpsTime.hour, gpsTime.minute, gpsTime.second, gpsTime.day, gpsTime.month, gpsTime.year);
        adjustTime(UTC_offset * SECS_PER_HOUR);

        sprintf(gpsTimeBuf, "Time: %02d:%02d:%02d", hour(), minute(), second());

        // sprite_Time->fillSprite(TFT_BLACK);
        // sprite_Time->setTextColor(TFT_RED);
        // sprite_Time->setCursor(0, 0);
        // sprite_Time->print(gpsTimeBuf);
        // sprite_Time->pushSprite(0, 43);
        sprite_Time->Render(gpsTimeBuf);
    }

    if (touched) {
        sprintf(buf, "x=%03d  y=%03d", x, y);

        // sprite_Coordinate->fillSprite(TFT_BLACK);
        // sprite_Coordinate->setTextColor(TFT_YELLOW);
        // sprite_Coordinate->setCursor(0, 0);
        // sprite_Coordinate->print(buf);
        // sprite_Coordinate->pushSprite(240/3, 240 - 43);
        sprite_Coordinate->Render(buf);
    }

    if(touchedJustNow){
        handleEvent(x, y);

        drv->setWaveform(0, 75);
        drv->setWaveform(1, 0);
        drv->go();
    }

    if(pmu_irq){
      pmu_irq = false;
      ttgo->power->readIRQ();
      if (ttgo->power->isPEKShortPressIRQ()) {
          drv->setWaveform(0, 75);
          drv->setWaveform(1, 0);
          drv->go();

          if (ttgo->bl->isOn()) {
              ttgo->closeBL();
          } else {
              ttgo->openBL();
              doClearScreen = true;
          }
          ttgo->power->clearIRQ();
      }
    }

    if(step_irq) {
        step_irq = false;
        bool rlst;
        do {
            rlst =  sensor->readInterrupt();
        } while (!rlst);

        if (sensor->isStepCounter()) {
            session.stepCount = sensor->getCounter();
            sprintf(stepBuf, "Step Count: %05d", session.stepCount);
            // sprite_StepCount->fillSprite(TFT_BLACK);
            // sprite_StepCount->setTextColor(TFT_WHITE);
            // sprite_StepCount->setCursor(0, 0);
            // sprite_StepCount->print(stepBuf);
            // // sprite_StepCount->pushSprite(240/2, 0);
            // sprite_StepCount->pushSprite(0, 0);
            sprite_StepCount->Render(stepBuf);
        }

        send_step_count();
    }

    auto isConnected = wireless.IsConnected();
    if(lastWifiStatus != (int)isConnected){
        lastWifiStatus = isConnected;

        sprintf(wifiBuf, "Wifi status");
        // sprite_WiFi->fillSprite(TFT_BLACK);
        // sprite_WiFi->setTextColor(isConnected ? TFT_GREEN : TFT_RED);
        // sprite_WiFi->setCursor(0, 0);
        // sprite_WiFi->print(wifiBuf);
        // sprite_WiFi->pushSprite(240/2, 43);
        sprite_WiFi->Render(wifiBuf);
    }

    // delay(5);
}
