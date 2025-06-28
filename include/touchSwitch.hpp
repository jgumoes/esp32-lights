#ifndef __TOUCH_SWITCH_HPP__
#define __TOUCH_SWITCH_HPP__

#include <Arduino.h>
#include <driver/touch_sensor.h>
#include <memory>

#include "DeviceTime.h"
#include "ModalLights.h"
#include "OneButtonInterface.hpp"

/**
 * @brief initialise the ESP32 S3 touch peripheral, and use it as a single button physical interface
 * 
 */
class S3TouchButton : public OneButtonInterface {
  private:
    uint16_t sleep_cycle = 192;  // about 1.5 mS, but RTC_SLOW_CLK doesn't actually exist so hard to tell
    uint16_t meas_time = 500;    // TODO: update; about 5mS when touched; the number of charge/discharge cycles per measurement interval. clock frequency is about 18MHz


    uint32_t _benchmark;  // the no-touch measurement value. seems to be self-callibrating
    uint32_t _noiseVal;   // raw value of the internal noise channel. used to cancel effects of supply and EMI noise
    uint32_t _rawTouchVal;

    const touch_pad_t _touchPin;
    const uint32_t _touchThreshold;

  public:

    S3TouchButton(
      std::shared_ptr<DeviceTimeClass> deviceTime,
      std::shared_ptr<ModalLightsController> modalLights,
      std::shared_ptr<ConfigStorageClass> configStorage,
      touch_pad_t touchPin = TOUCH_PAD_NUM7,
      uint32_t touchThreshold = 50000
    ) : OneButtonInterface(deviceTime, modalLights, configStorage), _touchPin(touchPin), _touchThreshold(touchThreshold) {
      // setup touch peripheral
      touch_pad_init();
      touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);

      touch_pad_init();
      touch_pad_config(_touchPin);
      touch_pad_set_meas_time(sleep_cycle, meas_time);

      touch_pad_denoise_t denoise = {
        /* The bits to be cancelled are determined according to the noise level. */
        .grade = TOUCH_PAD_DENOISE_BIT12,
        .cap_level = TOUCH_PAD_DENOISE_CAP_L7,
      };
      touch_pad_denoise_set_config(&denoise);
      touch_pad_denoise_enable();

      // touch_pad_isr_register(active_handler, NULL, TOUCH_PAD_INTR_MASK_ACTIVE);
      // touch_pad_isr_register(inactive_handler, NULL, TOUCH_PAD_INTR_MASK_INACTIVE);
      // touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_INACTIVE);
      // TODO: timeout interrupt


      touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
      touch_pad_fsm_start();
      touch_pad_set_thresh(_touchPin, _touchThreshold); // this make _benchmark stable
    }

    bool getCurrentStatus() override {
      return (touch_pad_get_status() & BIT(_touchPin)) != 0;
    }
    
    void printValues(){
      touch_pad_read_benchmark(_touchPin, &_benchmark);
      touch_pad_denoise_read_data(&_noiseVal);
      touch_pad_read_raw_data(_touchPin, &_rawTouchVal);

      Serial.print("benchmark: "); Serial.println(_benchmark);
      Serial.print("noise value: "); Serial.println(_noiseVal);
      Serial.print("touch value: "); Serial.println(_rawTouchVal);
    }
};


#endif