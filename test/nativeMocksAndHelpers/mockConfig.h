#ifndef __MOCK_CONFIG_H__
#define __MOCK_CONFIG_H__
// #include <ArduinoFake.h>
#include "ConfigManager.h"

class MockConfigHal : public ConfigAbstractHAL{
  private:
    ConfigsStruct _allConfigs;

    bool _simulateHardwareFault = 0;
  public:
    // MockConfigHal(ConfigsStruct configs) : _allConfigs(configs){};
    MockConfigHal(){};
    ~MockConfigHal() override {};
    ConfigsStruct getAllConfigs() override {
      return _allConfigs;
    }

    bool reloadConfigs() override {return !_simulateHardwareFault;};
  
    // RTCConfigsStruct getRTCConfigs() override {
    //   RTCConfigsStruct rtcConfigs;
    //   rtcConfigs.DST = _allConfigs.DST;
    //   rtcConfigs.timezone = _allConfigs.timezone;
    //   return rtcConfigs;
    // };

    // bool setRTCConfigs(RTCConfigsStruct rtcConfigs) override {
    //   _allConfigs.DST = rtcConfigs.DST;
    //   _allConfigs.timezone = rtcConfigs.timezone;
    //   return true;
    // };

    bool setConfigs(ConfigsStruct configs) override {
      if(_simulateHardwareFault){
        return false;
      }
      _allConfigs = configs;
      return true;
    }
};

static auto makeTestConfigManager(EventManagerConfigsStruct initialConfigs){
  ConfigsStruct configs;
  configs.eventConfigs = initialConfigs;
  auto mockConfigHal = std::make_unique<MockConfigHal>();
  mockConfigHal->setConfigs(configs);
  std::shared_ptr<ConfigManagerClass> configsManager = std::make_shared<ConfigManagerClass>(std::move(mockConfigHal));
  return configsManager;
};

static auto makeTestConfigManager(){
  EventManagerConfigsStruct defaultConfigs;
  return makeTestConfigManager(defaultConfigs);
};

#endif