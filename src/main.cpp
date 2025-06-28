#include <Arduino.h>

#include "CurtainLights.hpp"

void setup(){
  run_CurtainLights();
};

bool pinState = true;

void loop(){
  // TODO: reboot because this should be innaccessible
  // Serial.println("loop...");
  delay(200);
};

