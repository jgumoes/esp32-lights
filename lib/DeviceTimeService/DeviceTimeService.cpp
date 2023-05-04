/*
 * service specifications: https://www.bluetooth.com/specifications/specs/device-time-service-1-0/
 *
 * there are a couple of known issues:
 * writing to the DTCP without turning indications on should throw an error, but i haven't worked
 * out how to do that yet (the error-throwing function in the core library doesn't seem to be
 * declared or defined anywhere, i'm not sure what kind of c++ witchcraft makes that possible).
 * 
 * The Device Time data packet doesn't get updated when a client tries to read it. I'm not sure how
 * the BLE read process works, so I'm not able to sneak in a callback to update time on-demand.
 * it'll have to be a background process with freeRTOS
 * 
 * TODO: the dts should know the UTC timestamp of the next DST change
 *
 * TODO: Device Time characteristic behavior (3.3.1)
 *    - test: server shall indicate immediately when client enables indications
 *    - TODO: server shall indicate when significant change occors (i.e. abnormal change in time, any change to any other variable)
 *            The server shouldn't indicate to a client that's just performed the Time Update procedure, but can to other clients 
 *    - TODO: monitor for abnormal changes in time
 *
 * TODO: Common behavior of Time Update procedures (3.7.2.1)
 * TODO: Common Time Update behavior for success and failure
 * TODO: Common Time Update behavior and Base_Time
 * TODO: Common Time Update behavior and Base Time Second-Fractions
 * TODO: Common Time Update behavior and epoch year support
 * 
 * TODO:  Propose Time Update procedure (3.7.2.2)
 * TODO: Force Time Update procedure
 * TODO: Propose Non-Logged Time Adjustment Limit procedure
 * TODO: Retrieve Active Time Adjustments procedure
 *
 * TODO: handle server busy error (3.5.1)
 * TODO: time update needs to be timed and monitored
 * TODO: implement End-to-End Cyclic Redundancy Check (E2E CRC)
 * TODO: implement authorization
 */
#include <NimBLEDevice.h>

#include "DeviceTimeService.h"
#include "user_config.h"

NimBLEService *deviceTimeService;
NimBLECharacteristic *deviceTimeFeatureChar;
NimBLECharacteristic *deviceTimeParametersChar;
NimBLECharacteristic *deviceTimeChar;
NimBLECharacteristic *deviceTimeControlPointChar;

/* DeviceTimeClass */

/* public methods*/

/*
 * Responsible for time keeping
 * DS3231 doesn't support dst or timezones on the chip
 * those will have to be handled by this class & saved to file
 * // TODO: this should 1000% be handled in the RTC interface class
 * @param RTC an instance of the RTCInterfaceClass
 */
DeviceTimeClass::DeviceTimeClass(RTCInterfaceClass& RTC)
  : _RTC{ RTC }
{
  // TODO: read timeZone and DSTOffset from file
  // TODO: initial DTStatus flags should be established here
  dataPacket.timeZone = 0;
  dataPacket.DSTOffset = 0;
  dataPacket.DTStatus = DT_STATUS_EPOCH_YEAR;
}

/*
 * returns the UTC timestamp with 2000 epoch, unadjusted for timezone and DST
 */
uint32_t DeviceTimeClass::getBaseTime(){
  return _RTC.getUTCTimestamp();
}

/*
 * @param timestamp UTC unadjusted timestamp in seconds since midnight 1/1/2000
 * @note the timezone and DST offsets must already be 
 * @note - no sanity checks are done here
 * @note - commitChanges() will need to be called to save changes to filesystem
 */
bool DeviceTimeClass::setTimeFromUTC(uint32_t timestamp){
  // note: no sanity checks are to be completed here for (DTS BLE compliance)
  #ifdef DEBUG_PRINT_DEVICE_TIME_ENABLE
    Serial.print("Device Time setting time from UTC: "); Serial.println(timestamp);
  #endif
  _RTC.setUTCTimestamp(timestamp);
  resetTimeFault(); // TODO: check the time again after the RTC has been updated
  return 1;
}

/*
 * sets the timezone offset
 * @param timezone timezone offset in 15 minute increments
 * @return true if change accepted, false if out of range
 * @note - commitChanges() will need to be called to save changes to filesystem
 */
bool DeviceTimeClass::setTimezoneOffset(int8_t timezone){
  if (timezone <= 56 & timezone >= -48){
    dataPacket.timeZone = timezone;
   _RTC.setTimezoneOffset(timezone * 15 * 60);  // convert timezone offset into seconds

    #ifdef DEBUG_PRINT_DEVICE_TIME_ENABLE
      Serial.print("Device Time setting timezone: "); Serial.println(timezone);
    #endif
    return 1;
  }
  return 0;
}

/*
 * sets DST offset
 * @param offset accepts a byte code as outlined in BLE protocol
 * @return true if change accepted, false if offset is invalid
 * @note - commitChanges() will need to be called to save changes to filesystem
 */
bool DeviceTimeClass::setDSTOffset(uint8_t offset){
  dataPacket.DSTOffset = offset;
  uint16_t seconds;
  switch(offset){ // convert offset into seconds
    case 0:
      // no offset
      seconds = 0;
      break;
    case 2:
      // half an hour (+ 0.5h)
      seconds = 30 * 60;
      break;
    case 4:
      // daylight time (+ 1h)
      seconds = 60 * 60;
      break;
    case 8:
      // double daylight time (+ 2h)
      seconds = 2 * 60 * 60;
      break;
    default:
      return 0;
      break;
  };
  _RTC.setDSTOffset(seconds);
  #ifdef DEBUG_PRINT_DEVICE_TIME_ENABLE
    Serial.print("Device Time setting DST: "); Serial.println(offset);
  #endif
  return 1;
};

/*
 * sets time fault bit in the DT status flags,
 * and sets time-source quality bits to 0.
 * immediately indicates as per BLE DTS standard
 */
void DeviceTimeClass::raiseTimeFault(){
  dataPacket.DTStatus |= (DT_STATUS_TIME_FAULT | DT_STATUS_PROPOSE_TIME_UPDATE_REQUEST); // raise time fault and ask for a time update
  dataPacket.DTStatus &= ~(DT_STATUS_UTC_ALIGNED | DT_STATUS_QUALIFIED_LOCAL_TIME_SYNCH); // set time quality bits to 0;
  indicateDeviceTime(deviceTimeCharInstance);
}

/*
 * resets the time fault and propose time update bits in the DT status
 * @note should only be called after a client updates the time following a time fault
 */
void DeviceTimeClass::resetTimeFault(){
  dataPacket.DTStatus &= ~(DT_STATUS_TIME_FAULT | DT_STATUS_PROPOSE_TIME_UPDATE_REQUEST);
}

void DeviceTimeClass::setProposeTimeUpdateRequest(bool ptur){
  dataPacket.DTStatus |= ( ptur * DT_STATUS_PROPOSE_TIME_UPDATE_REQUEST);
  indicateDeviceTime();
}

/*
 * saves parameter changes to file
 * TODO: save timezone and DSToffset to file
 */
bool DeviceTimeClass::commitChanges(){
  // indicateDeviceTime();  // do not call, as indicating to the client that's performed a time update breaks DST protocol
  _RTC.updateTime();
  // TODO: check the time update worked
  // TODO: save offsets to file
  return 1;
}

/*
 * @return the Device Time Status flags
 */
uint16_t DeviceTimeClass::getDTStatus(){
  return dataPacket.DTStatus;
}

/*
 * updates the device time status flags
 * @param update flag mask to add to the status
 * @note this method will not remove flags, only raise them
 * @note TODO: is this actually necessary?
 */
void DeviceTimeClass::updateDTStatus(uint16_t update){
  dataPacket.DTStatus |= update;
}

/*
 * indicates dataPacket to the GATT server
 * @note manually tested to work OK
 */
void DeviceTimeClass::indicateDeviceTime(){
  dataPacket.baseTime = getBaseTime();
  deviceTimeCharInstance->indicate((const uint8_t*)&dataPacket, 8);
}
/*
 * indicates dataPacket to the GATT server
 * @note manually tested to work OK
 */
void DeviceTimeClass::indicateDeviceTime(NimBLECharacteristic* deviceTimeChar){
  deviceTimeCharInstance = deviceTimeChar;
  indicateDeviceTime();
}


void DeviceTimeClass::readDeviceTime(NimBLECharacteristic* deviceTimeChar){
  dataPacket.baseTime = getBaseTime(); 
  deviceTimeChar->setValue((const uint8_t*)&dataPacket, 8);
}

/* Device Time Class private methods */

/* DeviceTimeControlPointClass */
class DeviceTimeControlPointClass {
  public:
    static bool handelingResponse;  // this variable is shared accross all class instances
                                    // this is essentially a global variable wrapped in a class namespace
                                    // i don't know if other instances can/will be created (they shouldn't), but this is static just in case

    DeviceTimeControlPointClass(){ reset(); }

    /*
     * handles the write callback for the Device Time Control Point characteristic
     * This method should ideally be the single entry- and exit-point of the class.
     */
    void handleCallback(NimBLEAttValue value, uint16_t len){
      if (handelingResponse == 1){
        response(0X07);

        #ifdef DEBUG_PRINT_DEVICE_TIME_CP_ENABLE
          Serial.println("Device Time Control Point: a write request is already being handled");
        #endif
        return;
      }; // reject packet if request is already being handled
      const uint8_t* dataPacket = value.data();
      handelingResponse = 1;
      opcode = dataPacket[0];
      switch (opcode){
        case 2:
          // Propose Time Update
          if(unpackTimeUpdateData(dataPacket, len)){updateTime(false);}
          break;
        case 3:
          // force time update
          // TODO: implement authorization
          // updateTime(true);
          #ifdef TESTING_ENV
            updateTime(true);
          #else
            rejFlag0 |= DTCP_REJFLAG0_REQUEST_NOT_AUTHORIZED;
            rejectProcedure();
          #endif
          break;
        default:
          response(DTCP_RESPONSE_OPCODE_NOT_SUPPORTED);
          break;
      };
      reset();
    };
  
  private:

    /*
     * unpacks the incoming data into actual variables
     */
    bool unpackTimeUpdateData(const uint8_t* dataPacket, uint16_t len){
      if(len > 11){
        response(DTCP_RESPONSE_INVALID_OPERAND);
        #ifdef DEBUG_PRINT_DEVICE_TIME_CP_ENABLE
          Serial.println("Device Time Control Point: operand is too long");
        #endif
        return false;
      }
      timeUpdateFlags[0] = dataPacket[2]; // LSB first
      timeUpdateFlags[1] = dataPacket[1];
      baseTimeUpdate = (dataPacket[6] << 24) | (dataPacket[5] << 16) |  (dataPacket[4] << 8) | dataPacket[3];
      timezoneUpdate = dataPacket[7];
      DSTOffsetUpdate = dataPacket[8];
      timeSourceUpdate = dataPacket[9];
      timeAccuracyUpdate = dataPacket[10];

      #ifdef DEBUG_PRINT_DEVICE_TIME_CP_ENABLE
        Serial.print("received dataPacket: { ");
        for (int i = 0; i <= 9; i++){
          Serial.print(dataPacket[i]); Serial.print(", ");
        }
        Serial.print(dataPacket[10]); Serial.println("}");
      #endif
      return true;
    }

    /*
    * checks and updates the time
    * @param forceUpdate if true, the time will update regardless of time source quality (provided the epoch is correct)
    */
    void updateTime(bool forceUpdate){
      uint8_t flags = timeUpdateFlags[1];
      bool tz = 1; bool dst = 1; bool ts = 0; // default tz and dst to true, as no-change-requested will always be accepted

      if( flags & DTCP_TIME_UPDATE_TIMEZONE_ADJ ){ tz = DeviceTime.setTimezoneOffset(timezoneUpdate); };
      if( flags & DTCP_TIME_UPDATE_DST_ADJ ){ dst = DeviceTime.setDSTOffset(DSTOffsetUpdate); }

      // if( timestamp dumb ){ rejFlag0 |= DTCP_REJFLAG0_BASE_TIME_UNREALISTIC; } // TODO: reject if new timestamp is unrealistic
      if( !(flags & 1) && (DeviceTime.getDTStatus() & DT_STATUS_UTC_ALIGNED) ) { rejFlag0 |= DTCP_REJFLAG0_TIME_SOURCE_NOT_UTC_ALIGNED; } // reject if Time source is not UTC aligned (and Server is aligned).
      if( timeAccuracyUpdate >= 240){ rejFlag0 |= DTCP_REJFLAG0_TIME_ACCURACY; }  // reject if new timestamp has a drift of over 30s
      // if( quality bad ){ rejFlag0 |= DTCP_REJFLAG0_TIME_SOURCE_LOWER_QUALITY; }  // TODO: reject if the time source quality is worse
      if( (flags & 0x40) == 0){ rejFlag0 |= DTCP_REJFLAG0_EPOCH_YEAR_WRONG;}  // reject if wrong epoch is used
      
      if( (!rejFlag0) || (forceUpdate && !(rejFlag0 & DTCP_REJFLAG0_EPOCH_YEAR_WRONG)) ){
        // if no time-breaking flags have been raised,
        // or if forceUpdate is true (and the epoch is correct)
        // update the timestamp
        ts = DeviceTime.setTimeFromUTC(baseTimeUpdate);
        if(ts){DeviceTime.updateDTStatus( (flags & 3) << 1);}
      }
      
      if ( !(tz && dst) ){ rejFlag0 |= DTCP_REJFLAG0_REQUIRED_OPERAND_OUT_OF_RANGE; } // raise value out of range flag
      if ( tz && dst && (!ts) ){ rejFlag1 |= DTCP_REJFLAG1_BASE_TIME_REJECTED_BUT_TZ_DST_ACCEPTED;}  // accept tz+dst but reject ts
      else if ( ts && (!tz) && (!dst) ){ rejFlag1 |= DTCP_REJFLAG1_TZ_DST_REJECTED_BUT_BASE_TIME_ACCEPTED; }  // accept ts but reject tz&dst
      
      if( !DeviceTime.commitChanges()){ response(DTCP_RESPONSE_PROCEDURE_FAILED); }
      else if( rejFlag0 || rejFlag1){ rejectProcedure(); }
      else {response(DTCP_RESPONSE_SUCCESS);} // respond with success
    };

    /*
     * resets the incomming data values, and sets handelingResponse to 0
     */
    void reset(){
      handelingResponse = 0;
      opcode = 0;
      timeUpdateFlags[0] = 0;
      timeUpdateFlags[1] = 0;
      baseTimeUpdate = 0;
      timezoneUpdate = 0;
      DSTOffsetUpdate = 0;
      timeSourceUpdate = 0;
      timeAccuracyUpdate = 0;
      rejFlag1 = 0;
      rejFlag0 = 0;
    }

    /*
     * respond with a single response code
     */
    void response(uint8_t responseCode){
      uint8_t responseObject[3] = { 0x09, opcode, responseCode }; // DTCP response, request opcode, response code
      deviceTimeControlPointChar->indicate(responseObject, 3);
    };
    
    /*
    * rejFlag1 and rejFlag0 must be set before calling
    */
    void rejectProcedure(){
      uint8_t responseObject[5] = { 0x09, opcode, 0x05, rejFlag1, rejFlag0 }; // DTCP response, request opcode, response code,
      deviceTimeControlPointChar->indicate(responseObject, 5);
      handelingResponse = 0;
    }
  
    uint8_t opcode;
    uint8_t timeUpdateFlags[2] = {};
    uint32_t baseTimeUpdate;
    int8_t timezoneUpdate;
    uint8_t DSTOffsetUpdate;
    uint8_t timeSourceUpdate;
    uint8_t timeAccuracyUpdate;

    uint8_t rejFlag1; // the procedure rejection flags bits 15-8
    uint8_t rejFlag0; // the procedure rejection flags bits 7-0
};

bool DeviceTimeControlPointClass::handelingResponse = 0;  // must be initiated outside of class, after class definition

/* Declare the classes*/
DeviceTimeClass DeviceTime = DeviceTimeClass(RTCInterface);
DeviceTimeControlPointClass DTCP = DeviceTimeControlPointClass();

/* Create callback classes */
class DeviceTimeCallbacksClass : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* pCharacteristic){
    Serial.println("Device Time is being read");
    DeviceTime.readDeviceTime(pCharacteristic);
  }

  void onIndicate(NimBLECharacteristic* pCharacteristic){
    Serial.println("Device Time is being indicated");
    DeviceTime.indicateDeviceTime(pCharacteristic);  // indicate when indicate is enabled (as per DTS standard)
  }
};

class DeviceTimeControlPointCallbacksClass : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* deviceTimeControlPointChar){
    Serial.println("Device Time Control Point is getting written to");
    DTCP.handleCallback(deviceTimeControlPointChar->getValue(), deviceTimeControlPointChar->getDataLength());
  }
};

template<std::size_t dataSize>
/*
 * pass data using (const uint8_t*)data
 */
class ReadOnlyCallbacksClass : public NimBLECharacteristicCallbacks {
  // const size_t dataLength;
  uint8_t data[dataSize];

  public:
  ReadOnlyCallbacksClass(uint8_t *value) {
    for(int i=0;i<dataSize;++i){
      data[i] = value[i];
    }
  }

  void onRead(NimBLECharacteristic* pCharacteristic){
    Serial.println("Device Time is being read");
    pCharacteristic->setValue(data, dataSize);
  }
};


static DeviceTimeCallbacksClass DeviceTimeCallbacks;
static DeviceTimeControlPointCallbacksClass DeviceTimeControlPointCallbacks;

#define dtfDataSize 4
// 0: E2E-CRC                             = 0 (TODO: implement)
// 1: Time Change Logging                 = 0 (TODO: implement)
// 2: Base Time use fractions of second   = 0
// 3: Time or Date Displayed to User      = 0
// 4: Displayed Formats                   = 0
// 5: Displayed Formats Changeable        = 0
// 6: Seperate User Timeline              = 0 (may be needed for timezones and DST)
// 7: Authorization Required              = 0 (TODO: implement)
// 8: RTC Drift Tracking                  = 0
// 9: Use 1900 as epoch year              = 0
// 10: use 2000 as epoch year             = 1
// 11: Propose Non-Logged Time Adjustment = 0
// 12: Retrieve Active Time Adjustments   = 0
// 13 - 15: Reserved
uint8_t dtfData[dtfDataSize] = { 0xFF, 0xFF, 0b00000100, 0b10000000 };
static ReadOnlyCallbacksClass<dtfDataSize> DeviceTimeFeatureCallback(dtfData);

#define timeParamsSize 2
uint8_t timeParams[timeParamsSize] = {0xFF, 0xFF};
static ReadOnlyCallbacksClass<timeParamsSize> DeviceTimeParametersCallback(timeParams);

void setupDeviceTimeService(NimBLEServer *bleServer){
  // create the service
  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time service");
  #endif
  deviceTimeService = bleServer->createService(NimBLEUUID((uint16_t)0x1847));

  // setup Device Time Service and associated characteristics
  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time feature characteristic");
  #endif
  deviceTimeFeatureChar = deviceTimeService->createCharacteristic(NimBLEUUID((uint16_t)0x2B8E), READ, dtfDataSize);
  deviceTimeFeatureChar->setCallbacks(&DeviceTimeFeatureCallback);

  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time parameters characteristic");
  #endif
  deviceTimeParametersChar = deviceTimeService->createCharacteristic(NimBLEUUID((uint16_t)0x2B8F), READ, timeParamsSize);
  deviceTimeParametersChar->setCallbacks(&DeviceTimeParametersCallback);

  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time characteristic");
  #endif
  deviceTimeChar = deviceTimeService->createCharacteristic(NimBLEUUID((uint16_t)0x2B90), READ | INDICATE, 8);
  deviceTimeChar->setCallbacks(&DeviceTimeCallbacks);

  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time control point characteristic");
  #endif
  deviceTimeControlPointChar = deviceTimeService->createCharacteristic(NimBLEUUID((uint16_t)0x2B91), WRITE | INDICATE, 18);
  deviceTimeControlPointChar->setCallbacks(&DeviceTimeControlPointCallbacks);

  // timeChangeLogData =  = deviceTimeService->createCharacteristic(NimBLEUUID((uint16_t)0x2B92), NOTIFY);
  // recordAccessControlPoint =  = deviceTimeService->createCharacteristic(NimBLEUUID((uint16_t)0x2A52), WRITE | INDICATE);
  deviceTimeService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(deviceTimeService->getUUID());
}