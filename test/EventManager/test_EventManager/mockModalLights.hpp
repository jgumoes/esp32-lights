#ifndef __MOCK_MODAL_LIGHTS_HPP__
#define __MOCK_MODAL_LIGHTS_HPP__

#include <Arduino.h>

#include "ModalLights.h"

struct CalledModesStruct{
  modeUUID ID = 0;
  bool isActive = false;
};

class MockModalLights : public ModalLightsInterface{
  private:
    etl::vector<CalledModesStruct, UINT16_MAX> _calledModes{};

    uint8_t _cancelCallCount = 0;
    modeUUID _activeMode = 0;
    modeUUID _backgroundMode = 1; // this would be system default constant brightness
    uint8_t _setModeCount = 0;
    uint64_t _mostRecentTriggerTime = 0;
  public:
    MockModalLights(){};
    void updateLights() override{};

    void setModeByUUID(modeUUID modeID, uint64_t triggerTimeLocal_S, bool isActive) override {
      _setModeCount++;
      _calledModes.push_back(CalledModesStruct{.ID = modeID, .isActive = isActive});
      
      if(isActive){_activeMode = modeID;}
      else{_backgroundMode = modeID;}

      _mostRecentTriggerTime = triggerTimeLocal_S;
    }

    bool cancelActiveMode() override {
      throw("shouldn't be called from event manager");
      if(_cancelCallCount == 255){
        throw ("how have you called cancelActiveMode this many times? you should have called it 0 times you numpty");
      }
      _cancelCallCount++;
      return true;
    }

    /**
     * @brief cancelActiveMode() throws an error to make sure it's not call by EventManager. however, I do need to cancel the active mode sometimes in the tests, which is what this method is for
     * 
     */
    void test_cancelActiveMode(){
      _activeMode = 0;
    }

    uint8_t getCancelCallCount(modeUUID modeID){
      return _cancelCallCount;
    }

    uint8_t getModeCallCount(modeUUID modeID){
      if(_calledModes.empty()){
        return 0;
      }
      uint8_t callCount = 0;
      for(auto calledIT : _calledModes){
        if(calledIT.ID == modeID){
          callCount++;
        }
      }
      return callCount;
    }

    // TODO: remove _setModeCount
    uint8_t getSetModeCount(){return _setModeCount;}

    modeUUID getActiveMode(){return _activeMode;}
    modeUUID getBackgroundMode(){return _backgroundMode;}
    modeUUID getMostRecentMode(){
      if(_calledModes.empty()){
        return 0;
      }
      return _calledModes.back().ID;
    }
    uint64_t getMostRecentTriggerTime(){return _mostRecentTriggerTime;}

    void resetInstance(){
      _calledModes.clear();
      _cancelCallCount = 0;
      _setModeCount = 0;
      _activeMode = 0;
      _backgroundMode = 1;
      _mostRecentTriggerTime = 0;
    }

    bool setState(bool newState) override {
      _lightVals.state = newState;
      return newState;
    }

    duty_t setBrightnessLevel(duty_t brightness) override {
      _lightVals.values[0] = brightness;
      return brightness;
    }

    duty_t getSetBrightness() override {return _lightVals.values[0];}

    int16_t previousAdjustment = 0;
    duty_t adjustBrightness(duty_t amount, bool increasing) override {
      previousAdjustment = increasing
                          ? amount
                          : -amount;
      
      // if adjusting up from off
      if(
        !_lightVals.state
        && increasing
        && (amount > 0)
      ){
        _lightVals.state = true;
        _lightVals.values[0] = 0;
      }

      int16_t bigB = increasing
                    ? _lightVals.values[0] + amount
                    : _lightVals.values[0] - amount;
      if(bigB > 255){bigB = 255;}
      if(bigB < 0){bigB = 0;}

      duty_t newBrightness = static_cast<duty_t>(bigB);
      _lightVals.values[0] = newBrightness;
      return newBrightness;
    }
};

#endif