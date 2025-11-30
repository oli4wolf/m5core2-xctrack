#include "ble_uart.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "config.h"
#include <M5Unified.h>
#include <esp_log.h>
// External variables from main.cpp
extern float globalAltitude_m;
extern float globalVerticalSpeed_mps;
extern SemaphoreHandle_t xVariometerMutex;
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

//https://github.com/har-in-air/ESP32C3_BLUETOOTH_AUDIO_VARIO/blob/479e52d7708047b11a5285bc00b2d51094e3c3b5/src/ble_uart.cpp

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer* pBLEServer                = NULL;
BLEService* pService                 = NULL;
BLECharacteristic* pTxCharacteristic = NULL;
BLECharacteristic* pRxCharacteristic = NULL;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        ESP_LOGD("ble_uart.cpp", "BLE client connected");
    }
    void onDisconnect(BLEServer* pServer) {
        ESP_LOGD("ble_uart.cpp", "BLE client disconnected");
        pServer->getAdvertising()->start(); // Restart advertising
    }
};

static uint8_t ble_uart_nmea_checksum(const char *szNMEA);

void ble_task(void *pvParameter)
{
  ble_uart_init();
  ESP_LOGD("main.cpp","Bluetooth LE LK8EX1 messages @ 10Hz");
  while (1)
  {
    int32_t altitudeM = 0;
    int32_t climbrateCps = 0;
    float batVoltage = M5.Power.getBatteryVoltage();
    ble_uart_transmit_LK8EX1(altitudeM, climbrateCps, batVoltage);
    ESP_LOGD("main.cpp","Transmitted LK8EX1 message: Altitude=%d m, ClimbRate=%d cm/s, Battery=%.2f V", altitudeM, climbrateCps, batVoltage);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void ble_uart_init() {
	ESP_LOGD("ble_uart.cpp", "Starting BLE initialization");
	BLEDevice::init("M5Core2-Vario");
	BLEDevice::setMTU(46);
	BLEDevice::setPower(ESP_PWR_LVL_N0); // 0dB Device ist gleich nebendran.

	pBLEServer = BLEDevice::createServer();
	pBLEServer->setCallbacks(new MyServerCallbacks());

	pService          = pBLEServer->createService(SERVICE_UUID);
	
	pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
	pRxCharacteristic = pService->createCharacteristic(
		CHARACTERISTIC_UUID_RX,
		BLECharacteristic::PROPERTY_WRITE);
	pTxCharacteristic->addDescriptor(new BLE2902());

	pService->start();
	pBLEServer->getAdvertising()->start();
	ESP_LOGD("ble_uart.cpp", "BLE advertising started");
	}


static uint8_t ble_uart_nmea_checksum(const char *szNMEA){
	const char* sz = &szNMEA[1]; // skip leading '$'
	uint8_t cksum = 0;
	while ((*sz) != 0 && (*sz != '*')) {
		cksum ^= (uint8_t) *sz;
		sz++;
		}
	return cksum;
	}

// $LK8EX1,<pressure Pa>,<altitude m>,<vario cm/s>,<temperature C>,<battery V>*<checksum>
void ble_uart_transmit_LK8EX1(int32_t altm, int32_t cps, float batVoltage) {
	char szmsg[40];
	sprintf(szmsg, "$LK8EX1,999999,%d,%d,99,%.1f*", altm, cps, batVoltage);
	uint8_t cksum = ble_uart_nmea_checksum(szmsg);
	char szcksum[5];
	sprintf(szcksum,"%02X\r\n", cksum);
	strcat(szmsg, szcksum);
#ifdef BLE_DEBUG	
    dbg_printf(("%s", szmsg)); 
#endif
	pTxCharacteristic->setValue((const uint8_t*)szmsg, strlen(szmsg));
	pTxCharacteristic->notify();   
	}