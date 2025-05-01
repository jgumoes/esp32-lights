#ifndef _LIGHT_MODES_H_
#define _LIGHT_MODES_H_

#include "../DeviceTime/include/DeviceTime.h"
#include "lightDefines.h"
// #include <PrintDebug.h>

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

/**
 * @brief interpolates brightness and colours between starting values and final values across a time window. seperates the colour and brightness interpolations, but this is optional.
 * 
 */
class InterpolationClass{
  private:
    uint64_t _colourStartTime_uS = 0;     // UTC start time of colour interpolation
    uint64_t _brightnessStartTime_uS = 0; // UTC start time of brightness interpolation
    uint64_t _colourWindow_uS = 0;        // windows need to be 64 bits to allow conversion from rates
    uint64_t _brightnessWindow_uS = 0;
    duty_t _startingValues[nChannels+1];
    duty_t _finalValues[nChannels+1];
    uint8_t _isDone[nChannels+1];

    /**
     * @brief performs a 1D interpolation of a given value.
     * TODO: make this function pure and global
     * 
     * @param index 
     * @param lightVals
     * @param dT 
     * @param window_uS 
     */
    void _interpolate(uint8_t index, LightStateStruct& lightVals, uint64_t dT, uint64_t window_uS){
      // TODO: add  diff == 0 to expression
      if(
        window_uS == 0
        || dT >= window_uS
        || lightVals.values[index] == _finalValues[index]
      ){
        lightVals.values[index] = _finalValues[index];
        _isDone[index] = true;
        return;
      }
      if(_isDone[index] || dT == 0){
        return;
      }
      // if(dT > window_uS){dT = window_uS;}

      // 1000 = 2^3 * 5^3
      // todo: (k[i] + diffB[i] * dT)/window, maybe limit to millisecond floor
      // TODO: create constants on initialisation
      int16_t diff = _finalValues[index] - _startingValues[index];
      int64_t b0w = _startingValues[index] * window_uS;
      int64_t k = b0w + (window_uS>>1); // folding negative change into the always-positive top means window/2 is always added

      int32_t newValue = (k + diff*dT)/window_uS;
      if(
        (diff > 0 && newValue > _finalValues[index])
        || (diff < 0 && newValue < _finalValues[index])
      ){
        newValue = _finalValues[index];
        _isDone[index] = true;
        // reset the startingValue when done;
        _startingValues[index] = newValue;
      }
      lightVals.values[index] = static_cast<duty_t>(newValue);
    }

  public:
    InterpolationClass(){
      for(uint8_t i = 0; i < nChannels+1; i++){
        _startingValues[i] = 0;
        _finalValues[i] = 0;
        _isDone[i] = 0;
      }
    }
  
    /**
     * @brief update the light values
     * 
     * @param lightVals address of the lightVals struct 
     * @param utcTimestamp_uS 
     * @return true if interpolation has finished;
     * @return false if interpolation needs to continue
     */
    bool update(LightStateStruct& lightVals, uint64_t utcTimestamp_uS){
      _interpolate(0, lightVals, utcTimestamp_uS-_brightnessStartTime_uS, _brightnessWindow_uS);
      bool finished = _isDone[0];

      for(uint8_t i = 1; i < nChannels+1; i++){
        _interpolate(i, lightVals, utcTimestamp_uS-_colourStartTime_uS, _colourWindow_uS);
        finished &= _isDone[i];
      }
      return finished;
    }
  
    /**
     * @brief initialise the interpolation instance, returns true if an interpolation is needed. the hope is a single, static quickChange instance will be shared by every mode instead of re-constructing every time the mode changes. this also allows the same instance to be used for soft changes without calling this function (just set the brightness)
     * 
     * @param utcTimestamp_uS 
     * @param window_uS 
     * @param lightVals address of the current values struct. used as starting vals
     * @param finalValues[] nChannels+1 
     * @return true if interpolation is already finished;
     * @return false if interpolation needs to be active
     * @returns if the interpolation is finished
     */
    bool initialise(uint64_t utcTimestamp_uS, uint64_t window_uS, LightStateStruct& lightVals, duty_t finalValues[]){
      bool notNeeded = true;
      _colourStartTime_uS = utcTimestamp_uS;
      _brightnessStartTime_uS = utcTimestamp_uS;
      _colourWindow_uS = window_uS;
      _brightnessWindow_uS = window_uS;
      for(uint8_t i = 0; i < nChannels+1; i++){
        _finalValues[i] = finalValues[i];
        // _isDone[i] = (window_uS == 0) && notNeeded;
        _isDone[i] = (window_uS == 0) || lightVals.values[i] == finalValues[i];
        _startingValues[i] = _isDone[i] ? finalValues[i] : lightVals.values[i];
        notNeeded &= _isDone[i];
      }

      return notNeeded;
    };

    /**
     * @brief set a new final brightness.
     * 
     * @param finalBrightness new final brightness value
     * @param lightVals address of the current values struct. current values will be used a starting values
     * @param utcTimestamp_uS utc timestamp in microseconds
     * @param softChangeWindow_S softChange window in seconds
     */
    void setFinalBrightness(duty_t finalBrightness, LightStateStruct& lightVals, uint64_t utcTimestamp_uS, uint8_t softChangeWindow_S){
      if(utcTimestamp_uS < _colourStartTime_uS){return;} // TODO: this should return an error type

      _startingValues[0] = lightVals.values[0];
      _finalValues[0] = finalBrightness;
      _brightnessStartTime_uS = utcTimestamp_uS;
      _brightnessWindow_uS = softChangeWindow_S * secondsToMicros;

      _isDone[0] = false;
    };

    duty_t getFinalBrightness(){return _finalValues[0];}

    void getFinalValues(duty_t vals[nChannels+1]){
      memcpy(vals, _finalValues, nChannels + 1);
    }

    bool isFinished(){
      bool finished = true;
      for(uint8_t i = 0; i < nChannels + 1; i++){
        finished &= _isDone[i];
      }
      return finished;
    }
    
    void timeAdjust(int64_t adjustment_uS){
      _colourStartTime_uS += adjustment_uS;
      _brightnessStartTime_uS += adjustment_uS;
    }
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
  virtual void setBrightness(uint64_t utcTimestamp_uS, LightStateStruct& lightVals, duty_t brightness, bool softChange) = 0;

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
};

class ConstantBrightnessMode : public ModalStrategyInterface
{
private:
  std::shared_ptr<InterpolationClass> _interpClass;
  std::shared_ptr<ConfigManagerClass> _configs;

public:
  const modeUUID modeID;
  const bool isActive;
  const ModeTypes type = ModeTypes::constantBrightness;

  /**
   * @brief Construct a new Constant Brightness Mode object
   * 
   * @param currentTime_uS 
   * @param triggerTime_uS 
   * @param modeData[modePacketSize] array of size modePacketSize
   * @param interpClass
   * @param currentVals 
   * @param isActive 
   * @param configs 
   */
  ConstantBrightnessMode(
    uint64_t currentTime_uS,
    uint64_t triggerTime_uS,
    duty_t modeData[modePacketSize],
    std::shared_ptr<InterpolationClass> interpClass,
    LightStateStruct& currentVals,
    bool isActive,
    std::shared_ptr<ConfigManagerClass> configs
  ) : ModalStrategyInterface(),
      modeID(modeData[0]),
      _interpClass(interpClass),
      isActive(isActive),
      _configs(configs)
  {
    uint64_t utcStartTime_uS = currentTime_uS;

    // fill target colour vals
    duty_t targetValues[nChannels+1] = {_interpClass->getFinalBrightness()};
    for(uint8_t i = 0; i < nChannels; i++){
      targetValues[i+1] = modeData[i+2];
    }
    duty_t minOnBrightness = _configs->getModalConfigs().minOnBrightness;

    // set current vals to target vals if lights are off
    if(currentVals.state == 0 || currentVals.values[0] < minOnBrightness){
      memcpy(currentVals.values, targetValues, nChannels+1);
    }

    // force on default brightness if brightness is under default
    if(isActive){
      duty_t defaultOnBrightness = _configs->getModalConfigs().defaultOnBrightness;
      currentVals.state = true;
      // force current vals to min on brightness
      if(currentVals.values[0] < minOnBrightness){currentVals.values[0] = minOnBrightness;}
      targetValues[0] = currentVals.values[0] <= defaultOnBrightness
                        ? defaultOnBrightness
                        : currentVals.values[0];
    }
    
    uint64_t window = _configs->getModalConfigs().softChangeWindow * secondsToMicros;
    _interpClass->initialise(utcStartTime_uS, window, currentVals, targetValues);
    updateLightVals(utcStartTime_uS, currentVals);
  };

  void updateLightVals(uint64_t utcTimestamp_uS, LightStateStruct& lightVals) override {
    _interpClass->update(lightVals, utcTimestamp_uS);

    if(lightVals.values[0] < _configs->getModalConfigs().minOnBrightness){
      lightVals.state = false;
      lightVals.values[0] = 0;
      _interpClass->getFinalValues(lightVals.values); // TODO: will if statements make this more efficient?
    }
    return;
  }

  void setBrightness(uint64_t utcTimestamp_uS, LightStateStruct& lightVals, duty_t brightness, bool softChange) override {
    uint8_t window = softChange ? _configs->getModalConfigs().softChangeWindow : 0;

    duty_t defaultBrightness = _configs->getModalConfigs().defaultOnBrightness;
    if(isActive && (brightness < defaultBrightness)){brightness = defaultBrightness;} // TODO: min values should be set as pointers in the initialisation. for active mode, minOnBrightness should point to defaultBrightness
    
    duty_t minOnBrightness = _configs->getModalConfigs().minOnBrightness;
    if(brightness < minOnBrightness){
      brightness = (minOnBrightness-1) * softChange; // minB - 1 is identical to 0
    }
    else{
      // if new brightness is on, make sure state is on and current brightness is at least min
      lightVals.state = true; // TODO: is this redundant? can it be removed? does it duplicate the final line of this method?
      if(lightVals.values[0] < minOnBrightness){
        lightVals.values[0] = minOnBrightness;
      }
    }
    
    _interpClass->setFinalBrightness(
      brightness, lightVals, utcTimestamp_uS, window
    );
    updateLightVals(utcTimestamp_uS, lightVals);
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
      _interpClass->getFinalValues(lightVals.values); // yeat on passed the colour interpolation
      return false;
    }

    // if turning on:

    
    // previous brightness or default minimum, whichever's bigger
    duty_t minB = _configs->getModalConfigs().minOnBrightness;
    duty_t defaultOnBrightness = _configs->getModalConfigs().defaultOnBrightness;
    duty_t newBrightness = lightVals.values[0] < minB
                          ? defaultOnBrightness
                          : lightVals.values[0];
    lightVals.values[0] = minB; // set current brightness to min for a soft on

    uint8_t window_s = _configs->getModalConfigs().softChangeWindow;
    _interpClass->setFinalBrightness(newBrightness, lightVals, utcTimestamp_uS, window_s);
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
};


#endif