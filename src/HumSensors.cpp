/**
Xiaomi humidity/temperature BLE sensors data pulling class*/

#include "Arduino.h"
#include "HumSensors.h"

std::unique_ptr<BLEClientExt> BLEClientExt::pInst;
BLEScan *HumSensors::pBLEScan;
std::vector<std::string> HumSensors::addresses;
std::map<std::string, SensorData> HumSensors::sensorsData;

// The remote service we wish to connect to.
//BLEUUID BLEClientExt::serviceUUID("ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6");
// The characteristic of the remote service we are interested in.
//BLEUUID BLEClientExt::charUUID("ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6");

#define HUM_SENSOR_NAME "LYWSD03MMC"

void HumSensors::init()
{
  NimBLEDevice::init("ESP32");

  pBLEScan = NimBLEDevice::getScan(); //create new scan
  pBLEScan->setActiveScan(true);   //active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);
}

void HumSensors::refreshData()
{
  NimBLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
  int count = foundDevices.getCount();
  Serial.printf("+ Found device count : %d\n", count);
  for (int i = 0; i < count; i++)
  {
    NimBLEAdvertisedDevice b = foundDevices.getDevice(i);
    //Serial.println(b.toString().c_str());
    if (b.getName() == HUM_SENSOR_NAME)
    {
      NimBLEAddress addr = b.getAddress();
      Serial.println(b.toString().c_str());      
      if (std::find(addresses.begin(), addresses.end(), addr.toString()) == addresses.end())
      {
        addresses.push_back(addr.toString());
      }
      std::string strData = b.getServiceData();
      Serial.printf("Data: %s | ",strData.c_str());
      for(int i=0; i<strData.size();i++) {
        Serial.printf("%X ",strData.data()[i]);
      }
      Serial.println("");
    }
  }

  for (auto it = addresses.begin(); it != addresses.end(); it++)
  {
    std::string curAddr = *it;
    int j = 0;
    for (; j < count; j++)
    {
      if (foundDevices.getDevice(j).getAddress().equals(NimBLEAddress(curAddr)))
      {
        break;
      }
    }
    if (j == count)
    {
      Serial.printf("* Remove offline address : %s\n", curAddr.c_str());
      it = addresses.erase(it);
    }
    Serial.printf("+ Connect : %s\n", curAddr.c_str());
    BLEClientExt::getInstance().connectAndRegisterNotifications(BLEAddress(curAddr));
    int retries = 1000;
    while (BLEClientExt::getInstance().isConnected() && retries)
    {
      delay(10);
      retries--;
    };

    BLEClientExt::getInstance().disconnect();

    SensorData data = BLEClientExt::getInstance().getData();
    data.timestamp = std::chrono::system_clock::now();
    sensorsData[*it] = data;
  }

  Serial.println("********* Results ************");
  for (const auto &d : sensorsData)
  {
    Serial.printf("    temp = %.1f C ; humidity = %.1f %% ; voltage = %.3f V\n", d.second.temp, d.second.humi, d.second.voltage);
  }
  Serial.println("");
}

/* Task to be created. */
void vTaskCode(void *pvParameters)
{
  Serial.println("*************** Task started *******************");
  HumSensors::init();
  for (;;)
  {
    HumSensors::refreshData();
    delay(5000);
  }
}

/* Function that creates a task. */
void startBLETask(void)
{
  BaseType_t xReturned;
  TaskHandle_t xHandle = NULL;

  /* Create the task, storing the handle. */
  xReturned = xTaskCreate(
      vTaskCode,        /* Function that implements the task. */
      "BLE",            /* Text name for the task. */
      20000,            /* Stack size in words, not bytes. */
      (void *)1,        /* Parameter passed into the task. */
      tskIDLE_PRIORITY, /* Priority at which the task is created. */
      &xHandle);        /* Used to pass out the created task's handle. */

  //if( xReturned == pdPASS )
  //{
  /* The task was created.  Use the task's handle to delete the task. */
  //vTaskDelete( xHandle );
  //}
}