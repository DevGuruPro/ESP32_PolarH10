#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// The remote service we wish to connect to.
static BLEUUID hr_serviceUUID(BLEUUID((uint16_t)0x180D)); // Heart Rate Service
// The characteristic of the remote service we are interested in.
static BLEUUID    hr_charUUID(BLEUUID((uint16_t)0x2A37));

static BLEUUID accelero_serviceUUID(BLEUUID((uint16_t)0x0fb005c81));
static BLEUUID    accelero_charUUID(BLEUUID((uint16_t)0x0fb005c82));

static boolean doConnect = false;  
static boolean connected = false;  
static boolean doScan = false;

static BLERemoteCharacteristic *hr_pRemoteCharacteristic, *accelero_pRemoteCharacteristic;  
static BLEAdvertisedDevice* myDevice;

class MyClientCallback : public BLEClientCallbacks {  
    void onConnect(BLEClient* pclient) {  
      connected = true;  
    }  

    void onDisconnect(BLEClient* pclient) {  
      connected = false;  
      Serial.println("Disconnected");
    }  
};

static void hr_notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify) {
  Serial.print("Heart Rate: ");  
  if (length > 1) {  
    // Heart rate is usually in the second byte, this depends on the flags of the first byte  
    int heartRate = pData[1];
    Serial.println(heartRate);
  }  
}

static void accelero_notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,uint8_t* pData,size_t length,bool isNotify) {
  if( pData[0] != 0x02 ) return;
  Serial.print("Acceleromter Data: ");
  int frame_type = pData[9];
  Serial.print(frame_type,':');
  for( int i = 10; pData[i] ; i++ )
    Serial.print(pData[i]+' ');
  Serial.println();
  // if (length > 1) {
  //   int heartRate = pData[1];
  //   Serial.println(heartRate);
  // }
}

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());  

    BLEClient*  pClient  = BLEDevice::createClient();  
    Serial.println(" - Created client");  

    pClient->setClientCallbacks(new MyClientCallback());  

    // Connect to the remote BLE Server.  
    pClient->connect(myDevice);  
    Serial.println(" - Connected to server");  

    // Heart Rate  
    BLERemoteService* hr_pRemoteService = pClient->getService(hr_serviceUUID);  
    if (hr_pRemoteService == nullptr) {  
      Serial.print("Failed to find heart rate service UUID: ");  
      Serial.println(hr_serviceUUID.toString().c_str());  
      return false;
    }  
    Serial.println(" - Found heart rate service");  

    hr_pRemoteCharacteristic = hr_pRemoteService->getCharacteristic(hr_charUUID);  
    if (hr_pRemoteCharacteristic == nullptr) {  
      Serial.print("Failed to find our characteristic UUID: ");  
      Serial.println(hr_charUUID.toString().c_str());  
      return false;  
    }  
    Serial.println(" - Found our characteristic");  

    if(hr_pRemoteCharacteristic->canNotify())  
      hr_pRemoteCharacteristic->registerForNotify(hr_notifyCallback);


    // Accelerometer data  
    BLERemoteService* accelero_pRemoteService = pClient->getService(accelero_serviceUUID);  
    if (accelero_pRemoteService == nullptr) {  
      Serial.print("Failed to find heart rate service UUID: ");  
      Serial.println(accelero_serviceUUID.toString().c_str());  
      return false;
    }  
    Serial.println(" - Found heart rate service");  

    accelero_pRemoteCharacteristic = accelero_pRemoteService->getCharacteristic(accelero_charUUID);
    if (accelero_pRemoteCharacteristic == nullptr) {  
      Serial.print("Failed to find our characteristic UUID: ");  
      Serial.println(accelero_charUUID.toString().c_str());  
      return false;  
    }  
    Serial.println(" - Found our characteristic");  

    if(accelero_pRemoteCharacteristic->canNotify())  
      accelero_pRemoteCharacteristic->registerForNotify(accelero_notifyCallback);  

    return true;  
}  

/**  
   Scan for BLE servers and find the first one that advertises the service we are looking for.  
*/  
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {  
   void onResult(BLEAdvertisedDevice advertisedDevice) {  
      Serial.print("BLE Advertised Device found: ");  
      Serial.println(advertisedDevice.toString().c_str());  

      // We have found a device, let us now see if it contains the service we are looking for.  
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(hr_serviceUUID) && advertisedDevice.isAdvertisingService(hr_serviceUUID)) {  
        Serial.println("Successfully found");
        BLEDevice::getScan()->stop(); 
        myDevice = new BLEAdvertisedDevice(advertisedDevice);  
        doConnect = true;  
        doScan = true;  

      } // Found our server  
   } // onResult  
}; // MyAdvertisedDeviceCallbacks  


void setup() {  
    Serial.begin(115200);  
    Serial.println("Starting ESP32 BLE Client application...");  
    BLEDevice::init("");  

    // Retrieve a Scanner and set the callback we want to use to be informed when we  
    // have detected a new device.  Specify that we want active scanning and start the  
    // scan to run for 5 seconds.  
    BLEScan* pBLEScan = BLEDevice::getScan();  
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());  
    pBLEScan->setInterval(1349);  
    pBLEScan->setWindow(449);  
    pBLEScan->setActiveScan(true);  
    pBLEScan->start(5, false);
}  

void loop() {  
    // If the flag "doConnect" is true then we have scanned for and found the desired  
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are   
    // connected we set the connected flag to be true.  
    if (doConnect == true) {  
      if (connectToServer()) {  
        Serial.println("We are now connected to the BLE Server.");  
      } else { 
        Serial.println("We have failed to connect to the server; there is nothing more we will do.");  
      }  
      doConnect = false;  
    }

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached  
    // with the current time since boot.  
    if (connected) {
      // Here you can read or write to your characteristic if needed  
    }
    else if(doScan){  
      BLEDevice::getScan()->start(0);  // this is just a value to get the compiling going   
    }  
    
    delay(1000); // Delay a second between loops.  
}  