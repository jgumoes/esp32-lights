// third-party libraries
#include <Arduino.h>

// my libraries
// #include "DeviceTimeService.h"

// source files
#include "touch_switch.h"
#include "ac_lights_pwm.h"

void setup() {
  Serial.begin(9600);
  Serial.println("setting up...");
  setup_PWM(D1, D2);
  
  setupTouch(TOUCH_PAD_NUM7);

  setPowerLevel(100);
}
uint led_brightness = 0;
uint led_brightness_increment = 100/4;

void cycle_led_brightness(){
  led_brightness = (led_brightness + led_brightness_increment) % 101;
  setPowerLevel(led_brightness);
  Serial.print("led brightness:"); Serial.println(led_brightness);
}

void loop() {
  // toggle_Lights_State();
  Serial.print("power level: "); Serial.println(getPowerLevel());
  Serial.print("lights state: "); Serial.println(isLightsOn());
  // if(get_Lights_State()){
  //   cycle_led_brightness();
  // }

// #ifdef PRINT_TOUCH
  // print_Touch();
// #endif
  // update_Lights();

  // Serial.print("lights state: "); Serial.println(get_Lights_State());
  // Serial.print("duty cycle: "); Serial.println(getPowerLevel());
  delay(800);
}