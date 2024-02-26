#include "ModalLights.h"
#include <memory>

#ifdef native_env
  #include <iostream>
#endif

// ModalLights manages the brightness of the lights
// It does so by asking ModalStrategy what the brightness
// should be at a given time or user input, and passes the
// response to LEDLights
//
// The touch/button/encoder/etc. handler should request
// ModalLights to set a brightness, or 
namespace ModalLights
{
  namespace {
    // private stuff
    std::unique_ptr<ModalStrategy> _mode = std::make_unique<ConstantBrightnessMode>(0);
    // ModalStrategy* _mode = ConstantBrightnessMode(0);

    std::unique_ptr<LED_HAL> __lights;
  }

  void setConstantBrightnessMode(duty_t maxBrightness){
    // std::make_unique<ConstantBrightnessMode>(maxBrightness);
    // setDutyCycle(maxBrightness);
    _mode = std::make_unique<ConstantBrightnessMode>(maxBrightness);
  }

  // namespace setMode {
    // changing the mode shouldn't change the brightness
  //   void constantBrightness(duty_t maxBrightness){
  //     std::make_unique<ConstantBrightnessMode>(maxBrightness);
  //     setDutyCycle(maxBrightness);
  //   }
  // }

  void updateLights(uint64_t currentTimestamp){
    setDutyCycle(_mode->updateBrightness(currentTimestamp));
  }

  void setBrightnessLevel(duty_t brightness){
    std::cout << "setting brightness level";
    // printf("setting brightness level");
    setDutyCycle(_mode->setBrightness(brightness));
  }

  duty_t getBrightnessLevel(){
    // return getDutyLevel();
    return getDutyCycleValue();
  }
} // namespace ModalLights


// second re-write
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

// class ModalLights
// {
// private:
//   /* data */
//   std::unique_ptr<ModalStrategy> _mode = std::make_unique<ConstantBrightnessMode>(0);
//   std::unique_ptr<LED_HAL> __lights;

//   LightStateStruct _lightVals;  // cycle, state of the lights. gets checked and possibly changed by strategy instance

// public:
//   ModalLights(LED_HAL lightsHAL);
//   ~ModalLights();

//   /**
//    * @brief sets the mode to Constant Brightness
//    * 
//    * @param maxBrightness can be LED_LIGHTS_MAX_DUTY
//   */
//   void setConstantBrightnessMode(duty_t maxBrightness){
//     _mode = std::make_unique<ConstantBrightnessMode>(maxBrightness);
//   };

//   // void setMode(ModalStrategy newStrategy){
//   //   _mode = std::make_unique<ModalStrategy>(newStrategy);
//   // }

//   void updateLights(uint64_t currentTimestamp){
//     _mode->updateBrightness(currentTimestamp, &_lightVals);
//     __lights->setDutyCycle(_lightVals.brightness());
//   };
  
//   /**
//    * checks the next brightness against the light mode,
//    * then sets the returned brightness
//   */
//   void setBrightnessLevel(duty_t brightness){
//     _lightVals.dutyLevel = brightness;
//     _mode->findNextBrightness(&_lightVals);
//     __lights->setDutyCycle(_lightVals.brightness());
//   };

//   duty_t getBrightnessLevel(){
//     return _lightVals.brightness();
//   };

//   void setState(bool newState){
//     _lightVals.state = newState;
//     _mode->findNextBrightness(&_lightVals);
//     __lights->setDutyCycle(_lightVals.brightness());
//   }
// };
