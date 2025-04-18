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

#define PrintDebug_message(message) _PrintDebug_whitespace; std::cout << "- " << message

// byte integers should be cast to larger ints to avoid conversion to char
#define PrintDebug_variable(name, value) _PrintDebug_whitespace; std::cout << "- " << name << " = " << (value) << std::endl;

#define PrintDebug_UINT8(name, value) _PrintDebug_whitespace; std::cout << "- " << name << " = " << static_cast<unsigned int>(value) << std::endl;

#define PrintDebug_array(name, array, size) _PrintDebug_whitespace; std::cout << "- " << name << ":" << std::endl; for(unsigned int i = 0; i < size; i++){_PrintDebug_whitespace; std::cout << "-- " << i << ":\t\t" << array[i] << std::endl;}

#define PrintDebug_UINT8_array(name, array, size) _PrintDebug_whitespace; std::cout << "- " << name << ":" << std::endl; for(unsigned int i = 0; i < size; i++){_PrintDebug_whitespace; std::cout << "-- " << i << ":\t\t" << static_cast<unsigned int>(array[i]) << std::endl;}

#define PrintDebug_INT8_array(name, array, size) _PrintDebug_whitespace; std::cout << "- " << name << ":" << std::endl; for(unsigned int i = 0; i < size; i++){_PrintDebug_whitespace; std::cout << "-- " << i << ":\t\t" << static_cast<int>(array[i]) << std::endl;}

#endif