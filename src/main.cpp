#include <esp_system.h> // For PRO_CPU_NUM and APP_CPU_NUM
#include <M5Unified.h> // Make the M5Unified library available to your program.
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h> // Required for mutex

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include "sensor_task.h"     // Include the new sensor task header
#include "gps_task.h"        // Include the new GPS task header
#include "variometer_task.h" // Include the new variometer task header
#include "config.h"         // Include configuration constants

// BLE Server example
// https://github.com/naoki-sawada/m5stack-ble/blob/master/m5stack-ble/m5stack-ble.ino

M5GFX lcd;

// Variometer global variables
extern float globalAltitude_m;
extern float globalVerticalSpeed_mps;
extern SemaphoreHandle_t xVariometerMutex;
SemaphoreHandle_t xSensorMutex;

// GPS global variables
double globalLatitude = 46.947597;
double globalLongitude = 7.440434;
double globalAltitude = 542.5; // Initial altitude set to Bern, Switzerland
bool globalTestdata = false; // Flag to indicate if test data is being used
bool globalValid = false; // Indicates if a valid GPS fix is available
double globalDirection;
double globalSpeed; // Added for GPS speed in km/h
uint32_t globalTime;
SemaphoreHandle_t xGPSMutex;

// Task Stack Sizes
extern const int SENSOR_TASK_STACK_SIZE;
extern const int GPS_TASK_STACK_SIZE;
extern const int VARIOMETER_TASK_STACK_SIZE;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      M5.Lcd.println("connect");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      M5.Lcd.println("disconnect");
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    M5.Lcd.println("read");
    pCharacteristic->setValue("Hello World!");
  }
  
  void onWrite(BLECharacteristic *pCharacteristic) {
    M5.Lcd.println("write");
    std::string value = pCharacteristic->getValue();
    M5.Lcd.println(value.c_str());
  }
};

void initializeBLE(){
  BLEDevice::init("m5-stack");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void initializeM5Stack()
{
// M5Stack Core2 Initialization
// GPS, Sound, Bluetooth, No Wifi, No SD at the moment.

    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    cfg.internal_imu = true;  // default=true. use internal IMU.
    cfg.internal_rtc = true;  // default=true. use internal RTC.
    cfg.internal_spk = true;  // default=true. use internal speaker.
    cfg.internal_mic = false; // default=true. use internal microphone.
    cfg.external_imu = false; // default=false. use Unit Accel & Gyro.
    cfg.external_rtc = false; // default=false. use Unit RTC.

    M5.begin(cfg);
    lcd.init();
    M5.In_I2C.release();
}

void startupScreen()
{
    lcd.fillScreen(TFT_BLACK);
    lcd.setCursor(0, 0);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextSize(1);
    lcd.println("M5Stack Core2 XCTrack");
    lcd.println("v0.1");
    lcd.println("by @oli4wolf on github");
    lcd.println("This is a non-commercial project.");
    delay(5000);
}

void setup() {
  // put your setup code here, to run once:
  initializeM5Stack();
  startupScreen();
  // TASK Initialisation
  initSensorTask();     // Initialize the sensor task components
  initGPSTask();        // Initialize the GPS task components
  initVariometerTask(); // Initialize the variometer task components

  xSensorMutex = xSemaphoreCreateMutex();     // Initialize the sensor mutex
  xGPSMutex = xSemaphoreCreateMutex();        // Initialize the GPS mutex

  initializeBLE();

}

void loop() {
   // draw the sensor information (later transform it into a simple vario)
    // Commands
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    // generate random number between 0 and 100
    int randomValue = random(0, 100);
    M5.Lcd.println("Button B pressed!"+String(randomValue));
    pCharacteristic->setValue(("Button B pressed!"+String(randomValue)).c_str());
    pCharacteristic->notify();
}
