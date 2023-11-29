#include <RTC_interface.h>
#include <SoftwareAlarm.h>
#include <unity.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

SoftwareAlarm sun_10am_alarm(DateTimeStruct testDatetime){
  uint32_t testTimestamp = convertToLocalTimestamp(&testDatetime);
  SoftwareAlarm sun_10am_alarm {0, 0, 10, 0b01000000, testDatetime, testTimestamp};
  return sun_10am_alarm;
}

SoftwareAlarm mon_7am_alarm(DateTimeStruct testDatetime){
  uint32_t testTimestamp = convertToLocalTimestamp(&testDatetime);
  SoftwareAlarm mon_7am_alarm {0, 0, 7, 0b00000001, testDatetime, testTimestamp};
  return mon_7am_alarm;
}

const DateTimeStruct fri_nov_3_23_afternoon{39, 4, 12, 5, 3, 11, 23};
const DateTimeStruct sun_nov_5_23_morning{39, 4, 9, 7, 5, 11, 23};
const DateTimeStruct epoch_2000 {0, 0, 0, 6, 1, 1, 0};

void generic_constructor_test(
  SoftwareAlarm (*alarmFactory)(DateTimeStruct),
  DateTimeStruct testDatetime,
  DateTimeStruct expectedDatetime
  ){
  SoftwareAlarm alarm = alarmFactory(testDatetime);
  TEST_ASSERT_EQUAL(
    convertToLocalTimestamp(&expectedDatetime),
    alarm.getNextTimestamp()
  );
}

void suite_findNextTimestampOnConstruction(void){
  {
    DateTimeStruct expectedDatetime;
    expectedDatetime.seconds = 0;
    expectedDatetime.minutes = 0;
    expectedDatetime.hours = 10;
    expectedDatetime.dayOfWeek = 7;
    expectedDatetime.date = 5;
    expectedDatetime.month = 11;
    expectedDatetime.years = 23;
    generic_constructor_test(&sun_10am_alarm, fri_nov_3_23_afternoon, expectedDatetime);
  }

  {
    DateTimeStruct expectedDatetime;
    expectedDatetime.seconds = 0;
    expectedDatetime.minutes = 0;
    expectedDatetime.hours = 10;
    expectedDatetime.dayOfWeek = 7;
    expectedDatetime.date = 5;
    expectedDatetime.month = 11;
    expectedDatetime.years = 23;
    generic_constructor_test(&sun_10am_alarm, sun_nov_5_23_morning, expectedDatetime);
  }

  {
    DateTimeStruct expectedDatetime;
    expectedDatetime.seconds = 0;
    expectedDatetime.minutes = 0;
    expectedDatetime.hours = 7;
    expectedDatetime.dayOfWeek = 1;
    expectedDatetime.date = 6;
    expectedDatetime.month = 11;
    expectedDatetime.years = 23;
    generic_constructor_test(&mon_7am_alarm, sun_nov_5_23_morning, expectedDatetime);
  }

  {
    // test from epoch
    DateTimeStruct expectedDatetime;
    expectedDatetime.seconds = 0;
    expectedDatetime.minutes = 0;
    expectedDatetime.hours = 7;
    expectedDatetime.dayOfWeek = 1;
    expectedDatetime.date = 3;
    expectedDatetime.month = 1;
    expectedDatetime.years = 0;
    generic_constructor_test(&mon_7am_alarm, epoch_2000, expectedDatetime);
  }

}

void RUN_UNITY_TESTS(){
  /*
    needed tests:
      - alarm in the next week
      - alarm in the next month
      - alarm in the next year
      - feb 29 alarm from feb 28
      - mar 1 alarm from feb 29
      - alarm initiated at trigger time
      - alarm initiated after time but before window close

      - weekday morning alarms
      - weekend morning alarms
      - school night bedtime alarms
      
      - instantiate multiple alarms with same datetime variables (test for mutation)
      - repeteadly check & trigger same alarm object

      - alarm across DST change
      - alarm during DST change
      - timezone change during alarm
  */
  UNITY_BEGIN();
  RUN_TEST(suite_findNextTimestampOnConstruction);
  UNITY_END();
}

void WinMain(){
  RUN_UNITY_TESTS();
}