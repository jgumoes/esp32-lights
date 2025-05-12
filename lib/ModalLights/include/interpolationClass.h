#ifndef __INTERPOLATION_CLASS__
#define __INTERPOLATION_CLASS__

#include "lightDefines.h"
#include "DeviceTime.h"

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


#endif