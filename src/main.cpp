// third-party libraries
#include <Arduino.h>
// #include "driver/touch_sensor.h"
#include <Wire.h>
#include "driver/timer.h"

// my libraries
// #include "DeviceTimeService.h"

// source files
#ifdef XIAO_ESP32S3
  #include "touch_switch.h"
#endif
#include "lights_pwm.h"
#include <configManager.h>

ConfigManagerClass ConfigManager;
RTCInterfaceClass<TwoWire, ConfigManagerClass> RTC = RTCInterfaceClass<TwoWire, ConfigManagerClass>(Wire, ConfigManager);

#define TIMER_GROUP TIMER_GROUP_0
#define TIMER_NUM TIMER_0

/**
 * @brief initiate a General Purpose Timer to act as an on-board RTC.
 * It measures in microseconds because it uses the 80MHz clock,
 * but won't overflow until 2570.
 * 
 */
void setupTimer(){
  const timer_config_t config = {
    .alarm_en = TIMER_ALARM_DIS,
    .counter_dir = TIMER_COUNT_UP,
    .auto_reload = TIMER_AUTORELOAD_DIS,
    .divider = 80
  };
  timer_init(TIMER_GROUP, TIMER_NUM, &config);
  uint64_t startTime = RTC.getLocalTimestamp() * 1000000;
  timer_set_counter_value(TIMER_GROUP, TIMER_NUM, startTime);
  timer_start(TIMER_GROUP, TIMER_NUM);
}


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
  // touchAttachInterrupt(T4, toggle_Lights_State, 90);
  // touchInterruptSetThresholdDirection(true);
#endif
  setupTimer();
}
uint led_brightness = 0;
uint led_brightness_increment = 100/4;

void cycle_led_brightness(){
  led_brightness = (led_brightness + led_brightness_increment) % 101;
  setPowerLevel(led_brightness);
  Serial.print("led brightness:"); Serial.println(led_brightness);
}

volatile bool touchToggle = true;
uint cumDelay = 0;
#define LOOP_DELAY 50

void loop() {
  // toggle_Lights_State();
  // Serial.print("power level: "); Serial.println(getPowerLevel());
  // Serial.print("lights state: "); Serial.println(isLightsOn());
  // cycle_led_brightness();
  if(cumDelay >= 1000){
    uint64_t time1;
    double time2;
    timer_get_counter_value(TIMER_GROUP, TIMER_NUM, &time1);
    timer_get_counter_time_sec(TIMER_GROUP, TIMER_NUM, &time2);
    Serial.print(time1); Serial.print("; ");
    Serial.println(time2);
    cumDelay = 0;
  }
  else{
    cumDelay += LOOP_DELAY;
  }

#ifdef DEVKIT
  if(touchRead(T4) < 90){
    if(touchToggle){
      toggle_Lights_State();
      touchToggle = false;
    }
  }
  else{
    touchToggle = true;
  }
  delay(LOOP_DELAY);
#endif
}