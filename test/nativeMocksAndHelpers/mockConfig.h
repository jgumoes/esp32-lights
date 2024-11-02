// #include <ArduinoFake.h>

#include "ConfigManager.h"

class MockConfigHal : public ConfigAbstractHAL{
  private:
    ConfigsStruct _allConfigs;
  
  public:
    ~MockConfigHal() override {};
    ConfigsStruct getAllConfigs() override {
      return _allConfigs;
    }
  
    RTCConfigsStruct getRTCConfigs() override {
      RTCConfigsStruct rtcConfigs;
      rtcConfigs.DST = _allConfigs.DST;
      rtcConfigs.timezone = _allConfigs.timezone;
      return rtcConfigs;
    };

    bool setRTCConfigs(RTCConfigsStruct rtcConfigs) override {
      _allConfigs.DST = rtcConfigs.DST;
      _allConfigs.timezone = rtcConfigs.timezone;
      return true;
    };
};
