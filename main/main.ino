#include <HTTPClient.h>
#include <WiFi.h>
#define LILYGO_WATCH_2020_V2
#include <LilyGoWatch.h>

static TTGOClass* ttgo = nullptr;
static TFT_eSPI* tft = nullptr;
static Buzzer* buzzer = nullptr;
static TinyGPSPlus* gps = nullptr;
static Adafruit_DRV2605* drv = nullptr;

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
    
    ttgo->trunOnGPS();
    ttgo->gps_begin();
    gps =  ttgo->gps;

    drv = ttgo->drv;
    ttgo->enableDrv2650();

    tft->fillScreen(TFT_RED);
    tft->setCursor(0, 0);
    tft->println("Play Audio!");

    clearScreen();
}

char buf[128];

void loop(){
    int16_t x, y;
    if (ttgo->getTouch(x, y)) {
        sprintf(buf, "x:%03d  y:%03d", x, y);
        clearScreen();
        ttgo->tft->drawString(buf, x, y);
        drv->setWaveform(0, 75);  // play effect
        drv->setWaveform(1, 0);       // end waveform
        drv->go();
        delay(50);
    }
    delay(100);
}
