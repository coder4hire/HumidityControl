/**
Xiaomi humidity/temperature BLE sensors data pulling class*/

#include "Arduino.h"
#include "HumSensors.h"

std::unique_ptr<BLEClientExt> BLEClientExt::pInst;
BLEScan *HumSensors::pBLEScan;
std::map<NimBLEAddress, std::unique_ptr<BLEClientExt>> HumSensors::clients;

// The remote service we wish to connect to.
// BLEUUID BLEClientExt::serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
// The characteristic of the remote service we are interested in.
// BLEUUID BLEClientExt::charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");

#define HUM_SENSOR_NAME "LYWSD03MMC"

void HumSensors::init()
{
  NimBLEDevice::init("ESP32");

  pBLEScan = NimBLEDevice::getScan(); // create new scan
  pBLEScan->setActiveScan(true);      // active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);
}

void HumSensors::refreshClientsList()
{
  NimBLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
  int count = foundDevices.getCount();
  Serial.printf("+ Found device count : %d\n", count);
  for (int i = 0; i < count; i++)
  {
    NimBLEAdvertisedDevice b = foundDevices.getDevice(i);
    if (b.getName() == HUM_SENSOR_NAME)
    {
      NimBLEAddress addr = b.getAddress();
      if (!clients.count(addr))
      {
        Serial.printf("+ Addr : %s\n", addr.toString().c_str());
        clients[addr] = std::unique_ptr<BLEClientExt>(new BLEClientExt(BLEAddress(addr)));
        Serial.printf("+ client is added \n");
      }
    }
  }

  for (auto it = clients.begin(); it != clients.end(); it++)
  {
    std::string curAddr = it->first;
    if (!std::any_of(foundDevices.begin(), foundDevices.end(), [&curAddr](NimBLEAdvertisedDevice *dev)
                     { return dev->getAddress().equals(curAddr); }))
    {
      Serial.printf("* Remove offline address : %s\n", curAddr.c_str());
      it = clients.erase(it);
    }
  }
}

void HumSensors::refreshData()
{
  for (const auto &pair : clients)
  {
    for (int retries = 5;
      retries > 0 && !pair.second->connectAndRegisterNotifications();
      retries--);
  }

  delay(10000);

  for (const auto &pair : clients)
  {
    pair.second->disconnect();
  }

  // Serial.println("********* Results ************");
  // for (const auto &d : sensorsData)
  // {
  //   Serial.printf("    temp = %.1f C ; humidity = %.1f %% ; voltage = %.3f V\n", d.second.temp, d.second.humi, d.second.voltage);
  // }
  // Serial.println("");
}

/* Task to be created. */
void vTaskCode(void *pvParameters)
{
  Serial.println("*************** Task started *******************");
  HumSensors::init();
  HumSensors::refreshClientsList();
  for (;;)
  {
    HumSensors::refreshData();
    delay(5000);
  }
}

/* Function that creates a task. */
void startBLETask(void)
{
  TaskHandle_t xHandle = NULL;

  /* Create the task, storing the handle. */
  xTaskCreate(
      vTaskCode,        /* Function that implements the task. */
      "BLE",            /* Text name for the task. */
      20000,            /* Stack size in words, not bytes. */
      (void *)1,        /* Parameter passed into the task. */
      tskIDLE_PRIORITY, /* Priority at which the task is created. */
      &xHandle);        /* Used to pass out the created task's handle. */

  // if( xReturned == pdPASS )
  //{
  /* The task was created.  Use the task's handle to delete the task. */
  // vTaskDelete( xHandle );
  // }
}