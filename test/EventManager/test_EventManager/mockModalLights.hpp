#ifndef __MOCK_MODAL_LIGHTS_HPP__
#define __MOCK_MODAL_LIGHTS_HPP__

#include <Arduino.h>

#include "ModalLights.h"

class MockModalLights : public ModalLightsInterface{
  private:
    std::map<modeUUID, uint8_t> _calledModes;
    uint8_t _cancelCallCount = 0;
    // std::shared_ptr<DataStorageClass> _storage;
    modeUUID _activeMode = 0;
    modeUUID _backgroundMode = 1; // this would be system default constant brightness
    modeUUID _mostRecentMode = 0;
    uint8_t _setModeCount = 0;
    uint64_t _mostRecentTriggerTime = 0;
  public:
    MockModalLights(){};
    void updateLights() override{};

    void setModeByUUID(modeUUID modeID, uint64_t triggerTimeLocal_S, bool isActive) override {
      _setModeCount++;
      if(_calledModes.count(modeID) == 0){
        _calledModes[modeID] = 1;
      }
      else{
        _calledModes[modeID] += 1;
      }
      
      if(isActive){_activeMode = modeID;}
      else{_backgroundMode = modeID;}

      _mostRecentTriggerTime = triggerTimeLocal_S;
      _mostRecentMode = modeID;
    }

    bool cancelActiveMode() override {
      throw("shouldn't be called from event manager");
      if(_cancelCallCount == 255){
        throw ("how have you called cancelActiveMode this many times? you should have called it 0 times you numpty");
      }
      _cancelCallCount++;
      return true;
    }

    uint8_t getCancelCallCount(modeUUID modeID){
      return _cancelCallCount;
    }

    uint8_t getModeCallCount(modeUUID modeID){
      return _calledModes[modeID];
    }

    uint8_t getSetModeCount(){return _setModeCount;}

    modeUUID getActiveMode(){return _activeMode;}
    modeUUID getBackgroundMode(){return _backgroundMode;}
    modeUUID getMostRecentMode(){return _mostRecentMode;}
    uint64_t getMostRecentTriggerTime(){return _mostRecentTriggerTime;}

    void resetInstance(){
      _calledModes.clear();
      _cancelCallCount = 0;
      _mostRecentMode = 0;
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

    duty_t adjustBrightness(duty_t amount, bool increasing) override {
      duty_t newBrightness;
      if(increasing){
        newBrightness = amount + _lightVals.values[0] > 255
                      ? 255
                      : _lightVals.values[0] + amount;
      }
      else{
        newBrightness = amount > _lightVals.values[0]
                      ? 0
                      : _lightVals.values[0] - amount;
      }
      _lightVals.values[0] = newBrightness;
      return newBrightness;
    }
};

#endif