#pragma(once)

#include "moduleIDs.h"
#include "lightDefines.h"

typedef uint8_t packetSize_t;


namespace ConfigStructFncs{
  static errorCode_t defaultInvalid(const byte *dataArray){return errorCode_t::failed;};
  typedef errorCode_t(*validationFunc)(const byte*);

  class ConfigValidatorClass{
    private:
      const uint8_t _validationsSize = static_cast<uint8_t>(ModuleID::max)-1;
      validationFunc _validations[static_cast<uint8_t>(ModuleID::max)-1];

    public:
      ConfigValidatorClass(){
        for(uint16_t i = 0; i < _validationsSize; i++){
          _validations[i] = defaultInvalid;
        }
      };

      bool setValidationFunction(ModuleID type, validationFunc func){
        const uint8_t intType = static_cast<uint8_t>(type);
        if((intType < 1) || (intType > _validationsSize)){
          return false;
        }
        _validations[intType - 1] = func;
        return true;
      }

      /**
       * @brief defaults to errorCode_t::badID
       * 
       * @param dataArray 
       * @return errorCode_t 
       */
      errorCode_t isValid(const byte *dataArray){
        if(
          (dataArray[0] > _validationsSize)
          || (dataArray[0] == 0)
        ){
          return errorCode_t::badID;
        }
        return _validations[dataArray[0]-1](dataArray);
      }
  };
}

/*################################################

    configs for mandatory modules

################################################*/

struct TimeConfigsStruct{
  public:
  int32_t timezone = 0;  // timezone offset in seconds
  uint16_t DST = 0;      // daylight savings offset in seconds
  uint32_t maxSecondsBetweenSyncs = 60*60*24; // maximum time until a new sync is required

  // size of serialized data, excluding ID and checksum
  static const packetSize_t rawDataSize = sizeof(timezone)
                                 + sizeof(DST)
                                 + sizeof(maxSecondsBetweenSyncs);

  const ModuleID ID = ModuleID::deviceTime;
  
  /**
   * @brief checks the validity of the serialized data
   * 
   * @param dataArray 
   * @return errorCode_t
   * @retval errorCode_t::success
   * @retval errorCode_t::bad_time
   */
  static errorCode_t isDataValid(const byte dataArray[rawDataSize + 1]){
    uint8_t index = 1;
    const int32_t vTimezone = *reinterpret_cast<const int32_t*>(&dataArray[index]);
    if((vTimezone < -48*15*60) || (vTimezone > 56*15*60) || ((vTimezone % (15*60)) != 0)){
      return errorCode_t::bad_time;
    }
    index += sizeof(timezone);
    
    const uint16_t vDST = *reinterpret_cast<const uint16_t*>(&dataArray[index]);
    if(!((vDST == 0) || (vDST == 60*60) || (vDST == 30*60) || (vDST == 2*60*60))){
      return errorCode_t::bad_time;
    }
    index += sizeof(DST);

    const uint16_t vMaxTime = *reinterpret_cast<const uint16_t*>(&dataArray[index]);
    if(vMaxTime == 0){return errorCode_t::bad_time;}

    return errorCode_t::success;
  }
};

namespace ConfigStructFncs
{

  /**
   * @brief serialize a config into a byte array. first element is the config ID. does not perform a checksum.
   * 
   * @param dataArray pointer to write to
   */
  static void serialize(uint8_t outArray[TimeConfigsStruct::rawDataSize+1], const TimeConfigsStruct& configStruct) {
    outArray[0] = static_cast<uint8_t>(configStruct.ID);
    uint8_t index = 1;

    memcpy(&outArray[index], &configStruct.timezone, sizeof(configStruct.timezone));
    index += sizeof(configStruct.timezone);
    memcpy(&outArray[index], &configStruct.DST, sizeof(configStruct.DST));
    index += sizeof(configStruct.DST);
    memcpy(&outArray[index], &configStruct.maxSecondsBetweenSyncs, sizeof(configStruct.maxSecondsBetweenSyncs));
  };

  /**
   * @brief unpack a byte array into a config struct. the first element is the ID, then the data. does not compare the checksum
   * 
   * @param dataArray pointer to the start of the config data
   */
  static void deserialize(const byte *dataArray, TimeConfigsStruct& configStruct){
    if(static_cast<ModuleID>(dataArray[0]) != configStruct.ID){
      #ifdef native_env
        throw("data array has incorrect ID");
      #endif
      return;
    }
    uint8_t index = 1;
    memcpy(&configStruct.timezone, &dataArray[index], sizeof(configStruct.timezone));
    index += sizeof(configStruct.timezone);
    memcpy(&configStruct.DST, &dataArray[index], sizeof(configStruct.DST));
    index += sizeof(configStruct.DST);
    memcpy(&configStruct.maxSecondsBetweenSyncs, &dataArray[index], sizeof(configStruct.maxSecondsBetweenSyncs));
  };
} // namespace ConfigStructFncs



struct ModalConfigsStruct {
  public:
  duty_t minOnBrightness = 1;     // the absolute minimum brightness when state == on
  uint8_t softChangeWindow = 1;   // seconds; change window for sudden brightness changes
  duty_t defaultOnBrightness = 0; // for decorative lights, you might want them to always switch on to max. will get ignored by some modes


  // size of serialized data, excluding ID and checksum
  static const packetSize_t rawDataSize = sizeof(minOnBrightness)
                                 + sizeof(softChangeWindow)
                                 + sizeof(defaultOnBrightness);

  const ModuleID ID = ModuleID::modalLights;
  
  /**
   * @brief checks the validity of the serialized data
   * 
   * @param dataArray 
   * @return errorCode_t
   * @retval errorCode_t::success
   * @retval errorCode_t::badValue
   */
  static errorCode_t isDataValid(const byte dataArray[rawDataSize + 1]){
    const duty_t vMinB = *reinterpret_cast<const duty_t*>(&dataArray[1]);
    if(vMinB == 0){return errorCode_t::badValue;}

    const uint8_t vSoftWindow = *reinterpret_cast<const byte*>(&dataArray[2]);
    if(vSoftWindow >= (1 << 4)){return errorCode_t::badValue;}

    // const duty_t vDefB = *reinterpret_cast<const duty_t*>(&dataArray[sizeof(minOnBrightness) + sizeof(softChangeWindow)]);
    // no check for defaultOnBrightness

    return errorCode_t::success;
  }
};

namespace ConfigStructFncs
{
  static void serialize(byte *outArray, const ModalConfigsStruct& configStruct) {
    outArray[0] = static_cast<uint8_t>(configStruct.ID);
    uint8_t index = 1;

    memcpy(&outArray[index], &configStruct.minOnBrightness, sizeof(configStruct.minOnBrightness));
    index += sizeof(configStruct.minOnBrightness);
    memcpy(&outArray[index], &configStruct.softChangeWindow, sizeof(configStruct.softChangeWindow));
    index += sizeof(configStruct.softChangeWindow);
    memcpy(&outArray[index], &configStruct.defaultOnBrightness, sizeof(configStruct.defaultOnBrightness));
  }

  static void deserialize(const byte *dataArray, ModalConfigsStruct& configStruct){
    if(static_cast<ModuleID>(dataArray[0]) != configStruct.ID){
      #ifdef native_env
        throw("data array has incorrect ID");
      #endif
      return;
    }
    uint8_t index = 1;
    
    memcpy(&configStruct.minOnBrightness, &dataArray[index], sizeof(configStruct.minOnBrightness));
    index += sizeof(configStruct.minOnBrightness);
    memcpy(&configStruct.softChangeWindow, &dataArray[index], sizeof(configStruct.softChangeWindow));
    index += sizeof(configStruct.softChangeWindow);
    memcpy(&configStruct.defaultOnBrightness, &dataArray[index], sizeof(configStruct.defaultOnBrightness));
  }
} // namespace ConfigStructFncs

struct EventManagerConfigsStruct {
  public:
  uint32_t defaultEventWindow_S = 60*60;


  // size of serialized data, excluding ID and checksum
  static const packetSize_t rawDataSize = sizeof(defaultEventWindow_S);

  const ModuleID ID = ModuleID::eventManager;
  
  /**
   * @brief checks the validity of the serialized data
   * 
   * @param dataArray 
   * @return errorCode_t
   * @retval errorCode_t::success
   * @retval errorCode_t::bad_time
   */
  static errorCode_t isDataValid(const byte dataArray[rawDataSize + 1]){
    uint32_t vDefWindow = *reinterpret_cast<const uint32_t*>(&dataArray[1]);
    if(vDefWindow == 0){return errorCode_t::bad_time;}
    
    return errorCode_t::success;
  }
};

namespace ConfigStructFncs
{
  static void serialize(byte *outArray, const EventManagerConfigsStruct& configStruct) {
    outArray[0] = static_cast<uint8_t>(configStruct.ID);
    uint8_t index = 1;

    memcpy(&outArray[index], &configStruct.defaultEventWindow_S, sizeof(configStruct.defaultEventWindow_S));
  }

  static void deserialize(const byte *dataArray, EventManagerConfigsStruct& configStruct){
    if(static_cast<ModuleID>(dataArray[0]) != configStruct.ID){
      #ifdef native_env
        throw("data array has incorrect ID");
      #endif
      return;
    }
    uint8_t index = 1;
    memcpy(&configStruct.defaultEventWindow_S, &dataArray[index], sizeof(configStruct.defaultEventWindow_S));
  }
} // namespace ConfigStructFncs

/*################################################

    configs for optional modules

################################################*/

struct OneButtonConfigStruct {
  public:
  uint16_t timeUntilLongPress_mS = 500; // milliseconds before a short press turn into a long press

  uint16_t longPressWindow_mS = 2000; // milliseconds for a long press to dim the lights from 0 to 255 (or vice versa)


  // size of serialized data, excluding ID and checksum
  static const packetSize_t rawDataSize = sizeof(timeUntilLongPress_mS)
                                 + sizeof(longPressWindow_mS);

  const ModuleID ID = ModuleID::oneButtonInterface;
  
  /**
   * @brief checks the validity of the serialized data
   * 
   * @param dataArray 
   * @return true if valid
   * @return false if not valid
   * @retval errorCode_t::success
   * @retval errorCode_t::bad_time
   */
  static errorCode_t isDataValid(const byte dataArray[rawDataSize + 1]){
    uint8_t index = 1;
    const uint16_t vTimeLongPress = *reinterpret_cast<const uint16_t*>(&dataArray[index]);
    if(vTimeLongPress < 200){return errorCode_t::bad_time;}
    index += sizeof(timeUntilLongPress_mS);

    const uint16_t vLongPressWindow = *reinterpret_cast<const uint16_t*>(&dataArray[index]);
    if((vLongPressWindow < 1000) || (vLongPressWindow > 10000)){
      return errorCode_t::bad_time;
    }
    return errorCode_t::success;
  }
};

namespace ConfigStructFncs
{
  static void serialize(byte *outArray, const OneButtonConfigStruct& configStruct) {
    outArray[0] = static_cast<uint8_t>(configStruct.ID);
    uint8_t index = 1;

    memcpy(&outArray[index], &configStruct.timeUntilLongPress_mS, sizeof(configStruct.timeUntilLongPress_mS));
    index += sizeof(configStruct.timeUntilLongPress_mS);
    memcpy(&outArray[index], &configStruct.longPressWindow_mS, sizeof(configStruct.longPressWindow_mS));
  }

  static void deserialize(const byte *dataArray, OneButtonConfigStruct& configStruct){
    if(static_cast<ModuleID>(dataArray[0]) != configStruct.ID){
      #ifdef native_env
        throw("data array has incorrect ID");
      #endif
      return;
    }
    uint8_t index = 1;
    
    memcpy(&configStruct.timeUntilLongPress_mS, &dataArray[index], sizeof(configStruct.timeUntilLongPress_mS));
    index += sizeof(configStruct.timeUntilLongPress_mS);
    memcpy(&configStruct.longPressWindow_mS, &dataArray[index], sizeof(configStruct.longPressWindow_mS));
  }
} // namespace ConfigStructFncs

