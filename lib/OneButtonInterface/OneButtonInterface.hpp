#ifndef __ONE_BUTTON_INTERFACE_HPP__
#define __ONE_BUTTON_INTERFACE_HPP__

#include <Arduino.h>
#include <memory>

#include "DeviceTime.h"
#include "ModalLights.h"
#include "ConfigStorageClass.hpp"
#include "ProjectDefines.h"

enum class PressStates : uint8_t {
  none = 0,
  shortPress = 1,
  longPress = 2
};

enum class ButtonStatus : uint8_t {
  // currentStatus = 0; previousStatus = 0
  inactive = 0,
  // currentStatus = 0; previousStatus = 1
  fallingEdge = 1,
  // currentStatus = 1; previousStatus = 0
  risingEdge = 2,
  // currentStatus = 1; previousStatus = 1
  active = 3
};

/**
 * @brief A class for a single button physical interface. All it needs is a getCurrentStatus() and it'll be good to go!
 * Contains a state machine to distinguish between short and long presses. Releasing a short press will call modalLights->toggleState(). Holding a long press will adjust modal lights by {0, 255} interpolated across the window in the configs. The adjustment direction is up if brightness level == 0, down if brightness level == 255, or the oposite to the previous direction.
 * 
 */
class OneButtonInterface : public ConfigUser {
  protected:
    bool _previousStatus = false;  // press state during previous update() call
    PressStates _buttonState = PressStates::none;

    struct ShortPressParams {
      uint64_t endTimeUTC_uS;

      void reset(){
        endTimeUTC_uS = 0;
      };
    } _shortPress;

    struct LongPressParams {
      private:
      Interpolator<1> _interp;
      public:
      bool direction = false;
      duty_t cumAdj;

      void initInterp(uint64_t startTimeUTC_uS, OneButtonConfigStruct& configs){
        _interp.newWindowInterpolation(
          startTimeUTC_uS, configs.longPressWindow_mS*1000,
          1, 255
        );
      }

      duty_t getAdjustment(const uint64_t& currentTime_uS){
        duty_t newCumAdj = _interp.interpolateValue(currentTime_uS, 0);
        duty_t adj = newCumAdj - cumAdj;
        cumAdj = newCumAdj;
        return adj;
      }

      /**
       * @brief sets the interpolation to always return 0, until it's reset
       * 
       */
      void reset(){
        _interp.t0_uS = 0;
        _interp.t1_uS = 0;
        _interp.initialVals[0] = 0;
        _interp.targetVals[0] = 0;
        cumAdj = 0;
      }
    } _longPress;

    OneButtonConfigStruct _configs;

    std::shared_ptr<DeviceTimeClass> _deviceTime;
    std::shared_ptr<ModalLightsInterface> _modalLights;
    std::shared_ptr<ConfigStorageClass> _configStorage;

    void _enterState_none(){
      _previousStatus = false;
      _buttonState = PressStates::none;
      _shortPress.reset();
      _longPress.reset();
    }
    
    void _enterState_shortPress(){
      _shortPress.endTimeUTC_uS = _deviceTime->getUTCTimestampMicros() + (_configs.timeUntilLongPress_mS * 1000);
      _longPress.reset();
      _buttonState = PressStates::shortPress;
    }

    void _enterState_longPress(){
      // set adjustment direction
      {
        const duty_t currentBrightness = _modalLights->getBrightnessLevel();

        /*
        equivalent to:

        if(currentBrightness == 0){
          _longPress.direction = true;
        }
        else if(currentBrightness == 255){
          _longPress.direction = false;
        }
        else{
          _longPress.direction = !_longPress.direction;
        }
        */
        _longPress.direction = (
          currentBrightness == 0 ||
          !(currentBrightness == 255 || _longPress.direction)
        );
      }
      _buttonState = PressStates::longPress;
      _longPress.initInterp(_shortPress.endTimeUTC_uS, _configs);
      _shortPress.reset();
      _updateState_longPress(ButtonStatus::active);
    }


    void _updateState_none(const ButtonStatus status){
      switch(status)
      {
      case ButtonStatus::inactive:
        return;
      case ButtonStatus::fallingEdge:
        // this shouldn't have happened, so default to none
        break;
      case ButtonStatus::risingEdge:
        _enterState_shortPress();
        return;
      case ButtonStatus::active:
        // this shouldn't happen, but enter short press anyway
        _enterState_shortPress();
        return;
      default:
        // this should be inaccessible, but default to none state
        break;
      }
      _enterState_none();
    }

    void _updateState_shortPress(const ButtonStatus status){
      switch(status)
      {
      case ButtonStatus::inactive:
        // shouldn't happen 
        break;
      case ButtonStatus::fallingEdge:
        _modalLights->toggleState();
        _enterState_none();
        return;
      case ButtonStatus::risingEdge:
        // shouldn't happen, but re-enter shortPress
        _enterState_shortPress();
        return;
      case ButtonStatus::active:
        if(_shortPress.endTimeUTC_uS == 0){
          // if endTime hasn't been set, re-enter the state legally
          _enterState_shortPress();
          return;
        }
        if(
          _shortPress.endTimeUTC_uS <= _deviceTime->getUTCTimestampMicros()
        ){
          _enterState_longPress();
        }
        return;
      default:
        // this should be inaccessible, but default to none state
        break;
      }
      _enterState_none();
    }

    void _updateState_longPress(const ButtonStatus status){
      
      duty_t adj = _longPress.getAdjustment(_deviceTime->getUTCTimestampMicros());
      switch(status)
      {
      case ButtonStatus::inactive:
        // shouldn't happen
        _enterState_none();
        return;
      case ButtonStatus::fallingEdge:
        _modalLights->adjustBrightness(adj, _longPress.direction);
        _enterState_none();
        return;
      case ButtonStatus::risingEdge:
        // shouldn't happen, but enter state shortPress
        _enterState_shortPress();
        return;
      case ButtonStatus::active:
        _modalLights->adjustBrightness(adj, _longPress.direction);
        return;
      default:
        _enterState_none();
        // this should be inaccessible, but default to none state
        return;
      }
    }

  public:

    OneButtonInterface(
      std::shared_ptr<DeviceTimeClass> deviceTime,
      std::shared_ptr<ModalLightsInterface> modalLights,
      std::shared_ptr<ConfigStorageClass> configStorage
    ) : _deviceTime(deviceTime), _modalLights(modalLights), _configStorage(configStorage), ConfigUser(ModuleID::oneButtonInterface) {
      _configStorage->registerUser(this, _configs);
    };

    /**
     * @brief Get the current status
     * TODO: replace virtual method with a function pointer and poll the difference
     * 
     * @return true if button is being pressed
     * @return false if button isn't being pressed
     */
    virtual bool getCurrentStatus() = 0;

    PressStates getFSMState(){return _buttonState;}
    
    /**
     * @brief check the button status, update the state machine, and update modalLights
     * 
     */
    void update(){
      bool currentStatus = getCurrentStatus();

      const ButtonStatus status = static_cast<ButtonStatus>((currentStatus<<1)|_previousStatus);
      _previousStatus = currentStatus;

      switch (_buttonState)
      {
      case PressStates::shortPress:
        _updateState_shortPress(status);
        break;
      case PressStates::longPress:
        _updateState_longPress(status);
        break;
      default:
        _updateState_none(status);
        break;
      };
    }
  
  // ConfigUser overrides

  void newConfigs(const byte newConfig[maxConfigSize]) override {
    // I don't care about the behaviour. nobody is adjusting configs while holding the button
    ConfigStructFncs::deserialize(newConfig, _configs);
  }

  packetSize_t getConfigs(byte config[maxConfigSize]) override {
    ConfigStructFncs::serialize(config, _configs);
    return getConfigPacketSize(this->type);
  }
};


#endif