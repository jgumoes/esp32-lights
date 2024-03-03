#ifndef _MODAL_LIGHTS_H_
#define _MODAL_LIGHTS_H_

#include <memory>
#include "modes.h"

/**
 * LightsClass must have method setDutyCycle(duty_t)
*/
template <class LightsClass>
class ModalLightsController
{
private:
  /* data */
  std::unique_ptr<ModalStrategy> _mode = std::make_unique<ConstantBrightnessMode>(0);
//   std::unique_ptr<LED_HAL> _lights;
// LED_HAL& _lights;
// void setDutyCycle(duty_t duty_value)
   // std::function<void(duty_t)> _dutyCycleSetter;

   LightsClass _lights;


  LightStateStruct _lightVals;  // duty cycle & state of the lights. gets checked and possibly changed by strategy instance

public:
//   ModalLightsController(LED_HAL& lightsHAL) : _lights(lightsHAL){};
//   ModalLightsController();

  /**
   * @brief sets the mode to Constant Brightness
   * 
   * @param maxBrightness can be LED_LIGHTS_MAX_DUTY
  */
  void setConstantBrightnessMode(duty_t maxBrightness){
    _mode = std::make_unique<ConstantBrightnessMode>(maxBrightness);
   // _mode = std::move(std::make_unique<ConstantBrightnessMode>(maxBrightness));
  };

  void updateLights(uint64_t currentTimestamp){
    _mode->updateBrightness(currentTimestamp, &_lightVals);
    _lights.setDutyCycle(_lightVals.brightness());
  };
  
  /**
   * checks the next brightness against the light mode,
   * then sets the returned brightness
  */
  void setBrightnessLevel(duty_t brightness){
    _lightVals.dutyLevel = brightness;
    if(brightness > 0){
      _lightVals.state = 1;
    }
    _mode->checkLightVals(&_lightVals);
    _lights.setDutyCycle(_lightVals.brightness());
  };

  duty_t getBrightnessLevel(){
    return _lightVals.brightness();
  };

  void setState(bool newState){
    _lightVals.state = newState;
    _mode->checkLightVals(&_lightVals);
    _lights.setDutyCycle(_lightVals.brightness());
  }
};

#endif