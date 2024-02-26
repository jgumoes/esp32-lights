#ifndef _LIGHT_MODES_H_
#define _LIGHT_MODES_H_
#include <stdint.h>
#include "lights_pwm.h"

// namespace ModalLights {
enum LightModes {
  constantBrightness, // no time-based adjustments. brightness and state are managed by user interface only
  interpolation, // interpolation that always takes the same time
  constRate, // interpolation that occurs at a constant rate i.e. B = m*t + B0
  triangle,
  sine
};

enum WaveModes {
  WaveMode_none,
  WaveMode_wave, // wave with constant f (default)
  WaveMode_chirp // wave with increasing or decreasing chirp
};

void setLightsMode(
  LightModes lightMode,
  uint64_t initialTime,
  uint64_t currentTime,
  uint64_t endTime,
  duty_t initialBrightness,
  duty_t endBrightness
);

/**
 * @brief finds the correct brightness for the given timestamp.
 * should be called by updateBrightness
 * 
 * @param currentTimestamp the current timestamp in microseconds
 * @return duty_t the PWM duty
 */
// duty_t findBrightness(uint64_t currentTimestamp);

/**
 * @brief updates the brightness. should be called from the main loop or RTOS
 * TODO: move to library header
 * 
 * @param currentTimestamp 
 * @return duty_t the new brightness
 */
// duty_t updateBrightness(uint64_t currentTimestamp);

/*
 * call when the brightness is manually adjusted
 */
// void setBrightness(duty_t brightness);
// }

class ModalStrategy
{
public:
  virtual ~ModalStrategy() = default;

  /**
   * @brief calculates and returns a new brightness value based
   * on the current timestamp
   * 
   * @param currentTimestamp the current timestamp in microseconds
   * @returns the new brightness value
  */
  virtual duty_t updateBrightness(uint64_t currentTimestamp) = 0;

  /**
   * @brief re-adjusts the strategy params, and returns the correct 
   * brightness (i.e. if the brightness is being set above a maximum)
  */
  virtual duty_t setBrightness(duty_t brightness) = 0;
};

class ConstantBrightnessMode : public ModalStrategy
{
private:
  duty_t _maxBrightness;

public:
  ConstantBrightnessMode(duty_t brightness) : _maxBrightness(brightness){};

  // set the brightness of the lights
  duty_t setBrightness(duty_t brightness){
    return brightness <= _maxBrightness ? brightness : _maxBrightness;
  }
  
  // find and set the next brightness
  duty_t updateBrightness(uint64_t currentTimestamp) {
    return getDutyLevel();
  }
};


#endif