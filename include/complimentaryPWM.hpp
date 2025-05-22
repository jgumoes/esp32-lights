#ifndef __COMPLIMENTARY_PWM_HPP__
#define __COMPLIMENTARY_PWM_HPP__

#include <driver/ledc.h>

#include "ModalLights.h"

constexpr uint16_t maxValue_9bits = (UINT16_MAX >> 7);

class ComplimentaryPWM : public VirtualLightsClass{
  private:
    const int _pin0;
    const int _pin1;
  public:

  ComplimentaryPWM(int pin0 = D1, int pin1 = D2) : _pin0(pin0), _pin1(pin1) {
    ledc_timer_config_t timer_config_0 = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_9_BIT,
      .timer_num = LEDC_TIMER_0,
      .freq_hz = 1000,
    };
    esp_err_t timer_err = ledc_timer_config(&timer_config_0);
    // Serial.print("timer config: "); Serial.println(esp_err_to_name(timer_err));

    ledc_channel_config_t channel_config_0 = {
      .gpio_num = _pin0,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_0,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = 0,
      .flags{
        .output_invert = 0
      }
    };
    esp_err_t channel_0_err = ledc_channel_config(&channel_config_0);
    // Serial.print("channel 0 config: "); Serial.println(esp_err_to_name(channel_0_err));

    ledc_channel_config_t channel_config_1 = {
      .gpio_num = _pin1,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = LEDC_CHANNEL_1,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = LEDC_TIMER_0,
      .duty = 0,
      .hpoint = UINT8_MAX+1,
      .flags{
        .output_invert = 0
      }
    };
    esp_err_t channel_1_err = ledc_channel_config(&channel_config_1);
    // Serial.print("channel 1 config: "); Serial.println(esp_err_to_name(channel_1_err));
  };

  void setChannelValues(duty_t newValues[nChannels]){
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, newValues[0]);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, newValues[0]);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
  }
};

#endif