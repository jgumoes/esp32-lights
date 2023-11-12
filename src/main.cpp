// third-party libraries
#include <Arduino.h>
#include "driver/touch_sensor.h"

// my libraries
// #include "DeviceTimeService.h"

// source files
#include "touch_switch.h"
#include "lights_pwm.h"
#include <configManager.h>

void setup() {
  Serial.begin(9600);
  Serial.println("setting up...");
  
#ifdef XIAO_ESP32S3
  setup_PWM(D9, 18500, 0);
  setupTouch(TOUCH_PAD_NUM7);
#elif DEVKIT
  setup_PWM(23, 18500, 0);
  // setupTouch(TOUCH_PAD_NUM2); // GPIO2
  // touch_pad_init();
  // touch_pad_io_init(TOUCH_PAD_NUM2);
#endif

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
  cycle_led_brightness();

  print_Touch();
  // this doesn't work for the devkit
  // uint touch_val = touchRead(TOUCH_PAD_NUM2);
  // // uint16_t touch_val;
  // // touch_pad_read_raw_data(TOUCH_PAD_NUM2, &touch_val);
  // Serial.print("touch value: "); Serial.println(touch_val);

  delay(800);
}