/**
 * declare project defines here to avoid circular dependancies when including config manager
 * 
 */
#ifndef __PROJECT_DEFINES_H__
#define __PROJECT_DEFINES_H__

#include <Arduino.h>

// RTC_interface is an external module
#include "RTC_interface.h"

// Event Manager
// hardware default event window = 1 hour
#define hardwareDefaultEventWindow 60*60
// minimum event window = 1 minute
#define hardwareMinimumEventWindow 60

struct EventManagerConfigsStruct {
  uint32_t defaultEventWindow = hardwareDefaultEventWindow;
};

# endif