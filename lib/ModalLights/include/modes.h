#ifndef _LIGHT_MODES_H_
#define _LIGHT_MODES_H_

#include "../DeviceTime/include/DeviceTime.h"
#include "lightDefines.h"
#include <PrintDebug.h>
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
    uint64_t _colourStartTime_uS;     // UTC start time of colour interpolation
    uint64_t _brightnessStartTime_uS; // UTC start time of brightness interpolation
    uint64_t _colourWindow_uS;        // windows need to be 64 bits to allow conversion from rates
    uint64_t _brightnessWindow_uS;
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
      // int32_t newValue = _startingValues[index] + (((diff * dT) + (window_uS >> 1))/window_uS);
      // int32_t newValue = _startingValues[index] + interpDivide((int64_t)(dT*diff), (int64_t)(window_uS));
      // int32_t newValue = _startingValues[index] + roundingDivide((int64_t)(dT*diff), (int64_t)(window_uS));

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
     */
    bool initialise(uint64_t utcTimestamp_uS, uint64_t window_uS, LightStateStruct& lightVals, duty_t finalValues[]){
      // initialise the values even if not needed. it might be used for a quick change
      bool notNeeded = !isQuickChangeNeeded(lightVals, finalValues);

      _colourStartTime_uS = utcTimestamp_uS;
      _brightnessStartTime_uS = utcTimestamp_uS;
      _colourWindow_uS = window_uS;
      _brightnessWindow_uS = window_uS;
      for(uint8_t i = 0; i < nChannels+1; i++){
        _startingValues[i] = lightVals.values[i];
        _finalValues[i] = finalValues[i];
        _isDone[i] = (window_uS == 0) && notNeeded;
      }
      return notNeeded;
    };

    /**
     * @brief set a new final brightness, then calls update(). if forceNewInterpolation is true, it'll use the softChangeWindow_S as the new interpolation window. otherwise, it'll use either the softChangeWindow_S or the current interpolation, whichever ends later. flashing modes will want forceNewInterpolation = false;
     * 
     * @param finalBrightness new final brightness value
     * @param lightVals address of the current values struct. current values will be used a starting values
     * @param utcTimestamp_uS utc timestamp in microseconds
     * @param softChangeWindow_S softChange window in seconds
     * @param forceNewInterpolation if true, ignores the colour change window
     * @return true if the interpolation is finished
     */
    bool setFinalBrightness(duty_t finalBrightness, LightStateStruct& lightVals, uint64_t utcTimestamp_uS, uint8_t softChangeWindow_S, bool forceNewInterpolation){
      if(utcTimestamp_uS < _colourStartTime_uS){return false;} // TODO: this should return an error type

      _startingValues[0] = lightVals.values[0];
      _finalValues[0] = finalBrightness;
      _brightnessStartTime_uS = utcTimestamp_uS;

      uint64_t softWindow_uS = softChangeWindow_S * secondsToMicros;
      uint64_t colourEndTime_uS = _colourWindow_uS + _colourStartTime_uS;
      // if there's a colour change that'll end after the soft change, use that endtime instead (unless forced off)
      _brightnessWindow_uS = ((utcTimestamp_uS + softWindow_uS) >= (colourEndTime_uS * !forceNewInterpolation))
                            ? softWindow_uS
                            : colourEndTime_uS - _brightnessStartTime_uS;

      // // adjust brightness window so it finishes at same time as colour window
      // _brightnessWindow_uS = _colourWindow_uS + _colourStartTime_uS - _brightnessStartTime_uS;

      // // use softChange if it would finish later than brightness window
      // if((softChangeWindow_S * secondsToMicros) > _brightnessWindow_uS){
      //   _brightnessWindow_uS = softChangeWindow_S * secondsToMicros;
      // }
      // _isDone[0] = _brightnessWindow_uS == 0;
      _isDone[0] = false;
      return update(lightVals, utcTimestamp_uS);
    };

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
  // ModalStrategyInterface(uint64_t utcStartTime_uS): _utcStartTime_uS(utcStartTime_uS){};

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
  virtual void getTargetVals(duty_t vals[], uint64_t utcTimestamp_uS, LightStateStruct& lightVals) = 0;

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
bool _quickChangeActive;
duty_t _targetValues[nChannels+1];  // TODO: replace with accessing finalValues from InterpolationClass
std::shared_ptr<InterpolationClass> _quickChangeClass = std::make_shared<InterpolationClass>(); // TODO: make this static in virtual class
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
   * @param modeData[] array of size modePacketSize
   * @param previousVals[] array of size nChannels+1
   * @param currentVals 
   * @param isActive 
   * @param configs 
   */
  ConstantBrightnessMode(
    uint64_t currentTime_uS,
    uint64_t triggerTime_uS,
    duty_t modeData[modePacketSize],
    duty_t previousVals[],
    LightStateStruct& currentVals,
    bool isActive,
    std::shared_ptr<ConfigManagerClass> configs
  ) : ModalStrategyInterface(),
      modeID(modeData[0]),
      isActive(isActive),
      _configs(configs)
  {
    uint64_t utcStartTime_uS = currentTime_uS;

    // force on if active and off
    duty_t defaultOnBrightness = _configs->getModalConfigs().defaultOnBrightness;
    duty_t minOnBrightness = _configs->getModalConfigs().minOnBrightness;
    if(isActive && (currentVals.state * previousVals[0] < defaultOnBrightness)){
      currentVals.state = true;
      if(currentVals.values[0] < minOnBrightness){currentVals.values[0] = minOnBrightness;}
      previousVals[0] = previousVals[0] < defaultOnBrightness
                        ? defaultOnBrightness
                        : currentVals.values[0];
    }
    
    // _targetValues[0] = previousVals[0] * currentVals.state;
    _targetValues[0] = previousVals[0];
    for(uint8_t i = 0; i < nChannels; i++){
      _targetValues[i+1] = modeData[i+2];
    }
    if(currentVals.state == 0){
      memcpy(currentVals.values, _targetValues, nChannels+1);
    }

    uint64_t window = _configs->getModalConfigs().changeoverWindow * secondsToMicros;
    // if(currentVals.state){
      _quickChangeActive = !_quickChangeClass->initialise(utcStartTime_uS, window, currentVals, _targetValues);
    // }

  };

  void updateLightVals(uint64_t utcTimestamp_uS, LightStateStruct& lightVals) override {
    PrintDebug_function("updateLightVals");
    PrintDebug_UINT8_array("current vals before update", lightVals.values, nChannels+1);
    if(_quickChangeActive){
      _quickChangeActive = !_quickChangeClass->update(lightVals, utcTimestamp_uS);
    }
    // if lights are set to on but brightness is currently 0
    const duty_t minOnBrightness = _configs->getModalConfigs().minOnBrightness;
    lightVals.state = lightVals.values[0] > 0;
    // if(
    //   lightVals.state
    //   && lightVals.values[0] < minOnBrightness
    //   && _targetValues[0] > 0
    // ){
    //   lightVals.values[0] = minOnBrightness;
    // }
    PrintDebug_UINT8_array("target vals", _targetValues, nChannels+1);
    PrintDebug_UINT8_array("current vals after update", lightVals.values, nChannels+1);
    PrintDebug_endFunction;
    return;
  }

  void setBrightness(uint64_t utcTimestamp_uS, LightStateStruct& lightVals, duty_t brightness, bool softChange) override {
    uint8_t window = softChange ? _configs->getModalConfigs().softChangeWindow : 0;

    duty_t defaultBrightness = _configs->getModalConfigs().defaultOnBrightness;
    if(isActive && (brightness < defaultBrightness)){brightness = defaultBrightness;} // TODO: min values should be set as pointers in the initialisation. for active mode, minOnBrightness should point to defaultBrightness
    duty_t minOnBrightness = _configs->getModalConfigs().minOnBrightness;

    // if(brightness == 0 && window == 0){lightVals.state = false;}
    if(brightness > 0){
      brightness = brightness >= minOnBrightness ? brightness : minOnBrightness;
      lightVals.state = true;
      lightVals.values[0] = lightVals.values[0] > 0
                            ? lightVals.values[0]
                            : minOnBrightness;
    }
    
    // if lights have fallen to 0, state should be off
    // if(lightVals.values[0] == 0 && lightVals.state == true){lightVals.values[0] = minOnBrightness;}
    // lightVals.state = ((brightness > 0) || window > 0);
    
    // setting a new brightness during a colour change should force an immediate update
    _targetValues[0] = brightness;
    _quickChangeActive = !_quickChangeClass->setFinalBrightness(
      brightness, lightVals, utcTimestamp_uS, window, true
    );
    lightVals.state = lightVals.values[0] > 0;

    // updateLightVals(utcTimestamp_uS, lightVals);
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
      // disable the quick change if the lights are off
      _quickChangeActive = false;
      for(uint8_t i = 1; i < nChannels+1; i++){
        lightVals.values[i] = _targetValues[i];
      }
      return false;
    }

    // if turning on:

    // previous brightness or default minimum, whichever's bigger
    duty_t defaultOnBrightness = _configs->getModalConfigs().defaultOnBrightness;
    duty_t newBrightness = lightVals.values[0] < defaultOnBrightness
                          ? newBrightness = defaultOnBrightness
                          : lightVals.values[0];
    lightVals.values[0] = _configs->getModalConfigs().minOnBrightness; // set current brightness to min for a soft on
    _targetValues[0] = newBrightness;
    for(uint8_t i = 1; i < nChannels+1; i++){
      // TODO: replace lightVals with end colour ratios
      // TODO: actually, why am I doing this?
      _targetValues[i] = lightVals.values[i];
    }

    // interpolation should be reinitialised, incase a sudden brightness change happens
    _quickChangeActive = !_quickChangeClass->initialise(
      utcTimestamp_uS,_configs->getModalConfigs().softChangeWindow*secondsToMicros, lightVals, _targetValues
    );
    const duty_t minOnBrightness = _configs->getModalConfigs().minOnBrightness;
    if(lightVals.values[0] < minOnBrightness){lightVals.values[0] = minOnBrightness;}
    return false;
  }

  /**
   * @brief Get the target values
   * 
   * @param vals[] array of size nChannels+1
   * @param utcTimestamp_uS 
   * @param lightVals 
   */
  void getTargetVals(duty_t vals[], uint64_t utcTimestamp_uS, LightStateStruct& lightVals){
    updateLightVals(utcTimestamp_uS, lightVals);
    for(uint8_t i = 0; i < nChannels+1; i++){
      vals[i] = _targetValues[i];
    }
  }

  duty_t getTargetBrightness() override {return _targetValues[0];}

  void timeAdjust(int64_t adjustment_uS) override {
    _quickChangeClass->timeAdjust(adjustment_uS);
  }
};


#endif