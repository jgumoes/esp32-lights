#ifndef _CONFIG_MANAGER_H_
#define _CONFIG_MANAGER_H_

#include "RTC_interface.h"

class ConfigManagerClass
{
public:
  ConfigManagerClass();

  RTCConfigsStruct getRTCConfigs();
  uint8_t setRTCConfigs(RTCConfigsStruct configs);

private:
  
};


#endif