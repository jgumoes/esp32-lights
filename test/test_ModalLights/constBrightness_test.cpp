#include <ModalLights.h>
#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void updateLights_suite(void){
  // the brightness should always be constant
  const duty_t testBrightness = 200;
  const uint64_t endTime = 1000000000;
  { // start of time
    setLightsMode(constantBrightness, 0, 0, endTime, testBrightness, testBrightness);
    TEST_ASSERT_EQUAL(testBrightness, updateBrightness(0));
  }

  { // middle of time
    setLightsMode(constantBrightness, 0, endTime/2, endTime, testBrightness, testBrightness);
    TEST_ASSERT_EQUAL(testBrightness, updateBrightness(0));
  }

  { // end of time
    setLightsMode(constantBrightness, 0, endTime, endTime, testBrightness, testBrightness);
    TEST_ASSERT_EQUAL(testBrightness, updateBrightness(0));
  }

  { // after end of time
    setLightsMode(constantBrightness, 0, endTime + 100, endTime, testBrightness, testBrightness);
    TEST_ASSERT_EQUAL(testBrightness, updateBrightness(0));
  }
}


void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(updateLights_suite);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif