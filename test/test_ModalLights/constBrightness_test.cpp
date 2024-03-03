#include <ModalLights.h>
#include <unity.h>

duty_t mockHardwareDutyCycle = 0;

class TestLEDClass
{
public:

  TestLEDClass(){
    mockHardwareDutyCycle = 0;
  }

  void setDutyCycle(duty_t duty_value) {
    mockHardwareDutyCycle = duty_value;
  };
};


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

  TestLEDClass ledSpy;
  
  { // should initialise with the previous brightness
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    testController.setBrightnessLevel(testBrightness);
    testController.setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    TEST_ASSERT_EQUAL(testBrightness, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(testBrightness, mockHardwareDutyCycle);
  }

  { // brightness should decrease to the max when the initial brightness is too high
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(150);
    testController.setBrightnessLevel(testBrightness);
    TEST_ASSERT_EQUAL(150, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(150, mockHardwareDutyCycle);
  }
  
  { // brightness shouldn't change when previous mode was changing
    // TODO: need wavy modes
  }
}

void updateLights_suite(void){
  // the brightness should always be constant
  const duty_t testBrightness = 200;
  const uint64_t endTime = 1000000000;

  { // start of time
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    testController.setBrightnessLevel(testBrightness);
    testController.updateLights(0);
    TEST_ASSERT_EQUAL(testBrightness, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(testBrightness, mockHardwareDutyCycle);
  }

  { // middle of time
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    testController.setBrightnessLevel(testBrightness);
    testController.updateLights(endTime/2);
    TEST_ASSERT_EQUAL(testBrightness, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(testBrightness, mockHardwareDutyCycle);
  }

  { // end of time
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    testController.setBrightnessLevel(testBrightness);
    testController.updateLights(endTime);
    TEST_ASSERT_EQUAL(testBrightness, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(testBrightness, mockHardwareDutyCycle);
  }

  { // after end of time
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    testController.setBrightnessLevel(testBrightness);
    testController.updateLights(endTime + 100);
    TEST_ASSERT_EQUAL(testBrightness, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(testBrightness, mockHardwareDutyCycle);
  }
}

void setBrightness_suite(void){
  const duty_t testBrightness = 200;

  { // brightness can be set
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(LED_LIGHTS_MAX_DUTY);
    testController.setBrightnessLevel(5);
    testController.setBrightnessLevel(testBrightness);
    TEST_ASSERT_EQUAL(testBrightness, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(testBrightness, mockHardwareDutyCycle);
  }

  { // brightness can't exceed max
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(150);
    testController.setBrightnessLevel(200);
    TEST_ASSERT_EQUAL(150, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(150, mockHardwareDutyCycle);
  }

  { // brightness can be set below min
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(150);
    testController.setBrightnessLevel(1);
    TEST_ASSERT_EQUAL(1, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(1, mockHardwareDutyCycle);
  }

  { // setting brightness above 0 when lights are at 0, turns the lights on
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(150);
    testController.setBrightnessLevel(0);
    testController.setBrightnessLevel(200);
    TEST_ASSERT_EQUAL(150, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(150, mockHardwareDutyCycle);
  }

  { // setting brightness above 0 when lights are off, turns the lights on
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(255);
    testController.setState(0);
    testController.setBrightnessLevel(200);
    TEST_ASSERT_EQUAL(200, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(200, mockHardwareDutyCycle);
  }
}

void switchingState_suite(void){

  {
    // switching brightness off sets duty to 0
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(200);
    testController.setBrightnessLevel(150);
    testController.setState(0);
    TEST_ASSERT_EQUAL(0, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(0, mockHardwareDutyCycle);
  }

  {
    // switching off then on returns brightness to previous value
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(200);
    testController.setBrightnessLevel(150);
    testController.setState(0);
    testController.setState(1);
    TEST_ASSERT_EQUAL(150, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(150, mockHardwareDutyCycle);
  }

  {
    // setting brightness to 0 then switching on sets brightness to a default min
    ModalLightsController<TestLEDClass> testController;
    testController.setConstantBrightnessMode(200);
    testController.setBrightnessLevel(0);
    testController.setState(1);
    TEST_ASSERT_EQUAL(12, testController.getBrightnessLevel());
    TEST_ASSERT_EQUAL(12, mockHardwareDutyCycle);
  }
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(initialisation);
  RUN_TEST(updateLights_suite);
  RUN_TEST(setBrightness_suite);
  RUN_TEST(switchingState_suite);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif