// third-party libraries
#include <Arduino.h>

// my libraries
// #include "DeviceTimeService.h"

// source files
#include "touch_switch.h"
#include "ac_lights_pwm.h"

void setup() {
  Serial.begin(9600);
  Serial.println("setting up MCPWM");
  setup_PWM(2, 3);
  
  // setupTouch(TOUCH_PAD_NUM1);

}

void loop() {

// #ifdef DEVKIT
//   uint pot_val = analogRead(15);
//   // uint duty = map(pot_val, 0, 4095, 0, 50);
//   float duty = 100. * pot_val / 4095;
//   set_PWM_Duty(duty);
//   // Serial.print("duty cycle: "); Serial.print(duty); Serial.println("%");
// #endif

// #ifdef PRINT_TOUCH
  // print_Touch();
// #endif
  // update_Lights();

  Serial.print("lights state: "); Serial.println(get_Lights_State());
  Serial.print("duty cycle: "); Serial.println(get_PWM_Duty());
  delay(200);
}