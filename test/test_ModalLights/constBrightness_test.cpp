#include <ModalLights.h>
#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void initialisation(void){
  // i.e. what happens to the brightness when the mode changes
  const duty_t testBrightness = 200;
  const uint64_t endTime = 1000000000;
  
  { // should initialise with the previous brightness
    ModalLights::setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    ModalLights::setBrightnessLevel(testBrightness);
    ModalLights::setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    TEST_ASSERT_EQUAL(testBrightness, ModalLights::getBrightnessLevel());
  }

  { // brightness should decrease to the max when the initial brightness is too high

  }
  
  { // brightness shouldn't change when previous mode was changing

  }
}

void updateLights_suite(void){
  // the brightness should always be constant
  const duty_t testBrightness = 200;
  const uint64_t endTime = 1000000000;

  { // start of time
    ModalLights::setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    ModalLights::setBrightnessLevel(testBrightness);
    ModalLights::updateLights(0);
    TEST_ASSERT_EQUAL(testBrightness, ModalLights::getBrightnessLevel());
  }

  { // middle of time
    ModalLights::setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    ModalLights::setBrightnessLevel(testBrightness);
    ModalLights::updateLights(endTime/2);
    TEST_ASSERT_EQUAL(testBrightness, ModalLights::getBrightnessLevel());
  }

  { // end of time
    ModalLights::setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    ModalLights::setBrightnessLevel(testBrightness);
    ModalLights::updateLights(endTime);
    TEST_ASSERT_EQUAL(testBrightness, ModalLights::getBrightnessLevel());
  }

  { // after end of time
    ModalLights::setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    ModalLights::setBrightnessLevel(testBrightness);
    ModalLights::updateLights(endTime + 100);
    TEST_ASSERT_EQUAL(testBrightness, ModalLights::getBrightnessLevel());
  }
}

void setBrightness_suite(void){
  const duty_t testBrightness = 200;
  const uint64_t endTime = 1000000000;

  { // brightness can be set
    ModalLights::setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    ModalLights::setBrightnessLevel(5);
    ModalLights::setBrightnessLevel(testBrightness);
    TEST_ASSERT_EQUAL(testBrightness, ModalLights::getBrightnessLevel());
  }

  { // brightness can't exceed max

  }
}

void switchingState_suite(void){

  {
    // switching brightness off sets duty to 0
  }

  {
    // switching off then on returns brightness to previous value
  }

  {
    // setting brightness to 0 then switching on sets brightness to a default min
  }
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(initialisation);
  RUN_TEST(updateLights_suite);
  RUN_TEST(setBrightness_suite);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif