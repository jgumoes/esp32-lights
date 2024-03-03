#include "ModalLights.h"
#include <memory>

#ifdef native_env
  #include <iostream>
#endif

// // ModalLights manages the brightness of the lights
// // It does so by asking ModalStrategy what the brightness
// // should be at a given time or user input, and passes the
// // response to LEDLights
// //
// // The touch/button/encoder/etc. handler should request
// // ModalLights to set a brightness, or 
// namespace ModalLights
// {
//   namespace {
//     // private stuff
//     std::unique_ptr<ModalStrategy> _mode = std::make_unique<ConstantBrightnessMode>(0);
//     // ModalStrategy* _mode = ConstantBrightnessMode(0);

//     std::unique_ptr<LED_HAL> _lights;
//   }

//   void setConstantBrightnessMode(duty_t maxBrightness){
//     // std::make_unique<ConstantBrightnessMode>(maxBrightness);
//     // setDutyCycle(maxBrightness);
//     _mode = std::make_unique<ConstantBrightnessMode>(maxBrightness);
//   }

//   // namespace setMode {
//     // changing the mode shouldn't change the brightness
//   //   void constantBrightness(duty_t maxBrightness){
//   //     std::make_unique<ConstantBrightnessMode>(maxBrightness);
//   //     setDutyCycle(maxBrightness);
//   //   }
//   // }

//   void updateLights(uint64_t currentTimestamp){
//     setDutyCycle(_mode->updateBrightness(currentTimestamp));
//   }

//   void setBrightnessLevel(duty_t brightness){
//     std::cout << "setting brightness level";
//     // printf("setting brightness level");
//     setDutyCycle(_mode->setBrightness(brightness));
//   }

//   duty_t getBrightnessLevel(){
//     // return getDutyLevel();
//     return getDutyCycleValue();
//   }
// } // namespace ModalLights

