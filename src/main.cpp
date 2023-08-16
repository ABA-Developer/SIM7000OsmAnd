#include <Arduino.h>

#include "data.h"
#include "battery/battery.h"
#include "sim7000/sim7000.h"
#include "sdcard/sdcard.h"

void setup()
{
  // init pin
  pinMode(SIM_7000_ENABLE_PIN, OUTPUT);
  pinMode(CHARGING_PIN, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(DI1_PIN, INPUT);
  pinMode(ACCU_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT);
  
  initsdcard(); // init SD card

  Serial.begin(115200);
}

void loop(){}