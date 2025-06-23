#ifndef __GLOBAL_TEST_HELPERS_HPP__
#define __GLOBAL_TEST_HELPERS_HPP__

#include <unity.h>
#include <string>

#include "ProjectDefines.h"

static const std::string noMessage = "you didn't do the message now you don't know where the problem is u numpty";

struct TestHelperStruct{
  bool success = false;
  std::string message = noMessage;
};

/**
 * @brief wrap a helper function that returns a TestHelperStruct. you get all the benefits of a properly-defined function, but you know exactly where in the tests the failure occured
 * 
 */
#define TEST_WRAPPER(testHelperStruct) TEST_ASSERT_TRUE_MESSAGE(testHelperStruct.success, testHelperStruct.message.c_str())

std::string errorCodeToString(errorCode_t error){
  switch (error)
  {
  case errorCode_t::failed:
    return "failed";
  case errorCode_t::success:
    return "success";
  case errorCode_t::badValue:
    return "badValue";
  case errorCode_t::bad_time:
    return "badTime";
  case errorCode_t::badID:
    return "badID";
  case errorCode_t::bad_uuid:
    return "badUUID";

  case errorCode_t::event_not_found:
    return "eventNotFound";
  case errorCode_t::modeNotFound:
    return "modeNotFound";
  case errorCode_t::IDNotInUse:
    return "IDNotInUse";
  case errorCode_t::notFound:
    return "notFound";

  case errorCode_t::readFailed:
    return "readFailed";
  case errorCode_t::writeFailed:
    return "writeFailed";
  case errorCode_t::wearAttemptLimitReached:
    return "wearAttemptLimitReached";
  case errorCode_t::storage_full:
    return "storageFull";
  case errorCode_t::outOfBounds:
    return "outOfBounds";
  case errorCode_t::illegalAddress:
    return "illegalAddress";

  case errorCode_t::checksumFailed:
    return "checksumFailed";
  case errorCode_t::busy:
    return "busy";
  
  
  default:
    return "unknown error code: " + std::to_string(error);
  }
}

/**
 * @brief i.e. null -> "ModuleID::null"
 * 
 * @param moduleID 
 * @return std::string 
 */
std::string moduleIDToString(const ModuleID moduleID){
  switch (moduleID)
  {
  case ModuleID::null:
    return "ModuleID::null";

  case ModuleID::deviceTime:
    return "ModuleID::deviceTime";

  case ModuleID::modalLights:
    return "ModuleID::modalLights";

  case ModuleID::eventManager:
    return "ModuleID::eventManager";

  case ModuleID::dataStorageClass:
    return "ModuleID::dataStorageClass";

  case ModuleID::configStorage:
    return "ModuleID::configStorage";

  /* optional modules: these may or may not be included, depending on physical implementation */

  case ModuleID::oneButtonInterface:
    return "ModuleID::oneButtonInterface";

  case ModuleID::max:
    return "ModuleID::max";
  
  /* add new Module here */
  default:
    return "ModuleID doesn't exist:: " + std::to_string(static_cast<uint16_t>(moduleID));
  }
}

std::string addErrorToMessage(errorCode_t error, std::string message){
  return message + "; error = " + errorCodeToString(error);
}

TestHelperStruct testArraysAreNotEqual(const byte *expected, const byte *actual, const size_t size, const std::string message = noMessage){
  for(size_t i = 0; i < size; i++){
    if(expected[i] != actual[i]){
      return TestHelperStruct{.success = true, .message = message};
    }
  }
  return TestHelperStruct{.success = false, .message = message};
}

#define TEST_ASSERT_ERROR(expectedError, functionCall, message) (\
  {\
    errorCode_t _private_error_code_pls_dont_use_this_name_anywhere_else_ = functionCall;\
    TEST_ASSERT_EQUAL_MESSAGE(\
      expectedError,\
      _private_error_code_pls_dont_use_this_name_anywhere_else_,\
      addErrorToMessage(_private_error_code_pls_dont_use_this_name_anywhere_else_, message).c_str()\
    );\
  }\
)

#define TEST_ASSERT_SUCCESS(functionCall, message) (TEST_ASSERT_ERROR(errorCode_t::success, functionCall, message))

typedef etl::vector<EventDataPacket, 255> eventsVector_t;

typedef etl::vector<ModeDataStruct, 255> modesVector_t;

#endif