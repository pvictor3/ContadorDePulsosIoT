#include <Arduino.h>
#include <LiquidCrystal.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID                  "32c64438-23c1-40e3-8a85-2dddc120e432" // FlowMeter service UUID
#define CHARACTERISTIC_UUID_COUNTER   "3edde8fa-1ab3-4162-b53f-42884f1f6714" // Pulse Counter characteristic
#define CHARACTERISTIC_UUID_BATTERY   "ae994543-4583-4cd4-aa97-692ca9bfa711" // Battery level characteristic
#define CHARACTERISTIC_UUID_VALVE     "d752ffc7-0123-4932-92a6-920bc7852bcd" // Valve status characteristic
#define CHARACTERISTIC_UUID_DEVICENUMBER  "e0583abd-28a7-4c2a-8b7b-f156e2394ec4" //Device number characteristic
#define CHARACTERISTIC_UUID_BRAND  "af815952-d651-496a-9942-7e40684ff2ed"     //Brand characteristic

const char *OPEN_COM = "Abierto";
const char *CLOSE_COM = "Cerrado";

void IRAM_ATTR isr();
const byte intPin = 18;
volatile int pulseCounter = 0;

const byte rs = 5, en = 17, d4 = 16, d5 = 4, d6 = 2, d7 = 15;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const byte voltagePin = 36;

const byte servoPin = 19;
const byte servoChannel = 1;
int dutyCycle = 25;

BLEServer* pServer = NULL;
BLEService *pService = NULL;
BLECharacteristic *pCounter = NULL;
BLECharacteristic *pBattery = NULL;
BLECharacteristic *pValve = NULL;
BLECharacteristic *pDeviceNumber = NULL;
BLECharacteristic *pBrand = NULL;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks
{
  void onConnect(BLEServer* pServer)
  {
      deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer)
  {
      deviceConnected = false;
  }
};

class MyCounterCallbacks: public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    char count[16] = "";
    itoa(pulseCounter, count, 10);
    Serial.print("Pulsos enviados = ");
    Serial.println(count);
    pCharacteristic -> setValue(count);
  }

  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String newCounter = pCharacteristic -> getValue().c_str();
    int count = newCounter.toInt();
    if(!isnan(count))
    {
      pulseCounter = count;
      Serial.println("Conteo actualizado");
    }
    else
    {
      Serial.println("Dato no válido");
    }
  }
};

class MyBatteryCallbacks: public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
      int currentVoltage = analogRead(voltagePin);
      char voltage[16] = "";
      itoa(currentVoltage, voltage, 10);
      pCharacteristic -> setValue(voltage);
  }
};

class MyValveCallbacks: public BLECharacteristicCallbacks 
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    //Leer estado actual
    if(dutyCycle == 25)
    {
      pCharacteristic -> setValue("Abierta");
    }else if(dutyCycle == 13)
    {
      pCharacteristic -> setValue("Cerrada");
    }
    
  }

  void onWrite(BLECharacteristic *pCharacteristic)
  {
    String newState = pCharacteristic -> getValue().c_str();
    //Actualizar estado actual
    if(newState.equals(OPEN_COM))
    {
      dutyCycle = 25;
      ledcWrite(servoChannel,dutyCycle);
    }else if(newState.equals(CLOSE_COM))
    {
      dutyCycle = 13;
      ledcWrite(servoChannel,dutyCycle);  
    }else
    {
      Serial.println("Comando de válvula inválido");  
    }  
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Init peripherals...");

  //Activar interrupciones para contar los pulsos
  pinMode(intPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(intPin), isr, FALLING);

  //Inicializar LCD
  lcd.begin(16, 2);
  lcd.setCursor(1,0);
  lcd.print("Medidor 1308");
  lcd.setCursor(8,1);
  lcd.print("m3");

  //Inicializar PWM
  ledcSetup(1,50,8); //(channel, freq, resolution)
  ledcAttachPin(servoPin, 1);
  ledcWrite(servoChannel,dutyCycle);

  //Inicializar el BLE
  BLEDevice::init("ESP32");

  //Crear el servidor
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  //Crear un servicio
  pService = pServer -> createService(SERVICE_UUID);

  //Crear las caracteristicas del servicio
  pCounter = pService -> createCharacteristic(
                                              CHARACTERISTIC_UUID_COUNTER,
                                              BLECharacteristic::PROPERTY_READ |
                                              BLECharacteristic::PROPERTY_WRITE |
                                              BLECharacteristic::PROPERTY_NOTIFY
                                              );
  pCounter -> setCallbacks(new MyCounterCallbacks());
  pCounter -> addDescriptor(new BLE2902());

  pBattery = pService -> createCharacteristic(
                                              CHARACTERISTIC_UUID_BATTERY,
                                              BLECharacteristic::PROPERTY_READ
                                              );
  pBattery -> setCallbacks(new MyBatteryCallbacks());

  pValve = pService -> createCharacteristic(
                                            CHARACTERISTIC_UUID_VALVE,
                                            BLECharacteristic::PROPERTY_READ |
                                            BLECharacteristic::PROPERTY_WRITE
                                            );
  pValve -> setCallbacks(new MyValveCallbacks());

  pDeviceNumber = pService -> createCharacteristic(
                                                CHARACTERISTIC_UUID_DEVICENUMBER,
                                                BLECharacteristic::PROPERTY_READ
                                                );
  pDeviceNumber -> setValue("130894");

  pBrand = pService -> createCharacteristic(
                                                CHARACTERISTIC_UUID_BRAND,
                                                BLECharacteristic::PROPERTY_READ |
                                                BLECharacteristic::PROPERTY_WRITE
                                                );
  pBrand -> setValue("BLU TOWER");

  //Empezar el servicio
  pService -> start();

  //Hacerse visible a los clientes
  //Opcional, si ya se conoce la direccion no hace falta.
  pServer -> getAdvertising() -> start();

  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  // mostrar el numero de pulsos
  lcd.setCursor(0, 1);
  lcd.print(pulseCounter);
  delay(1000);
  if(deviceConnected)
  {
    char count[16] = "";
    itoa(pulseCounter, count, 10);
    pCounter -> setValue(count);
    pCounter -> notify();
    Serial.println("Dispositivo conectado");
  }else
  {
    Serial.println("Dispositivo desconectado");
  }
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
