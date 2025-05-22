#include <unity.h>

#include "OneButtonInterface.hpp"

#include "..\EventManager\test_EventManager\mockModalLights.hpp"
#include "../nativeMocksAndHelpers/mockConfig.h"
#include "../ModalLights/test_ModalLights/testHelpers.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))

class TestButton : public OneButtonInterface{
  public:
    TestButton(
      std::shared_ptr<DeviceTimeClass> deviceTime,
      std::shared_ptr<ModalLightsInterface> modalLights
    ) : OneButtonInterface(deviceTime, modalLights){};

    bool isPressed;

    bool getCurrentStatus(){return isPressed;}

    /**
     * @brief test method to force class into theoretically innaccessible states
     * 
     * @param newStatus 
     */
    void setPreviousStatus(bool newStatus){_previousStatus = newStatus;}

    /**
     * @brief test method to force class into theoretically innaccessible states
     * 
     * @param state 
     */
    void setFSMState(PressStates state){_buttonState = state;}

    ShortPressParams getShortPressParams(){return _shortPress;}

    LongPressParams getLongPressParams(){return _longPress;}
};

void setUp(void){}
void tearDown(void){}

namespace OneButtonInterfaceTests{
  struct TestObjects{
    std::shared_ptr<OnboardTimestamp> timestamp;
    std::shared_ptr<MockModalLights> modalLights;
    std::shared_ptr<TestButton> button;

    /**
     * @brief increment the time by the given microsecond increment, then call update on the button class
     * 
     * @param increment_uS microseconds
     * @return uint64_t new UTC timestamp in microseconds
     */
    uint64_t incrementTimeAndUpdate_uS(uint64_t increment_uS){
      timestamp->setTimestamp_uS(timestamp->getTimestamp_uS() + increment_uS);
      button->update();
      return timestamp->getTimestamp_uS();
    }

    /**
     * @brief increment the time by the give millisecond increment, then call update on the button class
     * 
     * @param increment_mS increment in milliseconds
     * @return uint64_t new UTC timestamp in microseconds
     */
    uint64_t incrementTimeAndUpdate_mS(uint64_t increment_mS){
      return incrementTimeAndUpdate_uS(increment_mS*1000);
    }
  };

  TestObjects testButtonFactory(uint64_t initialUTCTime_S = 794275200){
    TestObjects testObjects;
    std::shared_ptr<ConfigManagerClass> configs = makeTestConfigManager();
    testObjects.timestamp = std::make_shared<OnboardTimestamp>();
    std::shared_ptr<DeviceTimeClass> deviceTime = std::make_shared<DeviceTimeClass>(configs);
    deviceTime->setUTCTimestamp2000(initialUTCTime_S, 0, 0);
    testObjects.modalLights = std::make_shared<MockModalLights>();
    testObjects.button = std::make_shared<TestButton>(deviceTime, testObjects.modalLights);

    return testObjects;
  }

  bool releasePress_helper(
    TestObjects &testObjects,
    const OneButtonConfigs &configs,
    const std::string message
  ){
    auto button = testObjects.button;
    button->isPressed = false;
    button->update();
    TEST_ASSERT_EQUAL_MESSAGE(PressStates::none, button->getFSMState(), message.c_str());
    return true;
  }

  /**
   * @brief enter short press mode from none state. runs tests along the way
   * 
   * @param testObjects 
   * @param configs 
   */
  bool enterShortPress_helper(
    TestObjects &testObjects,
    const OneButtonConfigs &configs,
    const std::string message
  ){
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    const uint64_t startTime = testObjects.timestamp->getTimestamp_uS();

    TEST_ASSERT_EQUAL_MESSAGE(PressStates::none, button->getFSMState(), message.c_str());

    button->isPressed = true;
    button->update();
    TEST_ASSERT_EQUAL_MESSAGE((1000*configs.timeUntilLongPress_mS) + startTime, button->getShortPressParams().endTimeUTC_uS, message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(PressStates::shortPress, button->getFSMState(), message.c_str());
    return true;
  }

  /**
   * @brief enter long press mode from none state. runs tests along the way
   * 
   * @param testObjects 
   * @param configs 
   */
  uint64_t enterLongPress_helper(
    TestObjects &testObjects,
    const OneButtonConfigs &configs,
    const std::string message
  ){
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    const uint16_t shortPressTime_mS = configs.timeUntilLongPress_mS;
    const bool initialLightsState = modalLights->getState();
    const duty_t initialBrightness = modalLights->getBrightnessLevel();

    modalLights->previousAdjustment = 500;
    
    enterShortPress_helper(testObjects, configs, message);
    const uint64_t startTime = testObjects.timestamp->getTimestamp_uS();

    uint64_t currentTime_uS;
    const uint32_t incr_mS = 20;
    for(uint32_t shortT = 0; shortT < shortPressTime_mS; shortT += incr_mS){
      TEST_ASSERT_EQUAL_MESSAGE(PressStates::shortPress, button->getFSMState(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(
        startTime + (shortPressTime_mS * 1000),
        button->getShortPressParams().endTimeUTC_uS,
        message.c_str()
      );
      TEST_ASSERT_EQUAL_MESSAGE(initialLightsState, modalLights->getState(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(initialBrightness, modalLights->getBrightnessLevel(), message.c_str());
      currentTime_uS = testObjects.incrementTimeAndUpdate_mS(incr_mS);
    }

    TEST_ASSERT_EQUAL_MESSAGE(startTime+(1000*shortPressTime_mS), currentTime_uS, message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(PressStates::longPress, button->getFSMState(), message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(
      1,
      abs(modalLights->previousAdjustment),
      message.c_str()
    );
    return currentTime_uS;
  }
  
  /**
   * @brief enters long press from none state, then holds for exactly holdWindow_mS
   * 
   * @param expectedDirection 
   * @param holdWindow_mS how long to hold the press for
   * @param testObjects 
   * @param configs 
   * @return int16_t returns the cumulative adjustments made during the hold
   */
  int16_t enterAndHoldLongPress_helper(
    const bool expectedDirection,
    const uint64_t holdWindow_uS,
    TestObjects &testObjects,
    const OneButtonConfigs &configs,
    const std::string message
  ){
    const uint64_t testStartTime_uS = testObjects.timestamp->getTimestamp_uS();

    const int dir = expectedDirection ? 1 : -1; // direction modifier. up == 1, down == -1
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    const uint64_t shortPressTime_uS = configs.timeUntilLongPress_mS * 1000;
    const uint64_t adjWindow_uS = configs.longPressWindow_mS * 1000;

    enterLongPress_helper(testObjects, configs, message);
    const uint64_t longPressStart = testObjects.timestamp->getTimestamp_uS();
    TEST_ASSERT_EQUAL_MESSAGE(testStartTime_uS + shortPressTime_uS, longPressStart, message.c_str());
    const uint64_t expectedTestEnd_uS = longPressStart + holdWindow_uS;

    // button should have adjusted by 1 when it enters longPress
    int16_t cumAdj_abs = 1; // as in cumulative. please do try to keep decorum
    TEST_ASSERT_EQUAL_MESSAGE(dir*cumAdj_abs, modalLights->previousAdjustment, message.c_str());
    
    const uint64_t firstIncr_uS = static_cast<uint64_t>(100) * static_cast<uint64_t>(1000);
    uint64_t incrL_uS = MIN(firstIncr_uS, holdWindow_uS);
    uint64_t currentTime_uS = longPressStart;
    modalLights->previousAdjustment = 500;

    while((currentTime_uS + incrL_uS) <= (longPressStart + holdWindow_uS)){
      modalLights->previousAdjustment = 500;
      
      // increment time
      currentTime_uS = testObjects.incrementTimeAndUpdate_uS(incrL_uS);
      const std::string timeMessage = message + "; long press length (uS) = " + std::to_string(currentTime_uS-longPressStart);

      const int16_t actualAdj = modalLights->previousAdjustment;
      
      // find expected absolute adjustment
      const int16_t expAdj_abs = interpolate(1, 255, float(currentTime_uS - longPressStart)/adjWindow_uS) - cumAdj_abs;
      cumAdj_abs += expAdj_abs;

      // test the adjustment
      const int16_t expectedAdj = expAdj_abs * dir;
      TEST_ASSERT_EQUAL_MESSAGE(expectedAdj, actualAdj, timeMessage.c_str());

      // break when test end has been reached
      if(currentTime_uS >= (longPressStart + holdWindow_uS)){
        break;
      }

      // increase the increment. this will make sure the button adjusts based on time
      incrL_uS += 1000;
      // if the next increment will exceed the test time, change it so that test will end at the correct time
      if((incrL_uS + currentTime_uS) > expectedTestEnd_uS){
        incrL_uS = expectedTestEnd_uS - currentTime_uS;
      }
    }

    // test the end time is as expected
    const uint64_t endTime_uS = testObjects.timestamp->getTimestamp_uS();
    TEST_ASSERT_EQUAL_MESSAGE(expectedTestEnd_uS, endTime_uS, message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(longPressStart+holdWindow_uS, endTime_uS, message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(
      testStartTime_uS + shortPressTime_uS + holdWindow_uS,
      endTime_uS, message.c_str()
    );

    // test the adjustments were made properly
    const int16_t totalAdjustments = dir * cumAdj_abs;
    const int16_t expectedAdjustments = dir * interpolate(1, 255, float(endTime_uS - longPressStart)/adjWindow_uS);
    TEST_ASSERT_EQUAL_MESSAGE(expectedAdjustments, totalAdjustments, message.c_str());
    
    TEST_ASSERT_EQUAL_MESSAGE(PressStates::longPress, button->getFSMState(), message.c_str());

    return totalAdjustments;
  }
  
  /**
   * @brief enters long press from none, and holds 
   * 
   * @param expectedDirection 
   * @param testObjects 
   * @param configs 
   */
  uint64_t pressAndHoldThroughLongPress_helper(
    const bool expectedDirection,
    TestObjects &testObjects,
    const OneButtonConfigs &configs,
    const std::string message
  ){
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    const uint16_t shortPressTime_mS = configs.timeUntilLongPress_mS;
    const uint64_t window_uS = configs.longPressWindow_mS * 1000;

    volatile const uint64_t testStart_uS = testObjects.timestamp->getTimestamp_uS();
    
    enterAndHoldLongPress_helper(expectedDirection, window_uS, testObjects, configs, message);
    
    // adjustment should be 0 after the window
    modalLights->previousAdjustment = 500;
    testObjects.incrementTimeAndUpdate_mS(500);
    TEST_ASSERT_EQUAL_MESSAGE(0, modalLights->previousAdjustment, message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(PressStates::longPress, button->getFSMState(), message.c_str());

    // and release
    button->isPressed = false;
    modalLights->previousAdjustment = 500;  // it can't reach 500, so this indicates adjustments of 0
    const uint64_t endTime_uS = testObjects.incrementTimeAndUpdate_mS(700);
    TEST_ASSERT_EQUAL_MESSAGE(PressStates::none, button->getFSMState(), message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(0, modalLights->previousAdjustment, message.c_str());
    return endTime_uS;
  }

}

void testNoneState(){
  using namespace OneButtonInterfaceTests;

  const OneButtonConfigs defaultConfigs;
  // inactive tests
  {
    // should initialise into none state
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = false;
    testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());

    // modalLights shouldn't be updated
    const uint8_t incr_mS = 20;
    for(uint32_t t = 0; t <= (defaultConfigs.longPressWindow_mS+defaultConfigs.timeUntilLongPress_mS+(incr_mS*10)); t += incr_mS){
      TEST_ASSERT_EQUAL(false, modalLights->getState());
      TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
      TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
      testObjects.incrementTimeAndUpdate_mS(incr_mS);
    }
  }

  // fallingEdge tests
  {
    // should initialise into none state
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = false;
    button->setPreviousStatus(true);
    testObjects.incrementTimeAndUpdate_mS(10);

    // shouldn't be accessible, so state should revert to none
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());

    // modalLights shouldn't be updated
    const uint8_t incr_mS = 20;
    for(uint32_t t = 0; t <= (defaultConfigs.longPressWindow_mS+defaultConfigs.timeUntilLongPress_mS+(incr_mS*10)); t += incr_mS){
      TEST_ASSERT_EQUAL(false, modalLights->getState());
      TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
      TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
      testObjects.incrementTimeAndUpdate_mS(incr_mS);
    }
  }

  // risingEdge tests
  {
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;
    
    // pressing the button should enter shortPress
    const uint64_t testTime = testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      testTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    // modal lights shouldn't be called yet
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
  }

  // active tests
  {
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;
    button->setPreviousStatus(true);
    
    // should be inaccessible, but enter shortPress
    const uint64_t testTime = testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      testTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );
    
    // modal lights shouldn't be called yet
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
  }
};

void testShortPressState(){
  using namespace OneButtonInterfaceTests;

  const OneButtonConfigs defaultConfigs;

  // inactive tests (entering shortPress)
  {
    // pressing button should enter shortPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;

    const uint64_t testTime = testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      testTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    // modal lights shouldn't be called yet
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());

    // create the inactive state conditions
    button->isPressed = false;
    button->setPreviousStatus(false);

    testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());

    // modal lights shouldn't be called
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
  }
  
  // inactive tests (forced into shortPress)
  {
    // force shortPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = false;
    button->setPreviousStatus(false);
    button->setFSMState(PressStates::shortPress);

    testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());

    // modal lights shouldn't be called
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
  }
  
  // fallingEdge (entering shortPress)
  {
    // pressing the button should enter shortPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;

    const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      startTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    // modal lights shouldn't be called yet
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());

    // release the button
    button->isPressed = false;
    testObjects.incrementTimeAndUpdate_mS(50);
    
    // modal lights should be toggled
    testObjects.incrementTimeAndUpdate_mS(100);
    TEST_ASSERT_EQUAL(true, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_NOT_EQUAL(0, modalLights->getBrightnessLevel());
  }

  // fallingEdge (forced into shortPress)
  {
    // pressing the button should enter shortPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = false;
    button->setPreviousStatus(true);
    button->setFSMState(PressStates::shortPress);

    testObjects.incrementTimeAndUpdate_mS(50);
    
    // modal lights should be toggled
    testObjects.incrementTimeAndUpdate_mS(100);
    TEST_ASSERT_EQUAL(true, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_NOT_EQUAL(0, modalLights->getBrightnessLevel());
  }
  
  // risingEdge (entered into shortPress)
  {
    // pressing the button should enter shortPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;

    const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      startTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    // modal lights shouldn't be called yet
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());

    // force a rising edge (this shouldn't happen)
    button->setPreviousStatus(false);
    button->isPressed = true;
    const uint64_t testTime = testObjects.incrementTimeAndUpdate_mS(50);
    
    // should have re-entered shortPress
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      testTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    // modal lights shouldn't be called
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
  }
  
  // risingEdge (forced into shortPress)
  {
    // pressing the button should enter shortPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->setPreviousStatus(false);
    button->isPressed = true;
    button->setFSMState(PressStates::shortPress);

    // modal lights shouldn't be called yet
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());

    // force a rising edge (this shouldn't happen)
    button->setPreviousStatus(false);
    button->isPressed = true;
    const uint64_t testTime = testObjects.incrementTimeAndUpdate_mS(50);
    
    // should have re-entered shortPress
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      testTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    // modal lights shouldn't be called
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
  }

  // active should eventually enter longPress (entered into shortPress)
  {
    // pressing the button should enter shortPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;

    const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(10);
    const uint64_t longPressStart_uS = startTime + (defaultConfigs.timeUntilLongPress_mS * 1000);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      longPressStart_uS,
      button->getShortPressParams().endTimeUTC_uS
    );

    uint64_t currentTime;
    const uint32_t incr_mS = 20;
    while(currentTime < longPressStart_uS){
      TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
      // modal lights shouldn't be called yet
      TEST_ASSERT_EQUAL(false, modalLights->getState());
      TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
      TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());

      currentTime = testObjects.incrementTimeAndUpdate_mS(incr_mS);
    }

    // should have entered long press
    TEST_ASSERT_EQUAL(PressStates::longPress, button->getFSMState());
  }

  // forced active should re-enter shortPress
  {
    // force shortPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;
    button->setPreviousStatus(true);
    button->setFSMState(PressStates::shortPress);

    const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(10);
    const uint64_t longPressStart_uS = startTime + (defaultConfigs.timeUntilLongPress_mS * 1000);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      longPressStart_uS,
      button->getShortPressParams().endTimeUTC_uS
    );

    uint64_t currentTime;
    const uint32_t incr_mS = 20;
    while(currentTime < longPressStart_uS){
      // modal lights shouldn't be called yet
      TEST_ASSERT_EQUAL(false, modalLights->getState());
      TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
      TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());

      currentTime = testObjects.incrementTimeAndUpdate_mS(incr_mS);
    }

    // should have entered long press
    TEST_ASSERT_EQUAL(PressStates::longPress, button->getFSMState());
  }
}

void testLongPressState(){
  using namespace OneButtonInterfaceTests;

  const OneButtonConfigs defaultConfigs;

  const uint16_t window_mS = defaultConfigs.longPressWindow_mS;

  // inactive tests (entering longPress)
  {
    // pressing and holding the button should enter longPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    modalLights->previousAdjustment = 500;
    button->isPressed = true;

    const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      startTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    const uint64_t longPressTime = testObjects.incrementTimeAndUpdate_mS(defaultConfigs.timeUntilLongPress_mS);
    TEST_ASSERT_EQUAL(PressStates::longPress, button->getFSMState());

    // create the inactive state conditions
    button->isPressed = false;
    button->setPreviousStatus(false);

    testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());

    // modal lights should be adjusted by +1
    TEST_ASSERT_EQUAL(true, modalLights->getState());
    TEST_ASSERT_EQUAL(1, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(1, modalLights->getBrightnessLevel());
    TEST_ASSERT_EQUAL(1, modalLights->previousAdjustment);
  }

  // inactive tests (forced into longPress)
  {
    // pressing and holding the button should enter longPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);

    // create the inactive state conditions
    button->setFSMState(PressStates::longPress);
    button->isPressed = false;
    button->setPreviousStatus(false);

    testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());

    // modal lights shouldn't be called
    TEST_ASSERT_EQUAL(false, modalLights->getState());
    TEST_ASSERT_EQUAL(100, modalLights->getSetBrightness());
    TEST_ASSERT_EQUAL(0, modalLights->getBrightnessLevel());
  }

  // fallingEdge tests (entered into longPress)
  {
    // pressing and holding the button should enter longPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;

    const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      startTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    const uint64_t longPressTime = testObjects.incrementTimeAndUpdate_mS(defaultConfigs.timeUntilLongPress_mS);
    TEST_ASSERT_EQUAL(PressStates::longPress, button->getFSMState());

    const uint64_t testTime1 = testObjects.incrementTimeAndUpdate_mS(500);
    const uint16_t expAdj1 = interpolate(0, 255, float(testTime1-longPressTime)/(window_mS*1000));
    TEST_ASSERT_EQUAL(expAdj1, modalLights->previousAdjustment);
    
    // release the button
    {
      button->isPressed = false;
      const uint64_t testTime2 = testObjects.incrementTimeAndUpdate_mS(600);
      TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());
      const uint16_t expAdj2 = interpolate(0, 255, float(testTime2-longPressTime)/(window_mS*1000)) - expAdj1;
      TEST_ASSERT_EQUAL(expAdj2, modalLights->previousAdjustment);
    }

    // increments stop after release
    modalLights->previousAdjustment = 500;  // it can't reach 500, so this indicates adjustments of 0
    testObjects.incrementTimeAndUpdate_mS(700);
    TEST_ASSERT_EQUAL(500, modalLights->previousAdjustment);
  }
  
  // fallingEdge tests (forced into longPress)
  {
    // pressing and holding the button should enter longPress
    TestObjects testObjects = testButtonFactory(794275200 + 69420);
    testObjects.incrementTimeAndUpdate_mS(8008135);
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = false;
    button->setPreviousStatus(true);
    button->setFSMState(PressStates::longPress);
    modalLights->previousAdjustment = 500;

    // adjustment should be 0, and exit into none state
    const uint64_t testTime1 = testObjects.incrementTimeAndUpdate_mS(500);
    TEST_ASSERT_EQUAL(0, modalLights->previousAdjustment);
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());
  }

  // active tests (entered into longPress)
  {
    // pressing and holding the button should enter longPress
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;

    const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(10);
    TEST_ASSERT_EQUAL(PressStates::shortPress, button->getFSMState());
    TEST_ASSERT_EQUAL(
      startTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
      button->getShortPressParams().endTimeUTC_uS
    );

    const uint64_t longPressTime = testObjects.incrementTimeAndUpdate_mS(defaultConfigs.timeUntilLongPress_mS);
    TEST_ASSERT_EQUAL(PressStates::longPress, button->getFSMState());

    const uint16_t expAdj0 = 1;
    TEST_ASSERT_EQUAL(expAdj0, modalLights->previousAdjustment);

    const uint64_t testTime1 = testObjects.incrementTimeAndUpdate_mS(500);
    const uint16_t expAdj1 = interpolate(1, 255, float(500)/window_mS) - expAdj0;
    TEST_ASSERT_EQUAL(expAdj1, modalLights->previousAdjustment);
    
    // keep pressing the button
    const uint64_t testTime2 = testObjects.incrementTimeAndUpdate_mS(600);
    TEST_ASSERT_EQUAL(PressStates::longPress, button->getFSMState());
    const uint16_t expAdj2 = interpolate(1, 255, float(testTime2-longPressTime)/(window_mS*1000))-expAdj1 - expAdj0;
    TEST_ASSERT_EQUAL(expAdj2, modalLights->previousAdjustment);

    // keep pressing for window duration
    const uint64_t testTime3 = testObjects.incrementTimeAndUpdate_mS(window_mS);
    TEST_ASSERT_EQUAL(PressStates::longPress, button->getFSMState());
    const uint16_t expAdj3 = 255 - expAdj1 - expAdj2 - expAdj0;
    TEST_ASSERT_EQUAL(expAdj3, modalLights->previousAdjustment);
    
    // keep pressing after window
    modalLights->previousAdjustment = 500;  // it can't reach 500, so this indicates adjustments of 0
    testObjects.incrementTimeAndUpdate_mS(700);
    TEST_ASSERT_EQUAL(PressStates::longPress, button->getFSMState());
    TEST_ASSERT_EQUAL(0, modalLights->previousAdjustment);

    // and release
    button->isPressed = false;
    modalLights->previousAdjustment = 500;  // it can't reach 500, so this indicates adjustments of 0
    testObjects.incrementTimeAndUpdate_mS(700);
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());
    TEST_ASSERT_EQUAL(0, modalLights->previousAdjustment);
  }

  // active tests (forced into longPress)
  {
    TestObjects testObjects = testButtonFactory();
    auto button = testObjects.button;
    auto modalLights = testObjects.modalLights;
    modalLights->setBrightnessLevel(100);
    modalLights->setState(false);
    button->isPressed = true;
    button->setPreviousStatus(true);
    button->setFSMState(PressStates::longPress);
    modalLights->previousAdjustment = 500;

    // adjustment should be 0
    const uint32_t incr_mS = 20;
    for(uint32_t t = 0; t < 2*window_mS; t += incr_mS){
      modalLights->previousAdjustment = 500;
      testObjects.incrementTimeAndUpdate_mS(incr_mS);
      TEST_ASSERT_EQUAL(0, modalLights->previousAdjustment);
    }

    // falling edge should enter none state
    button->isPressed = false;
    modalLights->previousAdjustment = 500;
    testObjects.incrementTimeAndUpdate_mS(incr_mS);
    TEST_ASSERT_EQUAL(0, modalLights->previousAdjustment);
    TEST_ASSERT_EQUAL(PressStates::none, button->getFSMState());
  }
}

void testButtonOperation(){
  using namespace OneButtonInterfaceTests;

  const OneButtonConfigs defaultConfigs;
  
  const uint16_t shortPressTime_mS = defaultConfigs.timeUntilLongPress_mS;
  const uint16_t window_mS = defaultConfigs.longPressWindow_mS;

  TestObjects testObjects = testButtonFactory();
  auto button = testObjects.button;
  auto modalLights = testObjects.modalLights;
  modalLights->setBrightnessLevel(100);
  modalLights->setState(true);
  modalLights->updateLights();
  button->isPressed = false;

  volatile const uint64_t settupTime_uS = testObjects.timestamp->getTimestamp_uS();
  volatile const uint64_t testStart_uS = testObjects.incrementTimeAndUpdate_mS(60000);
  modalLights->updateLights();

  // longPress should adjust in interpolated increments from 0 to a cummulative 255
  
  // adjustment direction should be positive initially
  {
    const std::string message = "0: adjustment direction should be positive initially";
    pressAndHoldThroughLongPress_helper(true, testObjects, defaultConfigs, message);
    TEST_ASSERT_EQUAL(255, modalLights->getBrightnessLevel());
    releasePress_helper(testObjects, defaultConfigs, message);
    testObjects.incrementTimeAndUpdate_mS(60000);
  }

  // adjustment direction should be negative now
  {
    const std::string message = "1: adjustment direction should be negative now";
    volatile const uint64_t holdWindow_uS = getTimeDiffToInterpValue(155, 0, 255, window_mS) * 1000;
    int16_t adjustment = enterAndHoldLongPress_helper(false, holdWindow_uS, testObjects, defaultConfigs, message);
    TEST_ASSERT_EQUAL_MESSAGE(-155, adjustment, message.c_str());
    TEST_ASSERT_EQUAL(100, modalLights->getBrightnessLevel());
    releasePress_helper(testObjects, defaultConfigs, message);
  }

  // adjustment direction should be flipped to be positive
  {
    const std::string message = "2: adjustment direction should be flipped to be positive";
    const uint64_t holdWindow_uS = getTimeDiffToInterpValue(50, 1, 255, window_mS) * 1000;
    int16_t adjustment = enterAndHoldLongPress_helper(true, holdWindow_uS, testObjects, defaultConfigs, message);
    TEST_ASSERT_EQUAL(50, adjustment);
    TEST_ASSERT_EQUAL(150, modalLights->getBrightnessLevel());
    releasePress_helper(testObjects, defaultConfigs, message);
  }

  // short press should turn lights off
  {
    const std::string message = "3: short press should turn lights off";
    enterShortPress_helper(testObjects, defaultConfigs, message);
    releasePress_helper(testObjects, defaultConfigs, message);
    TEST_ASSERT_EQUAL(false, modalLights->getState());
  }
  
  // adjustment should be positive because lights are off, even though last adjustment was positive
  {
    const std::string message = "4: adjustment should be positive because lights are off, even though last adjustment was positive";
    pressAndHoldThroughLongPress_helper(true, testObjects, defaultConfigs, message);
    releasePress_helper(testObjects, defaultConfigs, message);
    TEST_ASSERT_EQUAL(255, modalLights->getBrightnessLevel());
    testObjects.incrementTimeAndUpdate_mS(60000);
  }

  // adjust down a bit
  {
    const std::string message = "5: adjust down a bit";
    const uint64_t holdWindow_uS = getTimeDiffToInterpValue(69, 255, 0, window_mS) * 1000;
    int16_t adjustment = enterAndHoldLongPress_helper(false, holdWindow_uS, testObjects, defaultConfigs, message);
    releasePress_helper(testObjects, defaultConfigs, message);
    TEST_ASSERT_EQUAL(69, modalLights->getBrightnessLevel());
    TEST_ASSERT_EQUAL((69-255), adjustment);
    testObjects.incrementTimeAndUpdate_mS(60000);
  }

  // set lights to max. next adjustment should still be negative, as lights are at max
  {
    const std::string message = "6: set lights to max. next adjustment should still be negative, as lights are at max";
    modalLights->setBrightnessLevel(255);
    testObjects.incrementTimeAndUpdate_mS(60000);
    TEST_ASSERT_EQUAL(255, modalLights->getBrightnessLevel());
    TEST_ASSERT_EQUAL(true, modalLights->getState());

    const uint64_t holdWindow_uS = getTimeDiffToInterpValue(69, 255, 0, window_mS) * 1000;
    int16_t adjustment = enterAndHoldLongPress_helper(false, holdWindow_uS, testObjects, defaultConfigs, message);
    releasePress_helper(testObjects, defaultConfigs, message);
    TEST_ASSERT_EQUAL(69, modalLights->getBrightnessLevel());
    TEST_ASSERT_EQUAL((69-255), adjustment);
    testObjects.incrementTimeAndUpdate_mS(60000);
  }

  // adjustment always starts at one
  {
    const std::string message = "7: adjustment always starts at one";
    modalLights->setBrightnessLevel(100);
    {
      const std::string message_up = message + "; going up";
      const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(60000);
      const int16_t adjustment = enterAndHoldLongPress_helper(true, 1, testObjects, defaultConfigs, message_up);
      releasePress_helper(testObjects, defaultConfigs, message_up);
      const uint64_t endTime = testObjects.timestamp->getTimestamp_uS();
      TEST_ASSERT_EQUAL(
        startTime + (defaultConfigs.timeUntilLongPress_mS * 1000) + 1,
        endTime
      );
      TEST_ASSERT_EQUAL(1, adjustment);
      TEST_ASSERT_EQUAL(101, modalLights->getBrightnessLevel());
    }

    {
      const std::string message_down = message + "; going down";
      const uint64_t startTime = testObjects.incrementTimeAndUpdate_mS(60000);
      const int16_t adjustment = enterAndHoldLongPress_helper(false, 0, testObjects, defaultConfigs, message_down);
      releasePress_helper(testObjects, defaultConfigs, message_down);
      const uint64_t endTime = testObjects.timestamp->getTimestamp_uS();
      TEST_ASSERT_EQUAL(
        startTime + (defaultConfigs.timeUntilLongPress_mS * 1000),
        endTime
      );
      TEST_ASSERT_EQUAL(-1, adjustment);
      TEST_ASSERT_EQUAL(100, modalLights->getBrightnessLevel());
    }
  }
}

void testTimeUpdates(){
  // button interface needs to subscribe to DeviceTime, as state changes are based on time and longPress performs a timed interpolation
  TEST_IGNORE_MESSAGE("important TODO, but build a working model first");
}

void noEmbeddedUnfriendlyLibraries(){
  #ifdef __PRINT_DEBUG_H__
    TEST_ASSERT_MESSAGE(false, "did you forget to remove the print debugs?");
  #else
    TEST_ASSERT(true);
  #endif

  #ifdef _GLIBCXX_MAP
    TEST_ASSERT_MESSAGE(false, "std::map is included");
  #else
    TEST_ASSERT(true);
  #endif
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(noEmbeddedUnfriendlyLibraries);
  RUN_TEST(testNoneState);
  RUN_TEST(testShortPressState);
  RUN_TEST(testLongPressState);
  RUN_TEST(testButtonOperation);
  RUN_TEST(testTimeUpdates);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif