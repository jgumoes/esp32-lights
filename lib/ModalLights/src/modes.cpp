#include "modes.h"

uint64_t _previousTimestamp;
duty_t _previousBrightness;

uint64_t _initialTime;
duty_t _initialBrightness;  // in bits from 0 to LED_LIGHTS_MAX_DUTY

uint64_t _endTime;
duty_t _endBrightness;

uint64_t _timeDiff; // i.e. the rise (or fall) time
duty_t _brightnessDiff;
int8_t _initialDirection;

duty_t _findBrightness_constantBrightness(uint64_t currentTimestamp){
  return _previousBrightness;
}

duty_t _findBrightness_interpolation(uint64_t currentTimestamp){
  if(currentTimestamp >= _endTime){
    return _endBrightness;  // TODO: adapt to saw-toothed wave
  }
  // TODO: possible recast error? fully test that this can't go further than _endBrightness
  _previousBrightness = _initialBrightness + ((currentTimestamp - _endTime) * _initialDirection * _brightnessDiff) / _timeDiff;
  _previousTimestamp = currentTimestamp;
  return _previousBrightness;
}

float _slope; // i.e. _brightnessDiff / _timeDiff (bits / uS)
duty_t _findBrightness_constRate(uint64_t currentTimestamp){
  if(currentTimestamp >= _endTime){
    return _endBrightness;
  }
  _previousBrightness = _slope * (currentTimestamp - _initialTime) + _initialBrightness;
  return _previousBrightness;
}

duty_t (*_find_brightness_function)(uint64_t) = &_findBrightness_interpolation;
WaveModes _currentWaveMode;

void setLightsToConstRate(
  float rate,
  uint64_t currentTimestamp,
  duty_t finalBrightness
  ){
  _initialTime = currentTimestamp;
  _initialBrightness = getDutyCycleValue();
  _slope = rate;
  _find_brightness_function = _findBrightness_constRate;
}

duty_t findBrightness(uint64_t currentTimestamp){
  return _find_brightness_function(currentTimestamp);
}

duty_t updateBrightness(uint64_t currentTimestamp){
  duty_t brightness = _find_brightness_function(currentTimestamp);
  _previousBrightness = brightness;
  _previousTimestamp = currentTimestamp;
  return brightness;
}

/**
 * @brief Set the Lights Mode. Used for non-repeating non-wave modes.
 * 
 * @param lightMode 
 * @param initialTime 
 * @param currentTime 
 * @param endTime 
 * @param initialBrightness 
 * @param endBrightness 
 */
void setLightsMode(
  LightModes lightMode,
  uint64_t initialTime,
  uint64_t currentTime,
  uint64_t endTime,
  duty_t initialBrightness,
  duty_t endBrightness
  ) {
  _initialTime = initialTime;
  _initialBrightness = initialBrightness;
  _previousBrightness = initialBrightness;
  _endTime = endTime;
  _endBrightness = endBrightness;

  switch (lightMode)
  {
  case constantBrightness:
    _find_brightness_function = _findBrightness_constantBrightness;
    break;
  case interpolation:
    if(endBrightness > initialBrightness){
      _initialDirection = 1;
    }
    else {
      _initialDirection = -1;
    }
      _find_brightness_function = _findBrightness_interpolation;
    break;
  case constRate:
      _timeDiff = endTime - initialTime;
      setLightsToConstRate((_initialBrightness - endBrightness) / _timeDiff, currentTime, endBrightness);
    break;
  
  default:
    _find_brightness_function = _findBrightness_interpolation;
    break;
  }
}