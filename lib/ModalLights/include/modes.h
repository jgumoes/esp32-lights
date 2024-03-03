#ifndef _LIGHT_MODES_H_
#define _LIGHT_MODES_H_
#include <stdint.h>
#include "lights_pwm.h"

struct LightStateStruct {
  duty_t dutyLevel = 0;   // the duty cycle of the lights. can have a value even when off
  bool state = 0;         // i.e. on or off

  /**
   * @brief returns the brightness of the lights
  */
  duty_t brightness(){
    return dutyLevel * state;
  };
};

class ModalStrategy
{
public:
  // virtual ~ModalStrategy();

  /**
   * @brief calculates and updates the light values based
   * on the current timestamp
   * 
   * @param currentTimestamp the current timestamp in microseconds
  */
  virtual void updateBrightness(uint64_t currentTimestamp, LightStateStruct* lightVals){};

  /**
   * @brief re-adjusts the strategy params, and updates lightVals with
   * correct values accordingly
   * (i.e. if the brightness is being set above a maximum)
  */
  virtual void checkLightVals(LightStateStruct* lightVals){};

};

#ifndef CONSTANT_BRIGHTNESS_MINIMUM
  #define CONSTANT_BRIGHTNESS_MINIMUM (LED_LIGHTS_MAX_DUTY * 0.05)
#endif
class ConstantBrightnessMode : public ModalStrategy
{
private:
  duty_t _maxBrightness;

public:
  ConstantBrightnessMode(duty_t brightness) : _maxBrightness(brightness){};

  void checkLightVals(LightStateStruct* lightVals) override {
    // state shouldn't effect it
    if(lightVals->dutyLevel > _maxBrightness){
      lightVals->dutyLevel = _maxBrightness;
    }

    if(lightVals->state && lightVals->dutyLevel == 0){
      lightVals->dutyLevel = CONSTANT_BRIGHTNESS_MINIMUM;
    }
  }

  void updateBrightness(uint64_t currentTimestamp, LightStateStruct* lightVals) override {
    return;
  }
};


#endif