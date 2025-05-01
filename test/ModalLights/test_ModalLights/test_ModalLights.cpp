#include <unity.h>
#include <ModalLights.h>
#include "test_constBrightness.h"

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void testRoundingDivide(){
  for(uint16_t top = 0; top <= 255; top++){
    for(uint16_t bottom = 1; bottom <= 255; bottom++){
      uint8_t expected = round((float)(top)/ (float)(bottom));
      uint8_t actual = interpDivide(top, bottom);
      std::string message = "top = ";
      message += std::to_string(top);
      message += "; bottom = ";
      message += std::to_string(bottom);
      TEST_ASSERT_EQUAL_MESSAGE(expected, actual, message.c_str());
    }
  }
}
void test_convertToDataPackets_helper(){
  // make sure the test helper function actually works properly!
  TEST_IGNORE_MESSAGE("TODO");
}

void testInitialisation(){
  // test initialisation
  // TODO: construction without any modes should default to constant brightness on update
  // TODO: passing a mode after construction but before update should change that mode
  TEST_IGNORE_MESSAGE("not implemented");
}

void validateModePacketTest(){
  // test that the mode packet is valid

  // for max ratios, at least one channel should have a brightness of 255

  // for min ratios, at least one channel should have a brightness of 255 or all channels should be 0 to start with the previous mode's colours
  TEST_IGNORE_MESSAGE("not implemented");
}

void testModeSwitching(){
  // when an active and background mode are set at the same time, when update is called the active mode is loaded first
  // TODO: loading a background mode then cancelling the active without calling update should switch to the pending background mode
  TEST_IGNORE_MESSAGE("TODO");
}

void noPrintDebug(){
  #ifdef __PRINT_DEBUG_H__
    TEST_ASSERT_MESSAGE(false, "did you forget to remove the print debugs?");
  #else
    TEST_ASSERT(true);
  #endif
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(noPrintDebug);
  RUN_TEST(testRoundingDivide);
  RUN_TEST(testInitialisation);
  RUN_TEST(validateModePacketTest);
  RUN_TEST(testModeSwitching);
  
  ConstantBrightnessModeTests::constBrightness_tests();
  UNITY_END();
}


#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif