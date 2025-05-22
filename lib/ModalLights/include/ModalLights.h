#ifndef _MODAL_LIGHTS_H_
#define _MODAL_LIGHTS_H_

#include <Arduino.h>
#include <memory>

#include "../DeviceTime/include/DeviceTime.h"
#include "modes.h"
#include "ProjectDefines.h"
#include "DataStorageClass.h"

class VirtualLightsClass{
  public:
  virtual ~VirtualLightsClass() = default;
  virtual void setChannelValues(duty_t newValues[nChannels]) = 0;
};

template <class ConcreteLightsClass>
std::unique_ptr<VirtualLightsClass> concreteLightsClassFactory(){
  return std::make_unique<ConcreteLightsClass>();
}

// once the mode classes are replaced with function pointers, the interface class should be deletable
class ModalLightsInterface{
  protected:
    LightStateStruct _lightVals;  // current duty cycle, state, and colour ratios of the lights. should only get changed by strategy instance, then passed to the lights instance

  public:
    virtual ~ModalLightsInterface() = default;

    virtual void updateLights() = 0;

    /**
     * @brief Set the mode by UUID. collects the mode from storage
     * and uses the datapacket to determine if the mode is active or background. changes are actioned next time update() is called.
     * TODO: should alert the homeserver if mode doesn't exist
     * 
     * @param modeID 
     * @param triggerTimeLocal_S the local timestamp in seconds that the event should have triggered at
     * @param isActive 
     */
    virtual void setModeByUUID(modeUUID modeID, uint64_t triggerTimeLocal_S, bool isActive) = 0;

    virtual bool cancelActiveMode() = 0;
    // virtual bool cancelActiveMode(InteractionSources source) = 0;  // post MVP

    // virtual bool setState(bool state, InteractionSources source) = 0; // post MVP
    virtual bool setState(bool newState) = 0;

    // virtual duty_t setBrightnessLevel(duty_t brightness, InteractionSources source) = 0;  // post MVP
    virtual duty_t setBrightnessLevel(duty_t brightness) = 0;

    /**
     * @brief adjust the brightness from the current brightness
     * 
     * @param amount 
     * @param increasing 
     * @param source 
     */
    virtual duty_t adjustBrightness(duty_t amount, bool increasing) = 0;
    // virtual duty_t adjustBrightness(duty_t adjustment, bool increasing, InteractionSources source) = 0;  // post MVP

    // ##### non-virtual methods #####

    /**
     * @brief Get the current brightness setting (aka target brightness) of the lights. not affected by state.
     * 
     * @return duty_t 
     */
    virtual duty_t getSetBrightness() = 0;

    bool getState(){
      return _lightVals.state;
    }

    /**
     * @brief Get the current actual brightness level, including state. i.e. _lightVals.values[0] * _lightVals.state
     * 
     * @return duty_t 
     */
    duty_t getBrightnessLevel(){
      return _lightVals.values[0] * _lightVals.state;
    };

    void toggleState(){
      setState(!_lightVals.state);
    }
};

static bool modeUsesTriggerTime(ModeTypes modeType){
  switch (modeType)
  {
  case ModeTypes::constantBrightness:
    return true;
  
  default:
    return true;
  }
}

/**
 * LightsClass must have method setDutyCycle(duty_t)
*/
// template<uint8_t nChannels>
class ModalLightsController : public ModalLightsInterface, public TimeObserver
{
private:
  /* data */
  std::unique_ptr<ModalStrategyInterface> _mode;

  std::unique_ptr<VirtualLightsClass> _lights;
  std::shared_ptr<DeviceTimeClass> _deviceTime;
  std::shared_ptr<DataStorageClass> _dataStorage;
  std::shared_ptr<ConfigManagerClass> _configsClass;
  ModalConfigsStruct _configs;
  
  std::shared_ptr<ModeInterpolationClass<nChannels>> _interpClass = std::make_shared<ModeInterpolationClass<nChannels>>();

  // TODO: remove mode IDs; they duplicate from the data packets
  modeUUID _activeMode = 0;
  ModeDataStruct _activeModeData = ModeDataStruct{};
  uint64_t _activeModeTriggerTimeUTC_uS = 0;
  
  modeUUID _backgroundMode = 0;
  ModeDataStruct _backgroundModeData = ModeDataStruct{};
  uint64_t _backgroundModeTriggerTimeUTC_uS = 0;

  modeUUID _nextActiveMode = 0;
  uint64_t _nextActiveTriggerTimeUTC_uS = 0;
  
  modeUUID _nextBackgroundMode = 1; // default mode incase event manager doesn't request any modes
  uint64_t _nextBackgroundTriggerTimeUTC_uS = 0;
  
  bool _isSetupComplete = false;

  /**
   * @brief change the current mode. _activeMode and _backgroundMode values must already be set, and the data already loaded from storage. if _activeMode is unset, it'll load initialise _backgroundMode
   * 
   */
  void _changeMode(){
    ModeDataStruct* dataPacket;
    bool isActive;
    uint64_t* triggerTimeUTC_uS;
    modeUUID* modeID;

    // set the pointers to the values
    {
      if(_activeMode != 0){
        isActive = true;
        dataPacket = &_activeModeData;
        triggerTimeUTC_uS = &_activeModeTriggerTimeUTC_uS;
      }
      else{
        // if backgroundMode hasn't been set yet, load default from storage
        if(_backgroundMode == 0){_loadMode();}

        isActive = false;
        dataPacket = &_backgroundModeData;
        triggerTimeUTC_uS = &_backgroundModeTriggerTimeUTC_uS;
      }
    }

    // ModeTypes modeType = static_cast<ModeTypes>(dataPacket[1]);
    ModeTypes modeType = dataPacket->type;
    uint64_t currentTimeUTC_uS = _deviceTime->getUTCTimestampMicros();
    // NOTE: these might be useful when interpolation class is disassembled
    // duty_t previousValues[nChannels+1];
    // _mode->getTargetVals(previousValues, currentTimeUTC_uS, _lightVals);

    // initialise the new mode
    switch(modeType){
      case ModeTypes::constantBrightness:
        _mode = std::make_unique<ConstantBrightnessMode>(
          currentTimeUTC_uS,
          *triggerTimeUTC_uS,
          dataPacket,
          _interpClass,
          _lightVals,
          isActive,
          _configs
        );
        break;
      default:
        break;
    }

    if(!_isSetupComplete){
      _isSetupComplete = true;
      _mode->setState(true, _lightVals, currentTimeUTC_uS);
    }
  }

  bool _loadNextActiveMode(){
    if(_nextActiveMode == 0){
      return false;
    }

    // set the mode data packet
    bool success;
    if(_nextActiveMode == _activeMode){
      // only re-trigger if the trigger times are different and the mode type cares about trigger times
      success = 
        (_nextActiveTriggerTimeUTC_uS != _activeModeTriggerTimeUTC_uS)
        && modeUsesTriggerTime(_activeModeData.type);
    }
    else if(_nextActiveMode == _backgroundMode){
      _activeModeData = _backgroundModeData;
      success = true;
    }
    else{
      // TODO: dataStorage should fill a ModeDataStruct instead of an array
      uint8_t dataArray[modePacketSize];
      success = _dataStorage->getMode(_nextActiveMode, dataArray);
      if(success){
        deserializeModeData(dataArray, &_activeModeData);
      }
    }

    // switch mode if mode data packet was set
    if(success){
      _activeMode = _nextActiveMode;
      _activeModeTriggerTimeUTC_uS = _nextActiveTriggerTimeUTC_uS;
      _changeMode();
    }

    // reset _next_X_Mode variables
    _nextActiveMode = 0;
    _nextActiveTriggerTimeUTC_uS = 0;
    return success;
  }

  bool _loadNextBackgroundMode(){
    if(_nextBackgroundMode == 0){
      return false;
    }

    // set the mode data packet
    bool success;
    if(_nextBackgroundMode == _backgroundMode){
      success = true;
    }
    else if(_nextBackgroundMode == _activeMode){
      _backgroundModeData = _activeModeData;
      success = true;
    }
    else{
      // TODO: dataStorage should fill a ModeDataStruct instead of an array
      uint8_t dataArray[modePacketSize];
      success = _dataStorage->getMode(_nextBackgroundMode, dataArray);
      if(success){
        deserializeModeData(dataArray, &_backgroundModeData);
      }
    }

    // switch mode if mode data packet was set
    if(success){
      _backgroundMode = _nextBackgroundMode;
      _backgroundModeTriggerTimeUTC_uS = _nextBackgroundTriggerTimeUTC_uS;
      if(_activeMode == 0){
        _changeMode();
      }
    }

    // reset _next_X_Mode variables
    _nextBackgroundMode = 0;
    _nextBackgroundTriggerTimeUTC_uS = 0;
    return success;
  }

  /**
   * @brief loads _nextMode from storage, and calls _changeMode if applicable. sets current values to _nextMode if _nextMode is valid, resets _next values if not. if _nextMode is background but current mode is active, it'll load the data from storage but not force a change. _nextMode values must already be set.
   * 
   */
  void _loadMode(){
    _loadNextActiveMode();
    _loadNextBackgroundMode();
  }

public:
  /**
   * @brief Construct an instance. doesn't load or set mode, as event manager will decide which modes should be first. update() will set background mode to the default constant brightness mode if none have been set.
   * 
   * @param lightsClass 
   * @param deviceTime 
   * @param dataStorage 
   * @param configs 
   */
  ModalLightsController(
    std::unique_ptr<VirtualLightsClass>&& lightsClass,
    std::shared_ptr<DeviceTimeClass> deviceTime,
    std::shared_ptr<DataStorageClass> dataStorage,
    std::shared_ptr<ConfigManagerClass> configs
  ) : _lights(std::move(lightsClass)), _deviceTime(deviceTime), _dataStorage(dataStorage), _configsClass(configs), _configs(configs->getModalConfigs()) {
    _nextBackgroundTriggerTimeUTC_uS = _deviceTime->getUTCTimestampMicros();  // incase EventManager doesn't set any modes before update is called

    _nextBackgroundMode = 1;

    _interpClass->setTargetBrightness(_configs.minOnBrightness);
    _lightVals.state = true;

    // register adjustment callback with deviceTime
    _deviceTime->add_observer(*this);
  }

  /**
   * @brief sets the next mode by UUID, but doesn't load or swap it until update() is called. this allows EventManager to blast through it's event store, calling multiple different modes in chronological order, but only the most recent ones get loaded
   * 
   * @param modeID 
   * @param triggerTimeLocal_S 
   * @param isActive 
   */
  void setModeByUUID(modeUUID modeID, uint64_t triggerTimeLocal_S, bool isActive) override{
    // TODO: check with storage if modeID is valid
    if(!_dataStorage->doesModeExist(modeID)){
      return;
    }
    if(isActive){
      _nextActiveMode = modeID;
      _nextActiveTriggerTimeUTC_uS = _deviceTime->convertLocalToUTCMicros(triggerTimeLocal_S * secondsToMicros);
      return;
    }
    if(modeID != _backgroundMode){
      _nextBackgroundMode = modeID;
      _nextBackgroundTriggerTimeUTC_uS = _deviceTime->convertLocalToUTCMicros(triggerTimeLocal_S * secondsToMicros);
    }
  };

  /**
   * @brief updates the lights. if a new mode is pending, it'll load the new mode
   * 
   */
  void updateLights() override {
    // check if a new mode is pending
    if(_nextActiveMode != 0 || _nextBackgroundMode != 0){_loadMode();}
    
    // update
    uint64_t utcTime_uS = _deviceTime->getUTCTimestampMicros();
    _mode->updateLightVals(utcTime_uS, _lightVals);
    _lights->setChannelValues(_lightVals.getLightValues());
  };
  
  /**
   * @brief request a new brightness level, the mode will decide if it's in bounds. this change is sudden, so softOn = true
   * 
   * @param brightness the desired brightness
   * @return duty_t the new brightness after soft change
   */
  duty_t setBrightnessLevel(duty_t brightness) override {
    if(_nextActiveMode != 0 || _nextBackgroundMode != 0){_loadMode();}
    _mode->setBrightness(_deviceTime->getUTCTimestampMicros(), _lightVals, brightness, true);
    _lights->setChannelValues(_lightVals.getLightValues());
    return getSetBrightness();
  };
  
  bool setState(bool newState) override {
    if(!_isSetupComplete){
      updateLights();
      return true;
    }
    if(_mode->setState(_deviceTime->getUTCTimestampMicros(), _lightVals, newState)){
      cancelActiveMode();
    };
    _lights->setChannelValues(_lightVals.getLightValues());
    return _lightVals.state;
  }

  /**
   * @brief adjust the brightness by a given amount and immediately update. the change is assumed to be gradual, so softOn = false
   * 
   * @param amount
   * @param increasing 
   */
  duty_t adjustBrightness(duty_t amount, bool increasing) override {
    // return early if lights are off and amount is decreasing
    if(_nextActiveMode != 0 || _nextBackgroundMode != 0){_loadMode();}
    if(
      amount == 0
      || (!_lightVals.state && !increasing)
    ){return _lightVals.values[0];}

    const duty_t minB = _configs.minOnBrightness;
    duty_t newBrightness;
    if(increasing){
      duty_t oldBrightness = (_lightVals.state && (_lightVals.values[0] >= minB))
                              ? _lightVals.values[0]
                              : minB - 1; // if lights are below minB, start at minB-1. adjust up by 1 will equal 1
      newBrightness = amount >= (LED_LIGHTS_MAX_DUTY - oldBrightness) 
                      ? LED_LIGHTS_MAX_DUTY
                      : oldBrightness + amount;
    }
    else{
      duty_t oldBrightness = _lightVals.values[0];
      newBrightness = amount >= oldBrightness - minB
                      ? 0
                      : oldBrightness - amount;
    }
    // TODO: can the if statements be cleaned up by moving some logic into updateLights()?
    _mode->setBrightness(_deviceTime->getUTCTimestampMicros(), _lightVals, newBrightness, false);
    _lights->setChannelValues(_lightVals.getLightValues());
    return getBrightnessLevel();
  }

  /**
   * @brief Get the current brightness setting of the lights. not affected by state.
   * 
   * @return duty_t 
   */
  duty_t getSetBrightness() override {
    duty_t minB = _configs.minOnBrightness;
    // duty_t currentB = _mode->getTargetBrightness();
    duty_t currentB = _interpClass->getTargetBrightness();
    duty_t setB = currentB >= minB ? currentB : 0;
    return setB;
  };

  bool cancelActiveMode() override {
    if(_activeMode == 0){return false;}
    _activeMode = 0;
    _activeModeTriggerTimeUTC_uS = 0;
    _changeMode();
    updateLights();
    return true;
  };

  /**
   * @brief Get the current mode IDs
   * 
   * @return CurrentModeStruct
   */
  CurrentModeStruct getCurrentModes(){
    CurrentModeStruct currentModes = {
      .activeMode = _activeMode,
      .backgroundMode = _backgroundMode
    };
    return currentModes;
  }

  /**
   * @brief change the softChangeWindow config value and update lights. sets the new value in config manager
   * 
   * @param newWindow_S 
   * @return true 
   * @return false if newWindow is larger than a nibble
   */
  bool changeSoftChangeWindow(uint8_t newWindow_S){
    if(newWindow_S >= (1 << 4)){return false;}
    uint64_t utcTimestamp_uS = _deviceTime->getUTCTimestampMicros();
    _mode->changeSoftChangeWindow(newWindow_S, utcTimestamp_uS, _lightVals);
    _lights->setChannelValues(_lightVals.getLightValues());

    _configs.softChangeWindow = newWindow_S;
    _configsClass->setModalConfigs(_configs);
    return true;
  }

  /**
   * @brief change the minimum on brightness config value and update the lights. set the new value in config manager
   * 
   * @param newMinBrightness 
   * @return true 
   * @return false if new min brightness == 0
   */
  bool changeMinOnBrightness(duty_t newMinBrightness){
    if(newMinBrightness == 0){return false;}
    uint64_t utcTimestamp_uS = _deviceTime->getUTCTimestampMicros();
    _mode->changeMinOnBrightness(newMinBrightness, utcTimestamp_uS, _lightVals);
    _lights->setChannelValues(_lightVals.getLightValues());

    _configs.minOnBrightness = newMinBrightness;
    _configsClass->setModalConfigs(_configs);
    return true;
  }

  bool changeDefaultOnBrightness(duty_t newDefaultOnBrightness){
    _configs.defaultOnBrightness = newDefaultOnBrightness;
    _mode->changeDefaultOnBrightness(newDefaultOnBrightness);
    _configsClass->setModalConfigs(_configs);
    return true;
  }

  ModalConfigsStruct getConfigs(){
    return _configs;
  }

  void notification(const TimeUpdateStruct& timeUpdates){
    if(timeUpdates.utcTimeChange_uS != 0){
      _mode->timeAdjust(timeUpdates);
    }
  }
};

#endif