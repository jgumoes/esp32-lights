#ifndef __TEST_CONFIGS_HPP__
#define __TEST_CONFIGS_HPP__

#include <etl/map.h>

#include "moduleIDs.h"
#include "ConfigStorageClass.hpp"

namespace ConfigManagerTestObjects{

/**
 * @brief takes a config struct and transforms it into a GenericConfigStruct, extracting the isValid() function in the process
 * 
 */
struct GenericConfigStruct{
// TODO: reduce impulse control
  private:
  byte _data[maxConfigSize];
  public:
  const ModuleID ID = ModuleID::null;
  const uint8_t packetSize = 0; // size of ModuleID + raw data
  const ConfigStructFncs::validationFunc validationFunction = ConfigStructFncs::defaultInvalid;
  const byte* data() const {return _data;}

  errorCode_t isValid(const byte* packet) const {
    if(ID != static_cast<ModuleID>(packet[0])){
      return errorCode_t::badID;
    }
    return validationFunction(&packet[1]);
  }

  GenericConfigStruct(ModuleID id = ModuleID::null): ID(id), packetSize(0){
    _data[0] = static_cast<byte>(id);
    for(uint16_t i = 1; i < maxConfigSize; i++){
      _data[i] = 0;
    }
  }

  GenericConfigStruct(): ID(ModuleID::null), packetSize(0){
    for(uint16_t i = 0; i < maxConfigSize; i++){
      _data[i] = 0;
    }
  }

  GenericConfigStruct(ModuleID id, uint8_t packetSize, const byte serializedData[maxConfigSize], ConfigStructFncs::validationFunc validation = ConfigStructFncs::defaultInvalid) : ID(id), packetSize(packetSize), validationFunction(validation) {
    memcpy(_data, serializedData, packetSize);
    // zero the rest
    for(uint16_t i = packetSize; i < (maxConfigSize); i++){
      _data[i] = 0;
    }
  }

  GenericConfigStruct(const byte serializedPacket[maxConfigSize], ConfigStructFncs::validationFunc validation)
    : ID(convertToModuleID(serializedPacket[0])),
      packetSize(getConfigPacketSize(ID)),
      validationFunction(validation)
  {
    memcpy(_data, serializedPacket, packetSize);
    for(uint16_t i = packetSize; i < maxConfigSize; i++){
      _data[i] = 0;
    }
  }

#ifdef __PRINT_DEBUG_H__
  void print() const {
    PrintDebug_function("GenericConfigStruct::print()");
    PrintDebug_INT8_array("stored packet", _data, packetSize);
    PrintDebug_INT8_array("*packet", data(), packetSize);
    PrintDebug_endFunction;
  }
  #endif

  GenericConfigStruct copyGenericStruct(){
    return GenericConfigStruct(data(), validationFunction);
  }

  /**
   * @brief copies the ID and data to the out array
   * 
   * @param out 
   */
  void copy(byte *out) const {
    memcpy(out, _data, packetSize);
  }

  void newDataPacket(const byte newPacket[maxConfigSize]){
    if(newPacket[0] != _data[0]){
      throw("wrong packet for this config type");
    }
    memcpy(_data, newPacket, maxConfigSize);
  }
};

// typedef TimeConfigsStruct ConfigStructType;
template <class ConfigStructType>
GenericConfigStruct makeGenericConfigStruct(ConfigStructType configStruct){
  byte packet[maxConfigSize];
  ConfigStructFncs::serialize(packet, configStruct);
  return GenericConfigStruct(configStruct.ID, configStruct.rawDataSize + sizeof(ModuleID), packet, ConfigStructType::isDataValid);
};

const etl::map<ModuleID, GenericConfigStruct, 255> defaultConfigs = {
  {ModuleID::deviceTime, makeGenericConfigStruct(TimeConfigsStruct{})},
  {ModuleID::modalLights, makeGenericConfigStruct(ModalConfigsStruct{})},
  {ModuleID::eventManager, makeGenericConfigStruct(EventManagerConfigsStruct{})},
  {ModuleID::oneButtonInterface, makeGenericConfigStruct(OneButtonConfigStruct{})}
  /* add new configs here */
};

const etl::map<ModuleID, GenericConfigStruct, 255> goodConfigs = {
  {ModuleID::deviceTime, makeGenericConfigStruct(
    TimeConfigsStruct{
      .timezone = -(int32_t)10*60*60,
      .DST = 60*60,
      .maxSecondsBetweenSyncs = 12*60*60
    }
  )},
  {ModuleID::modalLights, makeGenericConfigStruct(
    ModalConfigsStruct{
      .minOnBrightness = 20,
      .softChangeWindow = 5,
      .defaultOnBrightness = 42,
    }
  )},
  {ModuleID::eventManager, makeGenericConfigStruct(
    EventManagerConfigsStruct{
      .defaultEventWindow_S = 58008,
    }
  )},
  {ModuleID::oneButtonInterface, makeGenericConfigStruct(
    OneButtonConfigStruct{
      .timeUntilLongPress_mS = 420,
      .longPressWindow_mS = 5000,
    }
  )}
  /* add new configs here */
};

const etl::map<ModuleID, GenericConfigStruct, 255> moreGoodConfigs = {
  {ModuleID::deviceTime, makeGenericConfigStruct(
    TimeConfigsStruct{
      .timezone = (int32_t)2*60*60,
      .DST = 0,
      .maxSecondsBetweenSyncs = 24*60*60
    }
  )},
  {ModuleID::modalLights, makeGenericConfigStruct(
    ModalConfigsStruct{
      .minOnBrightness = 10,
      .softChangeWindow = 3,
      .defaultOnBrightness = 100,
    }
  )},
  {ModuleID::eventManager, makeGenericConfigStruct(
    EventManagerConfigsStruct{
      .defaultEventWindow_S = 30*60,
    }
  )},
  {ModuleID::oneButtonInterface, makeGenericConfigStruct(
    OneButtonConfigStruct{
      .timeUntilLongPress_mS = 1000,
      .longPressWindow_mS = 10000,
    }
  )}
  /* add new configs here */
};

const etl::map<ModuleID, GenericConfigStruct, 255> badConfigs = {
  {ModuleID::deviceTime, makeGenericConfigStruct(
    TimeConfigsStruct{
      .timezone = (int32_t)2*60*60 - 1,
      .DST = 45*60,
      .maxSecondsBetweenSyncs = 0
    }
  )},
  {ModuleID::modalLights, makeGenericConfigStruct(
    ModalConfigsStruct{
      .minOnBrightness = 0,
      .softChangeWindow = 255,
      .defaultOnBrightness = 0,
    }
  )},
  {ModuleID::eventManager, makeGenericConfigStruct(
    EventManagerConfigsStruct{
      .defaultEventWindow_S = 0,
    }
  )},
  {ModuleID::oneButtonInterface, makeGenericConfigStruct(
    OneButtonConfigStruct{
      .timeUntilLongPress_mS = 100,
      .longPressWindow_mS = 200,
    }
  )}
  /* add new configs here */
};



} // ConfigManagerTestObjects
#endif