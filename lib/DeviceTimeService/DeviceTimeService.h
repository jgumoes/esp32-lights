/*
 * TODO: the core logic needs to be seperated from BLE logic
 * Device time will also need to be set using the internet
 */

#ifndef _DEVICE_TIME_SERVICE_H_
#define _DEVICE_TIME_SERVICE_H_

#include "RTC_interface.h"

/* bit values for the Device Time status flags (Device Time Service 3.3.1.5)*/
#ifndef DT_STATUS_TIME_FAULT
  #define DT_STATUS_TIME_FAULT 0x01
#endif
#ifndef DT_STATUS_UTC_ALIGNED
  #define DT_STATUS_UTC_ALIGNED 0x02
#endif
#ifndef DT_STATUS_QUALIFIED_LOCAL_TIME_SYNCH
  #define DT_STATUS_QUALIFIED_LOCAL_TIME_SYNCH 0x04
#endif
#ifndef DT_STATUS_PROPOSE_TIME_UPDATE_REQUEST
  #define DT_STATUS_PROPOSE_TIME_UPDATE_REQUEST 0x08
#endif
#ifndef DT_STATUS_EPOCH_YEAR
  #define DT_STATUS_EPOCH_YEAR 0X10
#endif
#ifndef DT_STATUS_NON_LOGGED_TIME_CHANGE_ACTIVE
  #define DT_STATUS_NON_LOGGED_TIME_CHANGE_ACTIVE 0x20
#endif
#ifndef DT_STATUS_LOG_CONSOLIDATION_ACTIVE
  #define DT_STATUS_LOG_CONSOLIDATION_ACTIVE 0x40
#endif

#ifndef DTCP_RESPONSE_SUCCESS
  #define DTCP_RESPONSE_SUCCESS 0X01
#endif
#ifndef DTCP_RESPONSE_OPCODE_NOT_SUPPORTED
  #define DTCP_RESPONSE_OPCODE_NOT_SUPPORTED 0X02
#endif
#ifndef DTCP_RESPONSE_INVALID_OPERAND
  #define DTCP_RESPONSE_INVALID_OPERAND 0X03
#endif
#ifndef DTCP_RESPONSE_PROCEDURE_FAILED
  #define DTCP_RESPONSE_PROCEDURE_FAILED 0x04
#endif
#ifndef DTCP_RESPONSE_PROCEDURE_REJECTED
  #define DTCP_RESPONSE_PROCEDURE_REJECTED 0x05
#endif
#ifndef DTCP_RESPONSE_PROCEDURE_DEVICE_BUSY
  #define DTCP_RESPONSE_PROCEDURE_DEVICE_BUSY 0x07
#endif

/* bit values for the Device Time Control Point Time Update Flags (BLE DTS 3.7.1.1.1.1)*/
#ifndef DTCP_TIME_UPDATE_UTC_ALIGNED
  #define DTCP_TIME_UPDATE_UTC_ALIGNED 0X01
#endif
#ifndef DTCP_TIME_UPDATE_QUALIFIED_LOCAL
  #define DTCP_TIME_UPDATE_QUALIFIED_LOCAL 0X02
#endif
#ifndef DTCP_TIME_UPDATE_MANUAL_ADJ
  #define DTCP_TIME_UPDATE_MANUAL_ADJ 0X04
#endif
#ifndef DTCP_TIME_UPDATE_EXTERNAL_ADJ
  #define DTCP_TIME_UPDATE_EXTERNAL_ADJ 0X08
#endif
#ifndef DTCP_TIME_UPDATE_TIMEZONE_ADJ
  #define DTCP_TIME_UPDATE_TIMEZONE_ADJ 0X10
#endif
#ifndef DTCP_TIME_UPDATE_DST_ADJ
  #define DTCP_TIME_UPDATE_DST_ADJ 0X20
#endif
#ifndef DTCP_TIME_UPDATE_EPOCH
  #define DTCP_TIME_UPDATE_EPOCH 0X40
#endif
#ifndef DTCP_TIME_UPDATE_SECOND_FRACTIONS_NOT_VALID
  #define DTCP_TIME_UPDATE_SECOND_FRACTIONS_NOT_VALID 0X80
#endif

/* bit values for the Device Time Control Point rejection flag (BLE DTS 3.7.1.2.2.2)*/
#ifndef DTCP_REJFLAG0_BASE_TIME_UNREALISTIC
  #define DTCP_REJFLAG0_BASE_TIME_UNREALISTIC 0X01  // the Base Time Update value is unrealistic
#endif
#ifndef DTCP_REJFLAG0_REQUEST_NOT_AUTHORIZED
  #define DTCP_REJFLAG0_REQUEST_NOT_AUTHORIZED 0X02  // client not authorized to make request
#endif
#ifndef DTCP_REJFLAG0_REQUIRED_OPERAND_OUT_OF_RANGE
  #define DTCP_REJFLAG0_REQUIRED_OPERAND_OUT_OF_RANGE 0X04
#endif
#ifndef DTCP_REJFLAG0_TIME_SOURCE_NOT_UTC_ALIGNED
  #define DTCP_REJFLAG0_TIME_SOURCE_NOT_UTC_ALIGNED 0X08
#endif
#ifndef DTCP_REJFLAG0_TIME_ACCURACY
  #define DTCP_REJFLAG0_TIME_ACCURACY 0X10
#endif
#ifndef DTCP_REJFLAG0_TIME_SOURCE_LOWER_QUALITY
  #define DTCP_REJFLAG0_TIME_SOURCE_LOWER_QUALITY 0X20
#endif
#ifndef DTCP_REJFLAG0_EPOCH_YEAR_WRONG
  #define DTCP_REJFLAG0_EPOCH_YEAR_WRONG 0X40
#endif
#ifndef DTCP_REJFLAG1_LACK_OF_PRECISION
  #define DTCP_REJFLAG1_LACK_OF_PRECISION 0X01
#endif
#ifndef DTCP_REJFLAG1_BASE_TIME_REJECTED_BUT_TZ_DST_ACCEPTED
  #define DTCP_REJFLAG1_BASE_TIME_REJECTED_BUT_TZ_DST_ACCEPTED 0X02
#endif
#ifndef DTCP_REJFLAG1_TZ_DST_REJECTED_BUT_BASE_TIME_ACCEPTED
  #define DTCP_REJFLAG1_TZ_DST_REJECTED_BUT_BASE_TIME_ACCEPTED 0X04
#endif

/*
 * helpful struct of DeviceTimeClass
 * keeps certain variables in a need little packet,
 * ready to be sent through the GATT server
 */
struct deviceTimeDataPacket {
  uint32_t baseTime;
  int8_t timeZone;
  uint8_t DSTOffset;
  uint16_t DTStatus;
};

/*
 * Responsible for setting and accessing time keeping through BLE
 */
class DeviceTimeClass{
  public:
    RTCInterfaceClass& _RTC;
    NimBLECharacteristic *deviceTimeCharInstance;
    DeviceTimeClass(RTCInterfaceClass& RTC);

    deviceTimeDataPacket dataPacket;
    
    uint32_t getBaseTime();

    bool setTimeFromUTC(uint32_t timestamp);

    bool setTimezoneOffset(int8_t timezone);

    bool setDSTOffset(uint8_t offset);

    bool updateRTC();

    void raiseTimeFault();
    void resetTimeFault();
    
    void setProposeTimeUpdateRequest(bool ptur);

    bool commitChanges();

    uint16_t getDTStatus();

    void indicateDeviceTime();
    void indicateDeviceTime(NimBLECharacteristic* deviceTimeChar);
    void readDeviceTime(NimBLECharacteristic* deviceTimeChar);

    void updateDTStatus(uint16_t update);

  private:

    bool _timeFault = 0;
};

extern DeviceTimeClass DeviceTime;
// extern DeviceTimeControlPointClass DTCP;

void setupDeviceTimeService(NimBLEServer *bleServer);

#endif