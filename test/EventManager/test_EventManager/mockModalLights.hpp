#ifndef __MOCK_MODAL_LIGHTS_HPP__
#define __MOCK_MODAL_LIGHTS_HPP__

#include <Arduino.h>

#include "ModalLights.h"

class MockModalLights : public ModalLightsInterface{
  private:
    std::map<modeUUID, uint8_t> _calledModes;
    std::map<modeUUID, uint8_t> _canceledModes;
    // std::shared_ptr<DataStorageClass> _storage;
    modeUUID _activeMode = 0;
    modeUUID _backgroundMode = 1; // this would be system default constant brightness
    modeUUID _mostRecentMode = 0;
    uint8_t _setModeCount = 0;
    uint64_t _mostRecentTriggerTime = 0;
  public:
    MockModalLights(){};
    void update(uint64_t timestampUTC) override{};

    void setModeByUUID(modeUUID modeID, uint64_t triggerTimeLocal, bool isActive) override {
      _setModeCount++;
      if(_calledModes.count(modeID) == 0){
        _calledModes[modeID] = 1;
      }
      else{
        _calledModes[modeID] += 1;
      }
      _mostRecentTriggerTime = triggerTimeLocal;
      _mostRecentMode = modeID;

      if(isActive){_activeMode = modeID;}
      else{_backgroundMode = modeID;}
    }

    void cancelMode(modeUUID modeID) override {
      if(_canceledModes.count(modeID) == 0){
        _canceledModes[modeID] = 1;
      }
      else{
        _canceledModes[modeID]++;
      }
      if(_activeMode == modeID){
        _activeMode = 0;
      }
      if(_backgroundMode == modeID){
        _backgroundMode = 1;
      }
    }

    uint8_t getCancelCallCount(modeUUID modeID){
      return _canceledModes[modeID];
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
      _canceledModes.clear();
      _mostRecentMode = 0;
      _setModeCount = 0;
      _activeMode = 0;
      _backgroundMode = 1;
      _mostRecentTriggerTime = 0;
    }
};

#endif