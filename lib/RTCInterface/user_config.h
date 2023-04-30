/*
 * contains various defines used for configuring
 * various aspects of the source code.
 * All config defines should be listed here, and unused
 * ones should be commented out
 */

#ifndef __CONFIG_DEFINES
#define __CONFIG_DEFINES

#define TESTING_ENV // if enabled, allows a client a lot of control over the device. should be undefined for production (maybe a CI could catch this?)

/* XIAO_Seeeduino_BLE.ino */
#define DEBUG_PRINT_BLE // enables debugging for general BLE stuff

/* RTC_interface.cpp */
/* Select RTC chipset (uncomment 1 only) */
#define USE_DS3231

#define DEBUG_PRINT_ENABLE_SERIAL_DEBUGGING // use Serial.print for debugging

#ifdef DEBUG_PRINT_ENABLE_SERIAL_DEBUGGING

  /* RTC debugging */
  #define DEBUG_PRINT_RTC_DATETIME  // prints the entire datetime value
  #define DEBUG_PRINT_RTC_ENABLE

/* DeviceTimeService.cpp*/
  #define DEBUG_PRINT_DEVICE_TIME_ENABLE
  #define DEBUG_PRINT_DEVICE_TIME_CP_ENABLE
  #define DEBUG_PRINT_DEVICE_TIME_SERVICE_ENABLE
#endif

#endif