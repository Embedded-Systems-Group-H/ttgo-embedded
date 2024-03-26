#include "watch_interface.hpp"

static char buffer[128];

WatchInterface& WatchInterface::Get() {
    static WatchInterface watch;
    return watch;
}

void WatchInterface::SetTouchCallback(void(*touchCallback)(uint16_t, uint16_t)){
    touchCallback_ = touchCallback;
}

void WatchInterface::SetButtonCallback(void(*buttonCallback)()){
    buttonCallback_ = buttonCallback;
}

void WatchInterface::SetStepCountCallback(void(*stepCountCallback)(uint32_t)){
    stepCountCallback_ = stepCountCallback;
}

void WatchInterface::SetGpsCallback(void(*gpsCallback)(double, double)){
    gpsCallback_ = gpsCallback;
}

void WatchInterface::SetWifiStatusCallback(void(*wifiStatusCallback)(bool)){
    wifiStatusCallback_ = wifiStatusCallback;
}

void WatchInterface::SetTimeUpdatedCallback(void(*timeUpdatedCallback)(GpsTime)){
    timeUpdatedCallback_ = timeUpdatedCallback;
}

void WatchInterface::Update() {
    while(gnss_->available()) {
        int r = gnss_->read();
        gps_->encode(r);
    }
    
    int16_t x, y;
    bool touched2 = ttgo_->getTouch(x, y);
    touchedJustNow_ = !touched_ && touched2;
    touched_ = touched2;

    if(gps_->location.isUpdated() && gpsCallback_){
        gpsCallback_(gps_->location.lat(), gps_->location.lng());
    }

    bool dateUpdated = gps_->date.isUpdated();
    bool timeUpdated = gps_->date.isUpdated();
    bool datetimeUpdated = dateUpdated || timeUpdated;

    if(dateUpdated){
        gpsTime_.year = gps_->date.year();
        gpsTime_.month = gps_->date.month();
        gpsTime_.day = gps_->date.day();
    }

    if(timeUpdated){
        gpsTime_.hour = gps_->time.hour();
        gpsTime_.minute = gps_->time.minute();
        gpsTime_.second = gps_->time.second();
    }

    if(datetimeUpdated){
        setTime(gpsTime_.hour, gpsTime_.minute, gpsTime_.second, gpsTime_.day, gpsTime_.month, gpsTime_.year);
        adjustTime(UTC_offset * SECS_PER_HOUR);

        if(timeUpdatedCallback_){
            timeUpdatedCallback_(gpsTime_);
        }
    }

    if(touchedJustNow_ && touchCallback_){
        touchCallback_(x, y);
    }

    if(pmu_irq_){
        pmu_irq_ = false;
        ttgo_->power->readIRQ();

        if(ttgo_->power->isPEKShortPressIRQ()) {
            if(buttonCallback_){
                buttonCallback_();
            }
            ttgo_->power->clearIRQ();
        }
    }

    if(step_irq_) {
        step_irq_ = false;
        bool rlst;
        do {
            rlst =  sensor_->readInterrupt();
        } while (!rlst);

        if(sensor_->isStepCounter()) {
            auto count = sensor_->getCounter();
            session_.stepCount = count;
            if(stepCountCallback_){
                stepCountCallback_(count);
            }
        }
    }

    auto isConnected = wireless_.IsConnected();
    if(lastWifiStatus_ != (int)isConnected){
        lastWifiStatus_ = isConnected;

        if(wifiStatusCallback_){
            wifiStatusCallback_(isConnected);
        }
    }    
}

WatchInterface::~WatchInterface() {
}

bool WatchInterface::Quectel_L76X_Probe() {
    bool result = false;
    gnss_->write("$PCAS03,0,0,0,0,0,0,0,0,0,0,,,0,0*02\r\n");
    delay(5);
    // Get version information
    gnss_->write("$PCAS06,0*1B\r\n");
    uint32_t startTimeout = millis() + 500;
    while (millis() < startTimeout) {
        if (gnss_->available()) {
            String ver = gnss_->readStringUntil('\r');
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
    gnss_->write("$PCAS04,5*1C\r\n");
    delay(250);
    // only ask for RMC and GGA
    gnss_->write("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    delay(250);
    // Switch to Vehicle Mode, since SoftRF enables Aviation < 2g
    gnss_->write("$PCAS11,3*1E\r\n");
    return result;
}

WatchInterface::WatchInterface() :
    ttgo_(nullptr), tft_(nullptr), buzzer_(nullptr), drv_(nullptr), sensor_(nullptr), gps_(nullptr), gnss_(nullptr), pmu_irq____(false), step_irq_(false), touched_(false), touchedJustNow_(false), lastWifiStatus_(false), touchCallback_(nullptr), buttonCallback_(nullptr), stepCountCallback_(nullptr), gpsCallback_(nullptr)
{
}

void WatchInterface::InitializeHardwareInterfaces() {
    Serial.begin(115200);
    ttgo_ = TTGOClass::getWatch();
    ttgo_->begin();
    ttgo_->openBL();

    tft_ = ttgo_->tft;
    tft_->setTextFont(2);
    tft_->println("Starting program...");

    drv_ = ttgo_->drv;
    ttgo_->enableDrv2650();

    ttgo_->trunOnGPS();
    ttgo_->gps_begin();
    gps_ =  ttgo_->gps;
    gnss_ = ttgo_->hwSerial;

    if(!Quectel_L76X_Probe()) {
        tft_->setCursor(0, 0);
        tft_->setTextColor(TFT_RED);
        tft_->println("GNSS Probe failed!");
        while (1);
    }


    
    pinMode(AXP202_INT, INPUT);
    attachInterrupt(AXP202_INT, [&pmu_irq_]{
        pmu_irq_ = true;
    }, FALLING);

    ttgo_->power->adc1Enable(AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1 |
                            AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1,
                            true);
    ttgo_->power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ |
                            AXP202_BATT_REMOVED_IRQ | AXP202_BATT_CONNECT_IRQ |
                            AXP202_PEK_LONGPRESS_IRQ | AXP202_PEK_SHORTPRESS_IRQ,
                            true);
    ttgo_->power->clearIRQ();
    
    sensor_ = ttgo_->bma;

    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;
    sensor_->accelConfig(cfg);
    sensor_->enableAccel();
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [&step_irq_] {
        step_irq_ = 1;
    }, RISING);
    sensor_->enableFeature(BMA423_STEP_CNTR, true);
    sensor_->resetStepCounter();
    sensor_->enableStepCountInterrupt();

    wireless_.Enable();    
}

void WatchInterface::Init() {
    InitializeHardwareInterfaces();    
}

Wireless* WatchInterface::GetWirelessInterface(){
    return &wireless_;
}

void WatchInterface::VibrateOnce() {
    drv_->setWaveform(0, 75);
    drv_->setWaveform(1, 0);
    drv_->go();    
}

bool WatchInterface::IsScreenOn() const {
    return ttgo_->bl->isOn();    
}

void WatchInterface::SetScreen(bool turnOn) {
    if(turnOn){
        ttgo_->openBL();    
    }
    else{
        ttgo_->closeBL();
    }
}

TFT_eSPI* WatchInterface::GetTFT() {
    return tft_;    
}
