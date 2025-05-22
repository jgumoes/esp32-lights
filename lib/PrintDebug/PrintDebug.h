/*
to use, open a pio terminal and paste:
pio test -e native -f "*{test_fileName}*" -v
(don't add the file type)

add the following test to the test files to make sure PrintDebugs are removed:
void noPrintDebug(){
  #ifdef __PRINT_DEBUG_H__
    TEST_ASSERT_MESSAGE(false, "did you forget to remove the print debugs?");
  #else
    TEST_ASSERT(true);
  #endif
}
*/
#ifndef __PRINT_DEBUG_H__
#define __PRINT_DEBUG_H__

#include <stdio.h>

static unsigned int _printDebugLevel = 0;
#define _PrintDebug_whitespace for(unsigned int n = 0; n<_printDebugLevel; n++){std::cout << "\t";}

#define PrintDebug_function(functionName) _PrintDebug_whitespace; std::cout << functionName << "{" << std::endl; _printDebugLevel++;

#define PrintDebug_endFunction _printDebugLevel = _printDebugLevel > 0 ? _printDebugLevel - 1 : 0; _PrintDebug_whitespace; std::cout << "}" << std::endl

#define PrintDebug_message(message) _PrintDebug_whitespace; std::cout << "- " << message << std::endl

// byte integers should be cast to larger ints to avoid conversion to char
#define PrintDebug_variable(name, value) {\
  if(sizeof(value) == 1){\
    PrintDebug_UINT8(name, value);\
  }\
  else{\
    _PrintDebug_whitespace; std::cout << "- " << name << " = " << (value) << std::endl;\
  }\
}

#define PrintDebug_UINT8(name, value) _PrintDebug_whitespace; std::cout << "- " << name << " = " << static_cast<unsigned int>(value) << std::endl;

#define PrintDebug_array(name, array, size) _PrintDebug_whitespace; std::cout << "- " << name << ":" << std::endl; for(unsigned int i = 0; i < size; i++){\
  _PrintDebug_whitespace; std::cout << "-- " << i << ":\t\t";\
  if(sizeof(array[i])){\
    std::cout << static_cast<int>(array[i]);\
  }\
  else{\
    std::cout << array[i];\
  }\
  std::cout << std::endl;\
}

#define PrintDebug_UINT8_array(name, array, size) _PrintDebug_whitespace; std::cout << "- " << name << ":" << std::endl; for(unsigned int i = 0; i < size; i++){_PrintDebug_whitespace; std::cout << "-- " << i << ":\t\t" << static_cast<unsigned int>(array[i]) << std::endl;}

#define PrintDebug_INT8_array(name, array, size) _PrintDebug_whitespace; std::cout << "- " << name << ":" << std::endl; for(unsigned int i = 0; i < size; i++){_PrintDebug_whitespace; std::cout << "-- " << i << ":\t\t" << static_cast<int>(array[i]) << std::endl;}

#define PrintDebug_reset _printDebugLevel = 0; std::cout<<"########################### print debug reset ###########################" << std::endl

// TODO: fix the macro
void static PrintDebug_timeInDay_from_UTS(UsefulTimeStruct UTS){
  uint16_t timeInDay = UTS.timeInDay;
  const uint16_t hours = UTS.timeInDay / uint16_t(60*60);
  timeInDay -= hours*60*60;
  const uint16_t minutes = timeInDay/60;
  timeInDay -= minutes*60;
  std::string mPad = minutes < 10 ? "0" : "";
  std::string sPad = timeInDay < 10 ? "0" : "";
  std::string message = "time: " + std::to_string(hours) + ":" + mPad + std::to_string(minutes) + ":" + sPad + std::to_string(timeInDay);
  PrintDebug_message(message);  // DO NOT DELETE!
}

// #define PrintDebug_timeInDay_from_UTS(usefulTimeStruct) {\
//   uint16_t timeInDay = usefulTimeStruct.timeInDay;\
//   const uint16_t hours = usefulTimeStruct.timeInDay / uint16_t(60*60);\
//   timeInDay -= hours*60*60;\
//   const uint16_t minutes = timeInDay/60;\
//   timeInDay -= minutes*60;\
//   std::string mPad = minutes < 10 ? "0" : "";\
//   std::string sPad = timeInDay < 10 ? "0" : "";\
//   std::string message = "time: " + std::to_string(hours) + ":" + mPad + std::to_string(minutes) + ":" + sPad + std::to_string(timeInDay);\
// }

#endif