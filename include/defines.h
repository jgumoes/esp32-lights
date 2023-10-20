#ifndef __DEFINES_H__
#ifdef XIAO_ESP32S3
    // defines for lights_pwm
  #define PWM_0 2   // D1
  #define PWM_1 4   // D3
  #define PWM_FREQ 1000 // set frequency to 1 kHz. this is plenty for LED lights, without risking excessive RF interferance to the room

  // defines for touch_switch
  #define TOUCH_PIN 1 // touch pad 4, gpio pin D0
  #define TOUCH_THRESHOULD 50000
  #define TOUCH_DELAY 500 // minimum time between touch events, in milliseconds
#endif

#ifdef DEVKIT
  // defines for lights_pwm
  #define PWM_0 2
  #define PWM_1 4
  #define PWM_FREQ 1000 // set frequency to 1 kHz. this is plenty for LED lights, without risking excessive RF interferance to the room

  // defines for touch_switch
  #define TOUCH_PIN 13 // touch pad 4, gpio pin D13; for esp32 wroom 32
  #define TOUCH_THRESHOULD 50
  #define TOUCH_DELAY 500 // minimum time between touch events, in milliseconds
#endif
#endif