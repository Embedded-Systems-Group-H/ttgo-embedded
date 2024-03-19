#include <HTTPClient.h>
#include <WiFi.h>
#define LILYGO_WATCH_2020_V2
#include <LilyGoWatch.h>

static TTGOClass* ttgo = nullptr;
static TFT_eSPI* tft = nullptr;
static Buzzer* buzzer = nullptr;
static TinyGPSPlus* gps = nullptr;
static Adafruit_DRV2605* drv = nullptr;
HardwareSerial *GNSS = NULL;

TFT_eSprite *sprite_Time = nullptr;
TFT_eSprite *sprite_Location = nullptr;
TFT_eSprite *sprite_Coordinate = nullptr;

void pressed(){
    drv->setWaveform(0, 75);
    drv->setWaveform(1, 0);
    drv->go();
}

void released(){
    drv->setWaveform(0, 75);
    drv->setWaveform(1, 0);
    drv->go();
}

void clearScreen(){
    tft->fillScreen(TFT_BLACK);
    tft->setCursor(0, 0);
    tft->println("Testing program");
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
    tft->println("Play Audio!");
    // eSpTime->setTextColor(TFT_GREEN, TFT_BLACK);

    sprite_Time = new TFT_eSprite(tft);
    sprite_Location = new TFT_eSprite(tft);
    sprite_Coordinate = new TFT_eSprite(tft);

    sprite_Time->createSprite(240, 48);
    sprite_Time->setTextFont(2);
    sprite_Location->createSprite(240, 48);
    sprite_Location->setTextFont(2);
    sprite_Coordinate->createSprite(240, 48);
    sprite_Coordinate->setTextFont(2);

    ttgo->button->setPressedHandler(pressed);
    ttgo->button->setReleasedHandler(released);

    clearScreen();
}

char buf[128];
char gpsBuf[128];
char gpsTimeBuf[128];

void loop(){
    while(GNSS->available()) {
        int r = GNSS->read();
        Serial.write(r);
        gps->encode(r);
    }
    
    bool doClearScreen = false;
    int16_t x, y;
    doClearScreen = doClearScreen || (bool)ttgo->getTouch(x, y);
    doClearScreen = doClearScreen || (bool)gps->location.isUpdated();

    doClearScreen = false;
    if(doClearScreen){
        clearScreen();
    }

    if (gps->location.isUpdated()) {
        sprintf(gpsBuf, "Location: lat %.6f long %.6f", gps->location.lat(), gps->location.lng());
        
        sprite_Location->fillSprite(TFT_BLACK);
        sprite_Location->setTextColor(TFT_GREEN);
        sprite_Location->setCursor(0, 0);
        sprite_Location->print(gpsBuf);
        sprite_Location->pushSprite(0, 0);
        // ttgo->tft->drawString(gpsBuf, 10, 80);
    }

    if (gps->time.isUpdated()) {
        sprintf(gpsTimeBuf, "Time: %02d:%02d:%02d", gps->time.hour(), gps->time.minute(), gps->time.second());

        sprite_Time->fillSprite(TFT_BLACK);
        sprite_Time->setTextColor(TFT_RED);
        sprite_Time->setCursor(0, 0);
        sprite_Time->print(gpsTimeBuf);
        sprite_Time->pushSprite(0, 43);
        // ttgo->tft->drawString(gpsTimeBuf, 10, 40);
    }

    if (ttgo->getTouch(x, y)) {
        sprintf(buf, "x=%03d  y=%03d", x, y);

        sprite_Coordinate->fillSprite(TFT_BLACK);
        sprite_Coordinate->setTextColor(TFT_YELLOW);
        sprite_Coordinate->setCursor(0, 0);
        sprite_Coordinate->print(buf);
        sprite_Coordinate->pushSprite(240/3, 240 - 43);

        // ttgo->tft->drawString(buf, x, y);
        drv->setWaveform(0, 75);
        drv->setWaveform(1, 0);
        drv->go();
    }
    // delay(5);
}
