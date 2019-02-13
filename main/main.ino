#include <Arduino.h>
#include <LiquidCrystal.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer* pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks{
  void onConnect(BLEServer* pServer){
      deviceConnected = true;
    };

  void onDisconnect(BLEServer* pServer){
      deviceConnected = false;
    }
  };

class MyCallbacks: public BLECharacteristicCallbacks{
  void onWrite(BLECharacteristic *pCharacteristic){
      std::string rxValue = pCharacteristic -> getValue();
      if(rxValue.length() > 0)
      {
        Serial.println("*********");
        Serial.print("Received Value: ");
        Serial.println(rxValue);
      }
    }
  };

void IRAM_ATTR isr();
const byte intPin = 15;
volatile int pulseCounter = 0;

const byte rs = 5, en = 17, d4 = 16, d5 = 4, d6 = 0, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const byte voltagePin = 36;

void setup() {
  Serial.begin(115200);
  Serial.println("Init peripherals...");

  //Activar interrupciones para contar los pulsos
  pinMode(intPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(intPin), isr, FALLING);

  //Inicializar LCD
  lcd.begin(16, 2);
  lcd.print("Hello world!");

  //Configurar BLE
  BLEDevice::init("ESP32");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer -> createService(SERVICE_UUID);
  pCharacteristic = pService -> createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic -> addDescriptor(new BLE2902());

  pCharacteristic = pService -> createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());

  pService -> start();

  pServer -> getAdvertising() -> start();

  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // mostrar el numero de pulsos
  lcd.setCursor(0, 1);
  lcd.print(pulseCounter);

  if(deviceConnected){
    int txValue = analogRead(voltagePin);
    char txString[8];
    dtostrf(txValue, 1, 2, txString);

    pCharacteristic -> setValue(txString);
    pCharacteristic -> notify();
    Serial.print("*** Sent Value: ");
    Serial.print(txString);
    Serial.println(" ***");
    }
    delay(1000);
}

void IRAM_ATTR isr(){
    pulseCounter++;
  }
  
/*
 The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
*/
