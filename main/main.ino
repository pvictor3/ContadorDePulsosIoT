#include <Arduino.h>
#include <LiquidCrystal.h>
#define intPin 6
volatile int pulseCounter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Init peripherals...");

  //Activar interrupciones para contar los pulsos
  pinMode(intPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(intPin), isr, FALLING);

  //Inicializar LCD

}

void loop() {
  // put your main code here, to run repeatedly:

}

void IRAM_ATTR isr(){
    pulseCounter++;
  }
