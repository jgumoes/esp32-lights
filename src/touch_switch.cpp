#include <Arduino.h>
#include "defines.h"
#include "driver/touch_sensor.h"
#include "touch_switch.h"
#include "ac_lights_pwm.h"

void touchInterrupt(){
  toggle_Lights_State();
}

/*
 * Setup the interrupt for the touch switch. May not work
 * on ESP32S3.
 */
void setupTouch(){
  touch_pad_set_meas_time(150000 / 5, 0xffff);  // limit how quickly the light state cycles by setting the touch measurement to the slowest possible
  touchAttachInterrupt(TOUCH_PIN, touchInterrupt, TOUCH_THRESHOULD);
}

/*
 * Reads the touch value and prints it to serial
 */
void print_Touch(){
  uint touch_val = touchRead(TOUCH_PIN); // goes from 70 to 7
  Serial.print("touch value: "); Serial.println(touch_val);
}