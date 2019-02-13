#include <Arduino.h>
#include <LiquidCrystal.h>
const byte intPin = 15;
volatile int pulseCounter = 0;

const byte rs = 5, en = 17, d4 = 16, d5 = 4, d6 = 0, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void IRAM_ATTR isr();

void setup() {
  Serial.begin(115200);
  Serial.println("Init peripherals...");

  //Activar interrupciones para contar los pulsos
  pinMode(intPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(intPin), isr, FALLING);

  //Inicializar LCD
  lcd.begin(16, 2);
  lcd.print("Hello world!");
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.setCursor(0, 1);
  lcd.print(pulseCounter);
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
