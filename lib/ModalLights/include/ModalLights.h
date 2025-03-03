#ifndef _MODAL_LIGHTS_H_
#define _MODAL_LIGHTS_H_

#include <memory>
#include "modes.h"
#include "lightDefines.h"

class VirtualLightsClass{
  public:
  virtual ~VirtualLightsClass() = default;
  virtual void setDutyCycle(duty_t duty) = 0;
};

/**
 * LightsClass must have method setDutyCycle(duty_t)
*/
class ModalLightsController
{
private:
  /* data */
  std::unique_ptr<ModalStrategy> _mode = std::make_unique<ConstantBrightnessMode>(0);

   std::unique_ptr<VirtualLightsClass> _lights;

  // std::unique_ptr<LightsInterfaceClass> _lights;
  // std::unique_ptr<BrightnessStrategy> _mode = std::make_unique<ConstantBrightnessMode>(0);
  // std::unique_ptr<ColourStrategy> _mode = std::make_unique<SingleChannel>(0);

  LightStateStruct _lightVals;  // duty cycle & state of the lights. gets checked and possibly changed by strategy instance

public:

  ModalLightsController(std::unique_ptr<VirtualLightsClass> lightsClass) : _lights(std::move(lightsClass)){}
  /**
   * @brief sets the mode to Constant Brightness
   * 
   * @param maxBrightness can be LED_LIGHTS_MAX_DUTY
  */
  void setConstantBrightnessMode(duty_t maxBrightness){
    _mode = std::make_unique<ConstantBrightnessMode>(maxBrightness);
  };

  void updateLights(uint64_t currentTimestamp){
    _mode->updateBrightness(currentTimestamp, &_lightVals);
    // TODO: pass lightVals to colour mode
    _lights->setDutyCycle(_lightVals.getBrightness());
  };
  
  /**
   * checks the next brightness against the light mode,
   * then sets the returned brightness
  */
  void setBrightnessLevel(duty_t brightness){
    _lightVals.setBrightness(brightness);
    _mode->checkLightVals(&_lightVals);
    _lights->setDutyCycle(_lightVals.getBrightness());
  };

  duty_t getBrightnessLevel(){
    return _lightVals.getBrightness();
  };

  bool getState(){
    return _lightVals.state;
  }
  
  void setState(bool newState){
    _lightVals.state = newState;
    _mode->checkLightVals(&_lightVals);
    _lights->setDutyCycle(_lightVals.getBrightness());
  }

  void toggleState(){
    setState(!_lightVals.state);
  }

  void adjustBrightness(duty_t amount, bool direction){
    // check the adjustment won't do anything weird
    duty_t newBrightness;
    const duty_t currentBrightness = _lightVals.getBrightness();
    if(direction){
      newBrightness = (LED_LIGHTS_MAX_DUTY - currentBrightness < amount) ?
        LED_LIGHTS_MAX_DUTY : currentBrightness + amount;
    }
    else{
      newBrightness = (currentBrightness < amount) ? 0 : currentBrightness - amount;
    }

    _lightVals.setBrightness(newBrightness);
    _mode->checkLightVals(&_lightVals);
    _lights->setDutyCycle(_lightVals.getBrightness());
  }
};

#endif