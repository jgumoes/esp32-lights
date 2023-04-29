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

#include "DeviceTimeService.h"
#include "user_config.h"

// setup Device Time Service and associated characteristics
BLEService deviceTimeService = BLEService(UUID16_SVC_DEVICE_TIME);
BLECharacteristic deviceTimeFeature = BLECharacteristic(UUID16_CHR_DEVICE_TIME_FEATURE);
BLECharacteristic deviceTimeParameters = BLECharacteristic(UUID16_CHR_DEVICE_TIME_PARAMETERS);
BLECharacteristic deviceTime = BLECharacteristic(UUID16_CHR_DEVICE_TIME);
BLECharacteristic deviceTimeControlPoint = BLECharacteristic(UUID16_CHR_DEVICE_TIME_CONTROL_POINT);
// BLECharacteristic timeChangeLogData = BLECharacteristic(UUID16_CHR_TIME_CHANGE_LOG_DATA);
// BLECharacteristic recordAccessControlPoint = BLECharacteristic(UUID16_CHR_RECORD_ACCESS_CONTROL_POINT);


/* DeviceTimeClass */
/* public methods*/

/*
 * Responsible for time keeping
 * DS3231 doesn't support dst or timezones on the chip
 * those will have to be handled by this class & saved to file
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
  indicateDeviceTime();
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
  deviceTime.indicate(&dataPacket, 8);
}

/*
 * writes a dataPacket to the GATT server
 * TODO: when a device connects, this must be called every second by a low-priority scheduled task
 */
void DeviceTimeClass::writeDeviceTime(){
  dataPacket.baseTime = getBaseTime(); 
  deviceTime.write(&dataPacket, 8);
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
    void handleCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* dataPacket, uint16_t len){
      if (handelingResponse == 1){
        response(0X07);

        #ifdef DEBUG_PRINT_DEVICE_TIME_CP_ENABLE
          Serial.println("Device Time Control Point: a write request is already being handled");
        #endif
        return;
      }; // reject packet if request is already being handled
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
    bool unpackTimeUpdateData(uint8_t* dataPacket, uint16_t len){
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
      deviceTimeControlPoint.indicate(responseObject, 3);
    };
    
    /*
    * rejFlag1 and rejFlag0 must be set before calling
    */
    void rejectProcedure(){
      uint8_t responseObject[5] = { 0x09, opcode, 0x05, rejFlag1, rejFlag0 }; // DTCP response, request opcode, response code,
      deviceTimeControlPoint.indicate(responseObject, 5);
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


DeviceTimeClass DeviceTime = DeviceTimeClass(RTCInterface);
DeviceTimeControlPointClass DTCP = DeviceTimeControlPointClass();

void deviceTimeControlPointWriteCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len){

  #ifdef DEBUG_PRINT_DEVICE_TIME_CP_ENABLE
    Serial.println("Device Time Control Point write request has been made");
  #endif

  // should throw an error if a client writes without enabling indicate, but doesn't
  if ( !chr->indicateEnabled(conn_hdl) )
  {
    ble_gatts_rw_authorize_reply_params_t reply = { .type = BLE_GATTS_AUTHORIZE_TYPE_WRITE };
    reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_CPS_CCCD_CONFIG_ERROR;
    sd_ble_gatts_rw_authorize_reply(conn_hdl, &reply);

    #ifdef DEBUG_PRINT_DEVICE_TIME_CP_ENABLE
      Serial.println("Write request rejected (indicate CCCD wasn't written)");
    #endif
    return;
  }
  // the actual callback
  #ifdef DEBUG_PRINT_DEVICE_TIME_CP_ENABLE
    Serial.println("Write request accepted");
  #endif
  DTCP.handleCallback(conn_hdl, chr, data, len);
}

void deviceTimeIndicateCallback(uint16_t connHdl, BLECharacteristic* chr, uint16_t cccdValue){
  if (chr->indicateEnabled(connHdl)){
    DeviceTime.indicateDeviceTime();  // indicate when indicate is enabled (as per DTS standard)
  }
  #ifdef DEBUG_PRINT_DEVICE_TIME_ENABLE
    Serial.print("Device Time indicate CCCD set to: "); Serial.println(chr->indicateEnabled(connHdl));
  #endif
}

void setupDeviceTimeService(void){
  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time service");
  #endif
  deviceTimeService.begin();

  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time feature characteristic");
  #endif
  deviceTimeFeature.setProperties(CHR_PROPS_READ);
  deviceTimeFeature.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  deviceTimeFeature.setFixedLen(4);
  deviceTimeFeature.begin();
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
  uint16_t dtfData[2] = { 0xFFFF, 0b0000010010000000 };
  deviceTimeFeature.write(dtfData, 4);

  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time parameters characteristic");
  #endif
  deviceTimeParameters.setProperties(CHR_PROPS_READ);
  deviceTimeParameters.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  deviceTimeParameters.setFixedLen(2);
  deviceTimeParameters.begin();
  deviceTimeParameters.write16(0xFFFF);  // only clock resolution gets written

  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time characteristic");
  #endif
  deviceTime.setProperties( CHR_PROPS_READ | CHR_PROPS_INDICATE);
  deviceTime.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  deviceTime.setFixedLen(8);
  deviceTime.setCccdWriteCallback(deviceTimeIndicateCallback);
  deviceTime.begin();

  #ifdef DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
    Serial.println("setting up device time control point characteristic");
  #endif
  deviceTimeControlPoint.setProperties(CHR_PROPS_WRITE | CHR_PROPS_INDICATE);
  deviceTimeControlPoint.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  deviceTimeControlPoint.setMaxLen(18);
  // deviceTimeControlPoint.setCccdWriteCallback(dtcpCccdWriteCallback);
  deviceTimeControlPoint.setWriteCallback(deviceTimeControlPointWriteCallback);
  deviceTimeControlPoint.begin();
}