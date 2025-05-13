#ifndef __INTERPOLATION_CLASS__
#define __INTERPOLATION_CLASS__

#include <Arduino.h>

typedef uint8_t duty_t;

typedef uint8_t isDone_t;

namespace IsDoneBitFlags{
  constexpr isDone_t none = 0;
  constexpr isDone_t brightness = 1;
  constexpr isDone_t colours = 2;
  constexpr isDone_t both = 3;
};

/*
handles interpolation, both window and rate. calculates constants when interpolation is initiated. everything is out in the open and pretty exposed, so don't go abusing trust.
TODO: resizeWindow method

mathematically,
  b(t) = b(0) + (top/bottom)*(t - t0)

with rounding,
  b(t) = (b(0)*bottom + top*(t-t0) + (bottom/2))/bottom
       = (k + top*(t-t0))/bottom
*/
template <uint8_t nColours>
class InterpolationClass{
private:
  isDone_t _isDone = IsDoneBitFlags::both;

  isDone_t _setBrightnessDoneFlag(bool finished){
    _isDone = (_isDone & IsDoneBitFlags::colours) + finished;
    return _isDone;
  }

public:
  // static const uint8_t nColours = 3;  // intellisense struggles picking up errors for templated classes, so this is here to making writing and modifying easier

  // set values
  uint64_t t0_uS[2] = {0, 0};        // start times, [0] = brightness start time, [1] = colours start time
  duty_t initialVals[nColours+1];  // [0] = brightness, [1:] = colours
  duty_t targetVals[nColours+1];   // [0] = brightness, [1:] = colours
  
  // calculated values
  uint64_t t1_uS[2] = {0, 0};        // end times, window_uS + t0, or (r_uS * max(abs(dB))) + t0
  int16_t top[nColours+1];  // for window: b1 - b0; for rate: +/- 1 or 0
  uint64_t bottom[2];       // window_uS or rate_us. [0] = brightness, [1] = colours start time
  uint64_t k[nColours+1];   // rounding constant = b0*bottom + (bottom/2)

  InterpolationClass(){
    for(uint8_t i = 0; i < nColours + 1; i++){
      initialVals[i] = 0;
      targetVals[i] = 0;
    }
  };

  /**
   * @brief returns the bitflags for if the interpolation was finished in the last mutating method call.
   * 
   * @return isDone_t isDone bitflag matches IsDoneBitFlags
   */
  isDone_t isDone(){
    return _isDone;
  }
  
  /**
   * @brief change the start and end times using utcTimeChange_uS from the TimeUpdateStruct
   * 
   * @param utcTimeChange_uS 
   */
  void adjustTime(const int64_t& utcTimeChange_uS){
    t0_uS[0] += utcTimeChange_uS;
    t0_uS[1] += utcTimeChange_uS;
    t1_uS[0] += utcTimeChange_uS;
    t1_uS[1] += utcTimeChange_uS;
  }

  /**
   * @brief copies targetVals to lightVals and initialVals, and ends the interpolations
   * 
   * @param lightValues 
   * @return isDone_t 
   */
  isDone_t endInterpolation(duty_t lightValues[nColours+1]);

  /**
   * @brief calculates the brightness value at a given time, for a specified index. guards against time and value overflows. doesn't mutate any values
   * 
   * @param utcTimestamp_uS utc timestamp in microseconds
   * @param index 0 == brightness, >0 is a colour
   * @return duty_t the new value
   */
  duty_t interpolateValue(const uint64_t& utcTimestamp_uS, const uint8_t index);

  /**
   * @brief interpolates the values at a given timestamp, and sets currentVals. currentVals is a reference to a c style array, so you could write the target vals of another InterpolationClass instance
   * 
   * @param currentVals array of values to mutate
   * @param utcTimestamp_uS 
   * @return isDone_t isDone bitflag matches IsDoneBitFlags
   */
  isDone_t findNextValues(duty_t currentVals[nColours+1], const uint64_t& utcTimestamp_uS);

  /**
   * @brief set the brightness interpolation constants for a new window interpolation. initialVals and targetVals must already be set
   * 
   * @param utcTimestamp_us 
   * @param window_uS 
   * @return isDone_t isDone bitflag matches IsDoneBitFlags
   */
  isDone_t newBrightnessInterp_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS);

  /**
   * @brief sets a new window interpolation for brightness, using the currentVal as the initial brightness and newVal as the target
   * 
   * @param utcTimestamp_us 
   * @param window_uS 
   * @param currentVal the brightness value right now
   * @param newVal the new target brightness
   * @return isDone_t 
   */
  isDone_t newBrightnessVal_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS, const duty_t currentVal, const duty_t newVal);
  
  /**
   * @brief set the brightness and colour interpolation constants for a new window interpolation. initialVals and targetVals must already be set
   * 
   * @param utcTimestamp_us 
   * @param window_uS 
   * @return isDone_t isDone bitflag matches IsDoneBitFlags
   */
  isDone_t rebuildInterpConstants_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS);

  /**
   * @brief starts a new window interpolation. initialVals get set before target, so initial = this.targetVals should work fine
   * 
   * @param utcTimestamp_us 
   * @param window_uS 
   * @param initial 
   * @param target 
   * @return isDone_t 
   */
  isDone_t newInterp_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS, const duty_t initial[nColours+1], const duty_t target[nColours+1]);

  /**
   * @brief set the brightness interpolation constants for a new rate interpolation. initialVals and targetVals must already be set
   * 
   * @param utcTimestamp_us 
   * @param rate_uS 
   * @return isDone_t isDone bitflag matches IsDoneBitFlags
   */
  isDone_t newBrightnessInterp_rate(const uint64_t& utcTimestamp_us, const uint64_t rate_uS);


  isDone_t rebuildInterpConstants_rate(const uint64_t& utcTimestamp_us, const uint64_t rate_uS);
};

template <uint8_t nColours>
inline duty_t InterpolationClass<nColours>::interpolateValue(const uint64_t& utcTimestamp_uS, const uint8_t index){
  const bool isC = (index != 0);
  const uint64_t t0 = t0_uS[isC];
  
  if(
    (utcTimestamp_uS >= t1_uS[isC])
    || (bottom[isC] == 0)
    || (initialVals[index] == targetVals[index])
  ){
    // bottom == 0 when window or rate == 0 (i.e. change is instant or infinite)
    return targetVals[index];
  }
  if(
    (utcTimestamp_uS <= t0)
    || (top[index] == 0)
  ){
    // top == 0 when b1 == b0
    return initialVals[index];
  }
  // for uint8_t rate, max newValue should be 511. for window, guards should have caught values outside of duty_t range
  int16_t newValue = (k[index] + ((utcTimestamp_uS - t0) * top[index])) / bottom[isC];

  /*
  should be equivalent to:
    (increasing && newVal > target)
    || (!increasing && newVal < target)
  */
  if(
    (top[index] > 0) == (newValue > targetVals[index])
  ){
    return targetVals[index];
  }
  return static_cast<duty_t>(newValue);
}

template <uint8_t nColours>
inline isDone_t InterpolationClass<nColours>::findNextValues(duty_t currentVals[nColours+1], const uint64_t& utcTimestamp_uS){
  _isDone = 1;

  for(uint8_t c = 1; c < nColours + 1; c++){
    currentVals[c] = interpolateValue(utcTimestamp_uS, c);
    _isDone &= (currentVals[c] == targetVals[c]);
  }
  
  _isDone = (_isDone << 1);
  
  currentVals[0] = interpolateValue(utcTimestamp_uS, 0);
  _isDone |= (currentVals[0] == targetVals[0]);
  return _isDone;
}

template <uint8_t nColours>
inline isDone_t InterpolationClass<nColours>::newBrightnessInterp_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS){
  t0_uS[0] = utcTimestamp_us;
  t1_uS[0] = utcTimestamp_us + window_uS;
  top[0] = targetVals[0] - initialVals[0];
  bottom[0] = window_uS;
  k[0] = (initialVals[0] * window_uS) + (window_uS/2);
  return _setBrightnessDoneFlag(top[0] == 0);
}


template <uint8_t nColours>
inline isDone_t InterpolationClass<nColours>::newBrightnessVal_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS, const duty_t currentVal, const duty_t newVal){
  initialVals[0] = currentVal;
  targetVals[0] = newVal;
  return newBrightnessInterp_window(utcTimestamp_us, window_uS);
}

template <uint8_t nColours>
inline isDone_t InterpolationClass<nColours>::endInterpolation(duty_t lightValues[nColours + 1])
{
  for(uint8_t i = 0; i < nColours+1; i++){
    lightValues[i] = targetVals[i];
    initialVals[i] = targetVals[i];
    top[i] = 0;
  }
  _isDone = IsDoneBitFlags::both;
  return _isDone;
}

template <uint8_t nColours>
inline isDone_t InterpolationClass<nColours>::rebuildInterpConstants_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS){
  t0_uS[1] = utcTimestamp_us;
  t1_uS[1] = utcTimestamp_us + window_uS;
  bottom[1] = window_uS;

  _isDone = 1;
  for(uint8_t c = 1; c < nColours + 1; c++){
    top[c] = targetVals[c] - initialVals[c];
    _isDone &= top[c] == 0;
    k[c] = (initialVals[c] * window_uS) + (window_uS/2);
  }
  _isDone = (_isDone << 1);
  return newBrightnessInterp_window(utcTimestamp_us, window_uS);
}

template <uint8_t nColours>
inline isDone_t InterpolationClass<nColours>::newInterp_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS, const duty_t initial[nColours+1], const duty_t target[nColours+1]){
  t0_uS[1] = utcTimestamp_us;
  t1_uS[1] = utcTimestamp_us + window_uS;
  bottom[1] = window_uS;
  _isDone = 1;
  for(uint8_t c = 1; c < nColours + 1; c++){
    initialVals[c] = initial[c];
    targetVals[c] = target[c];
    top[c] = targetVals[c] - initialVals[c];
    _isDone &= top[c] == 0;
    k[c] = (initialVals[c] * window_uS) + (window_uS/2);
  }
  _isDone = (_isDone << 1);
  
  newBrightnessVal_window(utcTimestamp_us, window_uS, initial[0], target[0]);
  return _isDone;
}

template <uint8_t nColours>
inline isDone_t InterpolationClass<nColours>::newBrightnessInterp_rate(const uint64_t& utcTimestamp_us, const uint64_t rate_uS){
  t0_uS[0] = utcTimestamp_us;
  const int16_t dB = targetVals[0] - initialVals[0];
  t1_uS[0] = utcTimestamp_us + (abs(dB) * rate_uS);
  top[0] = dB < 0 ? -1 : dB > 0;
  bottom[0] = rate_uS;
  k[0] = (initialVals[0] * rate_uS) + (rate_uS/2);
  return _setBrightnessDoneFlag(dB == 0);
}

template <uint8_t nColours>
inline isDone_t InterpolationClass<nColours>::rebuildInterpConstants_rate(const uint64_t& utcTimestamp_us, const uint64_t rate_uS){
  t0_uS[1] = utcTimestamp_us;
  bottom[1] = rate_uS;
  
  _isDone = 1;
  uint8_t max_dC = 0;
  for(uint8_t c = 1; c < nColours + 1; c++){
    int16_t dC = targetVals[c] - initialVals[c];
    top[c] = dC < 0 ? -1 : dC > 0;
    const duty_t abs_dC = abs(dC);
    if(abs_dC > max_dC){max_dC == abs_dC;}
    _isDone &= dC == 0;
    k[c] = (initialVals[c] * rate_uS) + (rate_uS/2);
  }
  t1_uS[1] = utcTimestamp_us + (max_dC * rate_uS);

  _isDone = (_isDone << 1);
  return newBrightnessInterp_rate(utcTimestamp_us, rate_uS);
}

#endif