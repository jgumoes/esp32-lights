#include <Arduino.h>
#include "touch_switch.h"
#include "ac_lights_pwm.h"

void setup() {
  Serial.begin(9600);
  Serial.println("setting up MCPWM");
  setup_PWM();

  setupTouch();

  pinMode(15, INPUT);
  // printVals();
}

void loop() {

#ifdef DEVKIT
  uint pot_val = analogRead(15);
  // uint duty = map(pot_val, 0, 4095, 0, 50);
  float duty = 100. * pot_val / 4095;
  set_PWM_Duty(duty);
  // Serial.print("duty cycle: "); Serial.print(duty); Serial.println("%");
#endif

#ifdef PRINT_TOUCH
  print_Touch();
#endif
  update_Lights();

  Serial.print("lights state: "); Serial.println(get_Lights_State());
  delay(200);
}