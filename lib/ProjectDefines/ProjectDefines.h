/**
 * declare project defines here to avoid circular dependancies when including storage interfaces
 * 
 */
#ifndef __PROJECT_DEFINES_H__
#define __PROJECT_DEFINES_H__

#define maxNOfModes 50
#define maxNOfEvents 50

#include <Arduino.h>

/* ModalLights*/
#include "lightDefines.h"
#include "modalLightsDefines.hpp"

/* RTC_interface is an external module */
#include "RTC_interface.h"


/* EventManager */
typedef uint8_t eventUUID;
// hardware default event window = 1 hour
#define hardwareDefaultEventWindow 60*60
// minimum event window = 1 minute
#define hardwareMinimumEventWindow 60

struct EventManagerConfigsStruct {
  uint32_t defaultEventWindow = hardwareDefaultEventWindow;
};

// the data packet that gets recieved from the network and loaded from storage
struct EventDataPacket {
  eventUUID eventID = 0;
  modeUUID modeID = 0;
  uint32_t timeOfDay = 0;
  uint8_t daysOfWeek = 0; // lsb = Monday, msb-1 = Sunday, msb is reserved, i.e. 0b01100000 = saturday and sunday
  uint32_t eventWindow = 0; // TODO: delete me! how long after the time should the event trigger? should equal "timeout" in the Mode Data Struct
  bool isActive = 0;
};

/* Data Storage Class */
typedef modeUUID nModes_t;
typedef eventUUID nEvents_t;

// number of events/modes to preload from storage
#ifndef DataPreloadChunkSize
  // the unit tests expect this to be 5
  #define DataPreloadChunkSize 5
#endif

# endif