#include <BLEDevice.h> 
#include <DHT.h>

static BLEUUID serviceUUID("0000181A-0000-1000-8000-00805F9B34FB"); 
static BLEUUID    charUUID("00003A6F-0000-1000-8000-00805F9B34FB"); 
static BLEUUID charUUID_tx("00002A6F-0000-1000-8000-00805F9B34FB");
String My_BLE_Address = "24:0a:c4:c5:f1:8e"; 
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristic_tx;
static String identity ="NodeA";

#define DHTPIN 23
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println(pData[0]);
    if(pData[0]==1){
      Serial.println("Mendapatkan Sinyal Untuk Menghidupkan Sprayer");
      digitalWrite (2,HIGH);
      Serial.println("Sprayer Hidup");
      Serial.println();
    }else if(pData[0]==0){
      Serial.println("Mendapatkan Sinyal Untuk Menghidupkan Sprayer");
      digitalWrite (2,LOW);
      Serial.println("Sprayer Mati");
      Serial.println();
    }
}

void tx(float humid) {
  uint8_t humid_t = (uint8_t)humid;
  pRemoteCharacteristic_tx->writeValue(&humid_t, 1); 
  Serial.print("Mengirim Data Humidity Ke Getaway : ");
  Serial.print(humid);
  Serial.println("%");
  Serial.println("Selesai");
   Serial.println();
  delay(10000);
}

BLEScan* pBLEScan;
BLEScanResults foundDevices;

static BLEAddress *Server_BLE_Address;
String Scaned_BLE_Address;

boolean paired = false;

bool connectToserver (BLEAddress pAddress){
    Serial.println(" tesssssss");
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->connect(pAddress);
    Serial.println(" - Connected to GETAWAY");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");
      pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
      pRemoteCharacteristic_tx = pRemoteService->getCharacteristic(charUUID_tx);
      if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");
    Serial.println(charUUID.toString().c_str());

    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);
}



class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Scan Result: %s \n", advertisedDevice.toString().c_str());
      Server_BLE_Address = new BLEAddress(advertisedDevice.getAddress());
      
      Scaned_BLE_Address = Server_BLE_Address->toString().c_str();
      
    }
};

void setup() {
    delay(5000);
    Serial.begin(115200); 
    Serial.println("ESP32 BLE Server program");
    dht.begin();

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); 
    pBLEScan->setActiveScan(true);

    pinMode (2,OUTPUT);
}

void loop() {
  if(paired){
    tx(dht.readHumidity());
    return;
  }else{
    foundDevices = pBLEScan->start(3); 
  }
  
  while (foundDevices.getCount() >= 1)
  {
    if (Scaned_BLE_Address == My_BLE_Address && paired == false)
    {
      Serial.println("Found Device :-)... connecting to Server as client");
       if (connectToserver(*Server_BLE_Address))
      {
      paired = true;
      digitalWrite (2,HIGH);
      delay(1000);
      digitalWrite (2,LOW);
      break;
      }
      else
      {
      Serial.println("Pairing failed");
      break;
      }
    }
    
    if (Scaned_BLE_Address == My_BLE_Address && paired == true)
    {
      Serial.println("Our device went out of range");
      paired = false;
      digitalWrite (2,LOW);
      ESP.restart();
      break;
    }
    else
    {
    Serial.println("We have some other BLe device in range");
    break;
    }
  } 
}
