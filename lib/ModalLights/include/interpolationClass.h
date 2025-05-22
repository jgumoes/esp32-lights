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

template <uint8_t nDimensions>
class Interpolator{
private:
  bool _isDone = true;
public:
  // set values
  uint64_t t0_uS = 0;        // start times
  duty_t initialVals[nDimensions];
  duty_t targetVals[nDimensions];
  
  // calculated values
  uint64_t t1_uS = 0;        // end times, window_uS + t0, or (r_uS * max(abs(dB))) + t0
  int16_t top[nDimensions];  // for window: b1 - b0; for rate: +/- 1 or 0
  uint64_t bottom;       // window_uS or rate_us
  uint64_t k[nDimensions];   // rounding constant = b0*bottom + (bottom/2)

  Interpolator(){
    for(uint8_t d = 0; d < nDimensions; d++){
      initialVals[d] = 0;
      targetVals[d] = 0;
    }
  };

  isDone_t isDone(){
    return _isDone;
  };

  void adjustTime(const int64_t& utcTimeChange_uS){
    t0_uS += utcTimeChange_uS;
    t1_uS += utcTimeChange_uS;
  };

  duty_t interpolateValue(const uint64_t& utcTimestamp_uS, const uint8_t index){
    if(
      (utcTimestamp_uS >= t1_uS)
      || (bottom == 0)
      || (initialVals[index] == targetVals[index])
    ){
      // bottom == 0 when window or rate == 0 (i.e. change is instant or infinite)
      return targetVals[index];
    }
    if(
      (utcTimestamp_uS <= t0_uS)
      || (top[index] == 0)
    ){
      // top == 0 when b1 == b0
      return initialVals[index];
    }
    // for uint8_t rate, max newValue should be 511. for window, guards should have caught values outside of duty_t range
    int16_t newValue = (k[index] + ((utcTimestamp_uS - t0_uS) * top[index])) / bottom;

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
  };

  isDone_t findNextValues(duty_t currentVals[nDimensions], const uint64_t& utcTimestamp_uS){
    _isDone = 1;
    for(uint8_t d = 0; d < nDimensions; d++){
      currentVals[d] = interpolateValue(utcTimestamp_uS, d);
      _isDone &= (currentVals[d] == targetVals[d]);
    }
    return _isDone;
  };

  /**
   * @brief set the brightness interpolation constants for a new window interpolation.
   * 
   * @param startTimeUTC_uS start time of the interpolation
   * @param window_uS 
   * @param initial array of initial values
   * @param target array of target values
   * @return isDone_t 
   */
  isDone_t newWindowInterpolation(const uint64_t& startTimeUTC_uS, const uint64_t window_uS, const duty_t initial[nDimensions], const duty_t target[nDimensions]){
    t0_uS = startTimeUTC_uS;
    t1_uS = startTimeUTC_uS + window_uS;
    bottom = window_uS;
    _isDone = 1;
    for(uint8_t d = 0; d < nDimensions; d++){
      initialVals[d] = initial[d];
      targetVals[d] = target[d];
      top[d] = targetVals[d] - initialVals[d];
      _isDone &= top[d] == 0;
      k[d] = (initialVals[d] * window_uS) + (window_uS/2);
    }
    return _isDone;
  };

  /**
   * @brief set the brightness interpolation constants for a new window interpolation. sets all initialVals to initial, and all targetVals to target. basically a convenience method for 1-D interpolations
   * 
   * @param startTimeUTC_uS start time of the interpolation
   * @param window_uS 
   * @param initial initial value
   * @param target target value
   * @return isDone_t 
   */
  isDone_t newWindowInterpolation(const uint64_t& startTimeUTC_uS, const uint64_t window_uS, const duty_t initial, const duty_t target){
    t0_uS = startTimeUTC_uS;
    t1_uS = startTimeUTC_uS + window_uS;
    bottom = window_uS;
    const int16_t topVal = target - initial;
    _isDone = (topVal == 0);
    for(uint8_t d = 0; d < nDimensions; d++){
      initialVals[d] = initial;
      targetVals[d] = target;
      top[d] = topVal;
      k[d] = (initial * window_uS) + (window_uS/2);
    }
    return _isDone;
  };

  /**
   * @brief starts a new window interpolation with the same initialVals and targetVals, unless they've been changed in which case it just starts a new interpolation
   * 
   * @param initialTimeUTC_uS 
   * @param window_uS 
   * @return isDone_t 
   */
  isDone_t restartWindowInterpolation(const uint64_t& initialTimeUTC_uS, const uint64_t window_uS){
    t0_uS = initialTimeUTC_uS;
    t1_uS = initialTimeUTC_uS + window_uS;
    bottom = window_uS;
    _isDone = 1;
    for(uint8_t d = 0; d < nDimensions; d++){
      top[d] = targetVals[d] - initialVals[d];
      _isDone &= top[d] == 0;
      k[d] = (initialVals[d] * window_uS) + (window_uS/2);
    }
    return _isDone;
  }

  /**
   * @brief set the brightness interpolation constants for a new rate interpolation. initialVals and targetVals must already be set
   * 
   * @param utcTimestamp_us 
   * @param rate_uS 
   * @return isDone_t 
   */
  isDone_t newRateInterpolation(const uint64_t& utcTimestamp_us, const uint64_t rate_uS){
    t0_uS = utcTimestamp_us;
    bottom = rate_uS;
    
    _isDone = 1;
    uint8_t max_dV = 0;
    for(uint8_t d = 0; d < nDimensions; d++){
      int16_t dV = targetVals[d] - initialVals[d];
      top[d] = dV < 0 ? -1 : dV > 0;
      const duty_t abs_dV = abs(dV);
      if(abs_dV > max_dV){max_dV == abs_dV;}
      _isDone &= dV == 0;
      k[d] = (initialVals[d] * rate_uS) + (rate_uS/2);
    }
    t1_uS = utcTimestamp_us + (max_dV * rate_uS);

    return _isDone;
  };

  /**
   * @brief handles a time update notification
   * 
   * @param timeUpdates 
   */
  void notification(const TimeUpdateStruct& timeUpdates){
    t0_uS += timeUpdates.utcTimeChange_uS;
    t1_uS += timeUpdates.utcTimeChange_uS;
  };
};







/*
handles interpolation, both window and rate. calculates constants when interpolation is initiated. everything is out in the open and pretty exposed, so don't go abusing trust.
TODO: resizeWindow method
TODO: break into two sub-classes, so that single-value interpolator can be used elsewhere without the extra over-head

mathematically,
  b(t) = b(0) + (top/bottom)*(t - t0)

with rounding,
  b(t) = (b(0)*bottom + top*(t-t0) + (bottom/2))/bottom
       = (k + top*(t-t0))/bottom
*/
template <uint8_t nColours>
class ModeInterpolationClass{
private:
  isDone_t _isDone = IsDoneBitFlags::both;

  isDone_t _setBrightnessDoneFlag(bool finished){
    _isDone = (_isDone & IsDoneBitFlags::colours) + finished;
    return _isDone;
  }

public:

  Interpolator<1> brightness;
  Interpolator<nColours> colours;
  
  ModeInterpolationClass(){};

  void setInitialVals(const duty_t vals[nColours+1]){
    brightness.initialVals[0] = vals[0];
    memcpy(colours.initialVals, &vals[1], nColours);
  }

  void getInitialVals(duty_t out[nColours+1]){
    out[0] = brightness.initialVals[0];
    memccpy(&out[1], colours.initialVals, nColours);
  }
  
  void setTargetVals(const duty_t vals[nColours+1]){
    brightness.targetVals[0] = vals[0];
    memcpy(colours.targetVals, &vals[1], nColours);
  }

  void getTargetVals(duty_t out[nColours+1]){
    out[0] = brightness.targetVals[0];
    memcpy(&out[1], colours.targetVals, nColours);
  }

  void setTargetBrightness(const duty_t targetB){
    brightness.targetVals[0] = targetB;
  }

  duty_t getTargetBrightness(){
    return brightness.targetVals[0];
  }
  
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
  void notification(const TimeUpdateStruct& timeUpdates){
    brightness.notification(timeUpdates);
    colours.notification(timeUpdates);
  }

  /**
   * @brief copies targetVals to lightVals and initialVals, and ends the interpolations
   * 
   * @param lightValues 
   * @return isDone_t 
   */
  isDone_t endInterpolation(duty_t lightValues[nColours+1]);

  /**
   * @brief interpolates the values at a given timestamp, and sets currentVals. currentVals is a reference to a c style array, so you could write the target vals of another ModeInterpolationClass instance
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
inline isDone_t ModeInterpolationClass<nColours>::findNextValues(duty_t currentVals[nColours+1], const uint64_t& utcTimestamp_uS){
  _isDone = (colours.findNextValues(&currentVals[1], utcTimestamp_uS) << 1);
  _isDone |= brightness.findNextValues(currentVals, utcTimestamp_uS);
  return _isDone;
}

template <uint8_t nColours>
inline isDone_t ModeInterpolationClass<nColours>::newBrightnessInterp_window(const uint64_t& initialTimeUTC_uS, const uint64_t window_uS){
  bool finished = brightness.restartWindowInterpolation(initialTimeUTC_uS, window_uS);
  return _setBrightnessDoneFlag(finished);
}


template <uint8_t nColours>
inline isDone_t ModeInterpolationClass<nColours>::newBrightnessVal_window(const uint64_t& initialTimeUTC_uS, const uint64_t window_uS, const duty_t currentVal, const duty_t newVal){
  bool finished = brightness.newWindowInterpolation(initialTimeUTC_uS, window_uS, currentVal, newVal);
  return _setBrightnessDoneFlag(finished);
}

template <uint8_t nColours>
inline isDone_t ModeInterpolationClass<nColours>::endInterpolation(duty_t lightValues[nColours + 1])
{
  lightValues[0] = brightness.targetVals[0];
  brightness.initialVals[0] = brightness.targetVals[0];
  brightness.top[0] = 0;
  for(uint8_t c = 0; c < nColours; c++){
    lightValues[c+1] = colours.targetVals[c];
    colours.initialVals[c] = colours.targetVals[c];
    colours.top[c] = 0;
  }
  _isDone = IsDoneBitFlags::both;
  return _isDone;
}

template <uint8_t nColours>
inline isDone_t ModeInterpolationClass<nColours>::rebuildInterpConstants_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS){
  _isDone = (colours.restartWindowInterpolation(utcTimestamp_us, window_uS) << 1);
  _isDone |= brightness.restartWindowInterpolation(utcTimestamp_us, window_uS);
  return _isDone;
}

template <uint8_t nColours>
inline isDone_t ModeInterpolationClass<nColours>::newInterp_window(const uint64_t& utcTimestamp_us, const uint64_t window_uS, const duty_t initial[nColours+1], const duty_t target[nColours+1]){
  _isDone = (colours.newWindowInterpolation(utcTimestamp_us, window_uS, &initial[1], &target[1]) << 1);
  _isDone |= brightness.newWindowInterpolation(utcTimestamp_us, window_uS, initial, target);
  return _isDone;
}

template <uint8_t nColours>
inline isDone_t ModeInterpolationClass<nColours>::newBrightnessInterp_rate(const uint64_t& utcTimestamp_us, const uint64_t rate_uS){
  bool finished = brightness.newRateInterpolation(utcTimestamp_us, rate_uS);
  return _setBrightnessDoneFlag(finished);
}

template <uint8_t nColours>
inline isDone_t ModeInterpolationClass<nColours>::rebuildInterpConstants_rate(const uint64_t& utcTimestamp_us, const uint64_t rate_uS){
  _isDone = (colours.newRateInterpolation(utcTimestamp_us, rate_uS) << 1);
  _isDone |= brightness.newRateInterpolation(utcTimestamp_us, rate_uS);
  return _isDone;
};

#endif