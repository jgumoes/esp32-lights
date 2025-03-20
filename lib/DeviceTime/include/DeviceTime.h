/**
 * notes:
 *  - the local timestamp is stored locally and on the RTC chip, but the RTC_interface is built to accept UTC timestamps
 *  - the local timestamp is stored in microseconds because of timer peripheral,but it won't overflow until we're long dead and the world is engulfed in a malestrom of hurricanes and drought
 */

#ifndef __DEVICETIME_H__
#define __DEVICETIME_H__

#include "DeviceTimeWithRTC.hpp"
#include "DeviceTimeNoRTC.h"

// default DeviceTimeClass does not integrate an RTC
// typedef DeviceTimeNoRTCClass DeviceTimeClass;



#endif