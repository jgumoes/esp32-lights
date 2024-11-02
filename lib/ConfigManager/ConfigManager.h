#ifndef __CONFIGMANAGER_H__
#define __CONFIGMANAGER_H__

#include <memory>
#include "RTC_interface.h"

/**
 * the config hal should get the entire configs array when initialised. the array should populate an ordered map. 
 * 
 * for FRAM/EEPROM:
 *  - the config hal should store the address of each value in a second map. then, when writing, only the necessery addresses are written to
 * 
 */

struct ConfigsStruct
{
  int32_t timezone = 0;  // timezone offset in seconds
  uint16_t DST = 0;      // daylight savings offset in seconds
};


/**
 * @brief abstraction for the config HAL. will potentially target esp32 Preferences library, EEPROM/FRAM, etc. should only act as a helper class for writting and loading configs
 * 
 */
class ConfigAbstractHAL{
  public:
    virtual ~ConfigAbstractHAL() = default;
    virtual ConfigsStruct getAllConfigs() = 0;

    virtual RTCConfigsStruct getRTCConfigs() = 0;
    virtual bool setRTCConfigs(RTCConfigsStruct rtcConfigs) = 0;
};

template <typename ConcreteClass>
std::unique_ptr<ConfigAbstractHAL> makeConcreteConfigHal(){
  return std::make_unique<ConcreteClass>();
}

// should store the config structs locally for quick reference. setters should set the local config, then commit to storage. checking that configs are stored correctly is the responsibility of the concrete HAL
/**
 * @brief Accesses class for the stored configs. The configHAL dependancy must be injected through the concrete factory because C++ can't just be normal. i.e. to create a new instance: ConfigManagerClass configs(makeConcreteConfigHal<ConcreteConfigHal>())
 * 
 */
class ConfigManagerClass {
  private:
    std::unique_ptr<ConfigAbstractHAL> _configHAL;
    ConfigsStruct _configs;
  public:
    // ConfigManagerClass(ConfigAbstractHAL& configHAL) : _configHAL(std::make_unique<ConfigAbstractHAL>(configHAL)){
    //   _configs = _configHAL->getAllConfigs();
    // };

    ConfigManagerClass(std::unique_ptr<ConfigAbstractHAL> configHAL) : _configHAL(std::move(configHAL)){
      // _configHAL = std::move(configHAL);
      _configs = _configHAL->getAllConfigs();
    };

    // RTC_interface configs
    RTCConfigsStruct getRTCConfigs();
    bool setRTCConfigs(RTCConfigsStruct rtcConfigs);
};

#endif