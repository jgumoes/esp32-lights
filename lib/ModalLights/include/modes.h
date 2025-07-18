#ifndef _LIGHT_MODES_H_
#define _LIGHT_MODES_H_

// #include "../DeviceTime/include/DeviceTime.h"
#include "DeviceTime.h"
#include "lightDefines.h"
#include "interpolationClass.h"

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
  virtual void timeAdjust(const TimeUpdateStruct& timeUpdates) = 0;

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

  /**
   * @brief change the defaultOnBrightness config setting. there shouldn't be any logic.
   * 
   * @param newDefaultOnBrightness 
   */
  virtual void changeDefaultOnBrightness(duty_t newDefaultOnBrightness) = 0;
};

class ConstantBrightnessMode : public ModalStrategyInterface
{
private:
  std::shared_ptr<ModeInterpolationClass<nChannels>> _interpClass;

  // TODO: replace with reference to configs struct
  duty_t _softChangeWindow_S;
  duty_t _minOnBrightness;
  duty_t _defaultOnBrightness;
  
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
    std::shared_ptr<ModeInterpolationClass<nChannels>> interpClass,
    LightStateStruct& currentVals,
    bool isActive,
    const ModalConfigsStruct& configs
  ) : ModalStrategyInterface(),
      modeData(modeDataStruct),
      _interpClass(interpClass),
      isActive(isActive),
      _softChangeWindow_S(configs.softChangeWindow),
      _minOnBrightness(configs.minOnBrightness),
      _defaultOnBrightness(configs.defaultOnBrightness)
  {
    uint64_t utcStartTime_uS = currentTime_uS;
    _minSettableBrightness = _minOnBrightness;

    // fill target colour vals
    memcpy(_interpClass->colours.targetVals, modeData->endColourRatios, nChannels);

    // set current vals to target vals if lights are off
    if(currentVals.state == 0 || currentVals.values[0] < _minOnBrightness){
      _interpClass->getTargetVals(currentVals.values);
    }

    duty_t *targetB = _interpClass->brightness.targetVals;
    
    // if mode is active, force on default brightness if brightness is under default
    if(isActive){
      const duty_t modeMinB = modeData->minBrightness;
      _minSettableBrightness = modeMinB > _minOnBrightness ? modeMinB : _minOnBrightness;
      currentVals.state = true;
      // force current vals to min on brightness
      if(currentVals.values[0] < _minOnBrightness){currentVals.values[0] = _minOnBrightness;}
      *targetB = currentVals.values[0] <= _minSettableBrightness
                        ? _minSettableBrightness
                        : currentVals.values[0];
      if(*targetB < _defaultOnBrightness){
        *targetB = _defaultOnBrightness;
      }
    }
    uint64_t window = _softChangeWindow_S * secondsToMicros;
    _interpClass->setInitialVals(currentVals.values);
    _interpClass->rebuildInterpConstants_window(utcStartTime_uS, window);
    updateLightVals(utcStartTime_uS, currentVals);
  };

  void updateLightVals(uint64_t utcTimestamp_uS, LightStateStruct& lightVals) override {
    _interpClass->findNextValues(lightVals.values, utcTimestamp_uS);

    if(lightVals.values[0] < _minOnBrightness){
      lightVals.state = false;
      _interpClass->setTargetBrightness(0);
      _interpClass->endInterpolation(lightVals.values);
    }
    return;
  }

  duty_t setBrightness(uint64_t utcTimestamp_uS, LightStateStruct& lightVals, duty_t brightness, bool softChange) override {
    uint8_t window = softChange ? _softChangeWindow_S : 0;

    if(isActive && (brightness < _minSettableBrightness)){brightness = _minSettableBrightness;}
    
    if(brightness < _minOnBrightness){
      brightness = (_minOnBrightness-1) * softChange; // minB - 1 is identical to 0
    }
    else{
      // if new brightness is on, make sure state is on and current brightness is at least min
      lightVals.state = true; // TODO: is this redundant? can it be removed? does it duplicate the final line of this method? should it be removed? does this line introduce functionality that wouldn't be implemented later in this function? does leaving it be infact constitute a benefit towards the functionality of this program?
      if(lightVals.values[0] < _minOnBrightness){
        lightVals.values[0] = _minOnBrightness;
      }
    }
    
    _interpClass->newBrightnessVal_window(
      utcTimestamp_uS, window*secondsToMicros, lightVals.values[0], brightness
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
      // this should only happen due to a race condition between a slow network and a button press
      return false;
    }

    if(isActive){
      return true;
    }
    
    lightVals.state = newState;

    if(newState == false){
      _interpClass->endInterpolation(lightVals.values); // yeat on passed the colour interpolation
      return false;
    }

    // if turning on:

    // set to defaultOnBrightness, but only if it's valid
    duty_t newBrightness = _defaultOnBrightness <= _minOnBrightness
                          ? max(lightVals.values[0], _minOnBrightness)
                          : _defaultOnBrightness;

    _interpClass->newBrightnessVal_window(utcTimestamp_uS, _softChangeWindow_S*secondsToMicros, _minOnBrightness, newBrightness);
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
    _interpClass->getTargetVals(vals);
  }

  duty_t getTargetBrightness() override {return _interpClass->getTargetBrightness();}

  void timeAdjust(const TimeUpdateStruct& timeUpdates) override {
    _interpClass->notification(timeUpdates);
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

  void changeDefaultOnBrightness(duty_t newDefaultOnBrightness){
    _defaultOnBrightness = newDefaultOnBrightness;
  }
};


#endif