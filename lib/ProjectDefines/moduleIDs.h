#ifndef __MODULE_IDS_H__
#define __MODULE_IDS_H__

#include <Arduino.h>

typedef uint8_t ModuleID_t;
/**
 * @brief this enum serves as a key/reference for config manager, the non-volatile storage access, the (currently hyperthetical) error handler, and transferring data over the network.
 * 
 */
enum class ModuleID : ModuleID_t{
  null = 0,

  /* mandatory modules: these will always be included */
  deviceTime = 1,
  modalLights = 2,
  eventManager = 3,
  dataStorageClass = 4,
  configManager,  // note: make this the last mandatory module

  /* optional modules: these may or may not be included, depending on physical implementation */
  oneButtonInterface,

  /* the upper limit + 1 for the number of stored configs */
  max
};

// aka ModuleID::max - 1
const uint8_t maxNumberOfModules = static_cast<uint8_t>(ModuleID::max)-1;

/**
 * @brief convert a uint8_t to ModuleID. if the id is out of bounds, returns ModuleID::null
 * 
 * @param id 
 * @return ModuleID 
 */
static ModuleID convertToModuleID(uint8_t id){
  if((id == 0) || id >= static_cast<uint8_t>(ModuleID::max)){
    return ModuleID::null;
  }
  return static_cast<ModuleID>(id);
};

typedef uint16_t ModuleIDBitflag; // TODO: test type isn't too small

static uint32_t moduleIDBit(ModuleID moduleID){
  const uint8_t intID = static_cast<uint8_t>(moduleID);
  if(intID == 0 || intID > maxNumberOfModules){
    return 0;
  }
  uint32_t one = 1;
  return one << (intID - 1);
}

static bool isValidModuleID(uint8_t intID){
  return (
    (static_cast<uint8_t>(ModuleID::max) > intID)
    && (static_cast<uint8_t>(ModuleID::null) < intID)
  );
}

#endif