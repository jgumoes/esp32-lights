#ifndef _LIGHT_MODES_H_
#define _LIGHT_MODES_H_

// #include "../DeviceTime/include/DeviceTime.h"
#include "DeviceTime.h"
#include "lightDefines.h"
#include "interpolationClass.h"

/**
 * @brief is performing a quick change necessary? basically, are the two arrays different?
 * 
 * @param lightVals the actual current values
 * @param startingValues the desired current values
 * @return true if the arrays are different and need quick change
 * @return false if the arrays are the same and don't need quick change
 */
static bool isQuickChangeNeeded(
  LightStateStruct& lightVals,
  duty_t startingValues[nChannels+1]
){
  for(uint8_t i = 0; i < nChannels+1; i++){
    if(lightVals.values[i] != startingValues[i]){
      return true;
    }
  }
  return false;
}

/**
 * @brief checks if the colours portion of the values array are set
 * 
 * @param values 
 * @return true if all colour ratios are 0
 * @return false 
 */
static bool areColoursEmpty(duty_t values[nChannels+1]){
  for(uint8_t i = 1; i < nChannels+1; i++){
    if(values[i] != 0){
      return false;
    }
  }
  return true;
};

class ModalStrategyInterface
{
public:
  // TODO: fast interpolation should be common to all modes

  /**
   * @brief calculates and updates the light values based
   * on the current timestamp
   * 
   * @param utcTimestamp_uS the current timestamp in microseconds
  */
  virtual void updateLightVals(uint64_t utcTimestamp_uS, LightStateStruct& lightVals) = 0;

  /**
   * @brief politely asks the mode to set the brightness to a given value. the mode will update lightVals accordingly. if the brightness is being adjusted (e.g. incremented by encoder value), soft change shouldn't be used and softChange should be false. if the brightness is being set to a specific value (e.g. app sets brightness to 150), soft change should be used and softChange should be true.
   * 
   * @param utcTimestamp_uS 
   * @param lightVals 
   * @param brightness 
   * @param softChange i.e. should the brightness change be gradually?
   */
  virtual duty_t setBrightness(uint64_t utcTimestamp_uS, LightStateStruct& lightVals, duty_t brightness, bool softChange) = 0;

  /**
   * @brief politely asks the mode to set the state. the mode will update lightVals accordingly. since state change comes from button presses, and button presses cancel active modes, this method returns true if it thinks it's ready to be cancelled. background modes should always return false.
   * 
   * @param utcTimestamp_uS 
   * @param lightVals 
   * @param newState 
   * @return bool should the mode be cancelled?
   */
  virtual bool setState(uint64_t utcTimestamp_uS, LightStateStruct& lightVals, bool newState) = 0;

  /**
   * @brief fills vals with the current target brightness and colour ratios. this would either be the expected current values, or the values at max for the flashing modes.
   * 
   * @param vals vals[0] is brightness, the rest is colour ratios
   */
  virtual void getTargetVals(duty_t vals[nChannels+1], uint64_t utcTimestamp_uS, LightStateStruct& lightVals) = 0;

  /**
   * @brief returns the target brightness
   * 
   * @return duty_t 
   */
  virtual duty_t getTargetBrightness() = 0;

  /**
   * @brief alert the mode that the internal clock has been adjusted, and that any stored times also need to be adjusted.
   * 
   * @param adjustment_uS adjustment amount in microseconds
   */
  virtual void timeAdjust(int64_t adjustment_uS) = 0;

  /**
   * @brief perform any logic concerning the change of the softOnWindow and update the lights
   * 
   * @param newWindow_S 
   */
  virtual void changeSoftChangeWindow(uint8_t newWindow_S, uint64_t utcTimestamp_uS, LightStateStruct& lightVals) = 0;

  /**
   * @brief perform any logic concerning a change in minimum on brightness and update the lights
   * 
   * @param newMinBrightness 
   */
  virtual void changeMinOnBrightness(duty_t newMinBrightness, uint64_t utcTimestamp_uS, LightStateStruct& lightVals) = 0;
};

class ConstantBrightnessMode : public ModalStrategyInterface
{
private:
  std::shared_ptr<InterpolationClass> _interpClass;
  duty_t _softChangeWindow_S;
  duty_t _minOnBrightness;
  duty_t _minSettableBrightness;  // either active min or _minOnBrightness

public:
  const bool isActive;
  const ModeTypes type = ModeTypes::constantBrightness;

  const ModeDataStruct* modeData;

  /**
   * @brief Construct a new Constant Brightness Mode object
   * 
   * @param currentTime_uS 
   * @param triggerTime_uS 
   * @param modeData
   * @param interpClass
   * @param currentVals 
   * @param isActive 
   * @param configs 
   */
  ConstantBrightnessMode(
    uint64_t currentTime_uS,
    uint64_t triggerTime_uS,
    ModeDataStruct *modeDataStruct,
    std::shared_ptr<InterpolationClass> interpClass,
    LightStateStruct& currentVals,
    bool isActive,
    const ModalConfigsStruct& configs
  ) : ModalStrategyInterface(),
      modeData(modeDataStruct),
      _interpClass(interpClass),
      isActive(isActive),
      _softChangeWindow_S(configs.softChangeWindow),
      _minOnBrightness(configs.minOnBrightness)
  {
    uint64_t utcStartTime_uS = currentTime_uS;
    _minSettableBrightness = _minOnBrightness;

    duty_t oldTargets[nChannels+1]; _interpClass->getFinalValues(oldTargets);

    // fill target colour vals
    duty_t targetValues[nChannels+1] = {_interpClass->getFinalBrightness()};
    memcpy(&targetValues[1], modeData->endColourRatios, nChannels);

    // set current vals to target vals if lights are off
    if(currentVals.state == 0 || currentVals.values[0] < _minOnBrightness){
      memcpy(currentVals.values, targetValues, nChannels+1);
    }

    // force on default brightness if brightness is under default
    if(isActive){
      const duty_t modeMinB = modeData->minBrightness;
      _minSettableBrightness = modeMinB > _minOnBrightness ? modeMinB : _minOnBrightness;
      currentVals.state = true;
      // force current vals to min on brightness
      if(currentVals.values[0] < _minOnBrightness){currentVals.values[0] = _minOnBrightness;}
      targetValues[0] = currentVals.values[0] <= _minSettableBrightness
                        ? _minSettableBrightness
                        : currentVals.values[0];
    }
    uint64_t window = _softChangeWindow_S * secondsToMicros;
    _interpClass->initialise(utcStartTime_uS, window, currentVals, targetValues);
    updateLightVals(utcStartTime_uS, currentVals);
  };

  void updateLightVals(uint64_t utcTimestamp_uS, LightStateStruct& lightVals) override {
    _interpClass->update(lightVals, utcTimestamp_uS);

    if(lightVals.values[0] < _minOnBrightness){
      lightVals.state = false;
      lightVals.values[0] = 0;
      _interpClass->getFinalValues(lightVals.values); // TODO: will if statements make this more efficient?
    }
    return;
  }

  duty_t setBrightness(uint64_t utcTimestamp_uS, LightStateStruct& lightVals, duty_t brightness, bool softChange) override {
    uint8_t window = softChange ? _softChangeWindow_S : 0;

    if(isActive && (brightness < _minSettableBrightness)){brightness = _minSettableBrightness;} // TODO: min values should be set as pointers in the initialisation. for active mode, minOnBrightness should point to defaultBrightness
    
    if(brightness < _minOnBrightness){
      brightness = (_minOnBrightness-1) * softChange; // minB - 1 is identical to 0
    }
    else{
      // if new brightness is on, make sure state is on and current brightness is at least min
      lightVals.state = true; // TODO: is this redundant? can it be removed? does it duplicate the final line of this method? should it be removed?
      if(lightVals.values[0] < _minOnBrightness){
        lightVals.values[0] = _minOnBrightness;
      }
    }
    
    _interpClass->setFinalBrightness(
      brightness, lightVals, utcTimestamp_uS, window
    );
    updateLightVals(utcTimestamp_uS, lightVals);
    return brightness;
  }

  /**
   * @brief politely asks the mode to set the state. the mode will update lightVals accordingly. since state change comes from button presses, and button presses cancel active modes, this method returns true if it thinks it's ready to be cancelled. background modes should always return false.
   * 
   * @param utcTimestamp_uS 
   * @param lightVals 
   * @param newState 
   * @return bool should the mode be cancelled?
   */
  bool setState(uint64_t utcTimestamp_uS, LightStateStruct& lightVals, bool newState) override {
    if(newState == lightVals.state){
      duty_t targetVals[nChannels+1]; _interpClass->getFinalValues(targetVals);
      // this should only happen due to a race condition between a slow network and a button press
      return false;
    }

    if(isActive){
      return true;
    }
    
    lightVals.state = newState;

    if(newState == false){
      _interpClass->getFinalValues(lightVals.values); // yeat on passed the colour interpolation
      return false;
    }

    // if turning on:

    
    // previous brightness or minimum, whichever's bigger
    duty_t newBrightness = lightVals.values[0] < _minOnBrightness
                          ? _minOnBrightness
                          : lightVals.values[0];
    lightVals.values[0] = _minOnBrightness; // set current brightness to min for a soft on

    _interpClass->setFinalBrightness(newBrightness, lightVals, utcTimestamp_uS, _softChangeWindow_S);
    updateLightVals(utcTimestamp_uS, lightVals);
    return false;
  }

  /**
   * @brief Get the target values
   * 
   * @param vals[] array of size nChannels+1
   * @param utcTimestamp_uS 
   * @param lightVals 
   */
  void getTargetVals(duty_t vals[nChannels+1], uint64_t utcTimestamp_uS, LightStateStruct& lightVals){
    updateLightVals(utcTimestamp_uS, lightVals);
    _interpClass->getFinalValues(vals);
  }

  duty_t getTargetBrightness() override {return _interpClass->getFinalBrightness();}

  void timeAdjust(int64_t adjustment_uS) override {
    _interpClass->timeAdjust(adjustment_uS);
  }

  void changeSoftChangeWindow(uint8_t newWindow_S, uint64_t utcTimestamp_uS, LightStateStruct& lightVals) override {
    _softChangeWindow_S = newWindow_S;
    updateLightVals(utcTimestamp_uS, lightVals);
  }

  void changeMinOnBrightness(duty_t newMinBrightness, uint64_t utcTimestamp_uS, LightStateStruct& lightVals) override {
    _minOnBrightness = newMinBrightness;
    duty_t activeMin = isActive && modeData->minBrightness > _minOnBrightness;
    _minSettableBrightness = activeMin ? modeData->minBrightness : _minOnBrightness;
    updateLightVals(utcTimestamp_uS, lightVals);
  }
};


#endif