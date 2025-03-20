#ifndef __DEVICE_TIME_NO_RTC__
#define __DEVICE_TIME_NO_RTC__

#include "DeviceTimeInterface.h"

class DeviceTimeNoRTCClass : public DeviceTimeInterface{
  public:
    DeviceTimeNoRTCClass(std::shared_ptr<ConfigManagerClass> configManager)
      : DeviceTimeInterface(configManager)
    {
      _onboardTimestamp->setTimestamp_uS(BUILD_TIMESTAMP);
      _timeFault = true;
    }

    uint64_t getUTCTimestampMicros() override;

     /**
     * @brief sets the UTC timestamp from 2000 epoch. Timezone and DST are in seconds
     * 
     * @param newTimesamp in seconds
     * @param timezone in seconds
     * @param DST in seconds
     * @return if operation was successful
     */
    bool setUTCTimestamp2000(uint64_t newTimesamp, int32_t timezone, uint16_t DST) override;
};

#endif