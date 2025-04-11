#include <unity.h>

#include "ConfigManager.h"
// #include "DeviceTime.h"

#include "../nativeMocksAndHelpers/mockConfig.h"

// ConfigManagerClass configs;
std::shared_ptr<ConfigManagerClass> configs;

void setUp(void) {
    // set stuff up here
  configs = std::make_shared<ConfigManagerClass>(makeConcreteConfigHal<MockConfigHal>());
}

void tearDown(void) {
    // clean stuff up here
    configs = NULL;
}

void RTCConfigs(void){
  // ConfigManagerClass configs(makeConcreteConfigHal<MockConfigHal>());

  RTCConfigsStruct rtcConfigs;
  rtcConfigs.DST = 18092;
  rtcConfigs.timezone = 92384;
  rtcConfigs.maxSecondsBetweenSyncs = 69*69*12;
  configs->setRTCConfigs(rtcConfigs);
  RTCConfigsStruct returnedConfigs = configs->getRTCConfigs();
  uint16_t retDST = returnedConfigs.DST;
  int32_t retTimezone = returnedConfigs.timezone;
  uint32_t retSecondsBetweenSyncs = returnedConfigs.maxSecondsBetweenSyncs;
  TEST_ASSERT_EQUAL(18092, retDST);
  TEST_ASSERT_EQUAL(92384, retTimezone);
  TEST_ASSERT_EQUAL(69*69*12, retSecondsBetweenSyncs);
}

void EventManagerConfigs(void){
  // ConfigManagerClass configs(makeConcreteConfigHal<MockConfigHal>());

  // should default to default
  {
    EventManagerConfigsStruct returnedConfigs = configs->getEventManagerConfigs();
    TEST_ASSERT_EQUAL(hardwareDefaultEventWindow, returnedConfigs.defaultEventWindow);
  }
  
  // change the values
  {
    uint32_t newEventWindow = 20*60;
    TEST_ASSERT_NOT_EQUAL(hardwareDefaultEventWindow, newEventWindow);  // just making sure

    EventManagerConfigsStruct eventConfigs;
    eventConfigs.defaultEventWindow = newEventWindow;
    TEST_ASSERT_TRUE(configs->setEventManagerConfigs(eventConfigs));
    EventManagerConfigsStruct returnedConfigs = configs->getEventManagerConfigs();
    TEST_ASSERT_EQUAL(newEventWindow, returnedConfigs.defaultEventWindow);
  }

  // rejects eventWindow = 0
  {
    EventManagerConfigsStruct returnedConfigs1 = configs->getEventManagerConfigs();

    EventManagerConfigsStruct eventConfigs;
    eventConfigs.defaultEventWindow = 0;
    TEST_ASSERT_FALSE(configs->setEventManagerConfigs(eventConfigs));

    EventManagerConfigsStruct returnedConfigs2 = configs->getEventManagerConfigs();
    TEST_ASSERT_EQUAL(returnedConfigs1.defaultEventWindow, returnedConfigs2.defaultEventWindow);
  }
}

void ModalConfigs(void){
  // ConfigManagerClass configs(makeConcreteConfigHal<MockConfigHal>());
  
  // default values
  {
    const ConfigsStruct defaultConfigs;
    
    ModalConfigsStruct returnedConfigs = configs->getModalConfigs();
    TEST_ASSERT_EQUAL(defaultConfigs.changeoverWindow, returnedConfigs.changeoverWindow);
    TEST_ASSERT_EQUAL(defaultConfigs.minOnBrightness, returnedConfigs.minOnBrightness);
    TEST_ASSERT_EQUAL(defaultConfigs.softChangeWindow, returnedConfigs.softChangeWindow);
  }

  // change values
  {
    const ModalConfigsStruct testConfigs1{
      .minOnBrightness = 10,
      .changeoverWindow = 5,
      .softChangeWindow = 3,
    };
    TEST_ASSERT_TRUE(configs->setModalConfigs(testConfigs1));
    ModalConfigsStruct returnedConfigs1 = configs->getModalConfigs();
    TEST_ASSERT_EQUAL(testConfigs1.changeoverWindow, returnedConfigs1.changeoverWindow);
    TEST_ASSERT_EQUAL(testConfigs1.minOnBrightness, returnedConfigs1.minOnBrightness);
    TEST_ASSERT_EQUAL(testConfigs1.softChangeWindow, returnedConfigs1.softChangeWindow);


    const ModalConfigsStruct testConfigs2{
      .minOnBrightness = 100,
      .changeoverWindow = 10,
      .softChangeWindow = 0,
    };
    TEST_ASSERT_TRUE(configs->setModalConfigs(testConfigs2));
    ModalConfigsStruct returnedConfigs2 = configs->getModalConfigs();
    TEST_ASSERT_EQUAL(testConfigs2.changeoverWindow, returnedConfigs2.changeoverWindow);
    TEST_ASSERT_EQUAL(testConfigs2.minOnBrightness, returnedConfigs2.minOnBrightness);
    TEST_ASSERT_EQUAL(testConfigs2.softChangeWindow, returnedConfigs2.softChangeWindow);
  }

  // should reject minOnBrightness = 0, and window > 15 seconds
  {
    const ModalConfigsStruct goodConfigs{
      .minOnBrightness = 1,
      .changeoverWindow = 15,
      .softChangeWindow = 15,
    };
    TEST_ASSERT_TRUE(configs->setModalConfigs(goodConfigs));

    {
      ModalConfigsStruct badBrightness = configs->getModalConfigs();
      badBrightness.minOnBrightness = 0;
      TEST_ASSERT_FALSE(configs->setModalConfigs(badBrightness));
      TEST_ASSERT_EQUAL(goodConfigs.minOnBrightness, configs->getModalConfigs().minOnBrightness);
    }
    {
      ModalConfigsStruct badChangeoverWindow = configs->getModalConfigs();
      badChangeoverWindow.changeoverWindow = 16;
      TEST_ASSERT_FALSE(configs->setModalConfigs(badChangeoverWindow));
      TEST_ASSERT_EQUAL(goodConfigs.changeoverWindow, configs->getModalConfigs().changeoverWindow);
    }
    {
      ModalConfigsStruct badSoftChangeWindow = configs->getModalConfigs();
      badSoftChangeWindow.softChangeWindow = 16;
      TEST_ASSERT_FALSE(configs->setModalConfigs(badSoftChangeWindow));
      TEST_ASSERT_EQUAL(goodConfigs.softChangeWindow, configs->getModalConfigs().softChangeWindow);
    }
  }
}

void RUN_UNITY_TESTS(){
  UNITY_BEGIN();
  RUN_TEST(RTCConfigs);
  RUN_TEST(EventManagerConfigs);
  RUN_TEST(ModalConfigs);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif