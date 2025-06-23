#ifndef __MOCKWIRE_H__
#define __MOCKWIRE_H__

#include <stdint.h>

#include <etl/vector.h>

#include "./helpers.h"

class MockWire
{
public:
/*
 * instantiate the class with pre-set time values.
 * call convertToBcd() to convert them to the format
 * expected from DS3231
 */
  MockWire(uint8_t seconds, uint8_t minutes, uint8_t hours,
      uint8_t dayOfWeek, uint8_t date, uint8_t month, uint8_t years, bool convertToBCD = true){
        setBufferVals(seconds, minutes, hours, dayOfWeek, date, month, years, true);
      }

  /*
   * instantiate the class without any pre-set values
   */
  MockWire(){ MockWire(0, 0, 0, 0, 0, 0, 0); };

  /**
   * @brief Set the Buffer vals
   * 
   * @param seconds 
   * @param minutes 
   * @param hours 
   * @param dayOfWeek 
   * @param date 
   * @param month 
   * @param years 
   * @param convertToBCD 
   */
  void setBufferVals(uint8_t seconds, uint8_t minutes, uint8_t hours,
      uint8_t dayOfWeek, uint8_t date, uint8_t month, uint8_t years, bool convertToBCD = true){
    flush();
    mockBuffer[0] = seconds;
    mockBuffer[1] = minutes;
    mockBuffer[2] = hours;
    mockBuffer[3] = dayOfWeek;
    mockBuffer[4] = date;
    mockBuffer[5] = month;
    mockBuffer[6] = years;

    readSize = 0;
    readIndex = 0;

    if(convertToBCD){convertToBcd();}
  };
  
  uint8_t mockBuffer[13];
  uint8_t bytesToRead;
  uint8_t readSize;
  uint8_t readIndex;

  uint8_t writeHasBeenCalledTimes = 0;
  bool writeHasModifiedBuffer = false;
  uint8_t readHasBeenCalledTimes = 0;
  uint8_t changeBufferAfterRead = 0;  // change the buffer before the nth time read has been called. i.e. =0 won't change the buffer, =1 will change the buffer after the first time read() is called, 2 for the 2nd time, etc.
  etl::vector<uint8_t, 255> writeBuffer;

  uint8_t nextMockBuffer[13];
  
  bool wireAddressSet = false;
  bool registerAddressSet = false;

  void convertToBcd(){
    mockBuffer[0] = AS_BCD(mockBuffer[0]);
    mockBuffer[1] = AS_BCD(mockBuffer[1]);
    mockBuffer[2] = AS_BCD(mockBuffer[2]);
    mockBuffer[3] = AS_BCD(mockBuffer[3]);
    mockBuffer[4] = AS_BCD(mockBuffer[4]);
    mockBuffer[5] = AS_BCD(mockBuffer[5]);
    mockBuffer[6] = AS_BCD(mockBuffer[6]);
  };

  const uint8_t monthDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

  /*
   * increment a value in the buffer.
   * Example: for seconds, .incrementValue(0, 60);
   * @param index the index in the buffer to increment
   * @param maxValue buffer value ticks over if this value is reached
   * @return true if the value ticks over
   */
  bool incrementValue(uint8_t index, uint8_t maxValue){
    const uint8_t resetValues[7] = {0,0,0,0,1,1,0};
    uint8_t decValue = AS_DEC(mockBuffer[index]) + 1;
    if(decValue > maxValue){
      mockBuffer[index] = resetValues[index];
      return true;
    }
    mockBuffer[index] = AS_BCD(decValue);
    return false;
  };

  uint8_t datetimeBuffer[7];
  void makeDatetimeBuffer(){
    for(int i = 0; i < 7; i++){
      datetimeBuffer[i] = mockBuffer[i];
    }
  };

  /*
   * returns true if size is less than 13, otherwise false
   */
  uint8_t requestFrom(int address, int size){
    if(size > 13){ return false; }
    readSize = size + readIndex;
    readIndex = 0;
    return true;
  };
  int read(){
    if(readIndex >= readSize){
      if(changeBufferAfterRead == 0){
        readIndex = 0;
      }
      else{
        return 0;
      }
    }
    uint8_t readValue = mockBuffer[readIndex];
    readIndex++;
    readHasBeenCalledTimes++;
    if((readHasBeenCalledTimes == changeBufferAfterRead) && (changeBufferAfterRead > 0)){
      COPY_ARRAY(nextMockBuffer, mockBuffer);
    }
    return readValue;
  };
  size_t write(uint8_t value){
    if(!wireAddressSet){ throw "Wire address hasn't been set!"; }
    writeHasBeenCalledTimes += 1;
    if(!registerAddressSet){
      // first write() call sets the register address
      registerAddressSet = true;
      readIndex = value;
    }
    else{
      writeHasModifiedBuffer = true;
      mockBuffer[readIndex] = value;
      readIndex++;
    }
    return 0;
  };
  size_t write(const uint8_t* data, size_t quantity){
    for(size_t i = 0; i < quantity; i++){
      write(data[i]);
    }
    return quantity;
  };

  void beginTransmission(int address){
    wireAddressSet = true;
    // readIndex = 0;
  };
  uint8_t endTransmission(){
    wireAddressSet = false;
    registerAddressSet = false;
    // readIndex = 0;
    return 0;
  };
  
  bool isWireAvailable = true;
  int available(){return isWireAvailable * 13;}
  int peek(){return mockBuffer[0];}
  void flush(){
    for(int i = 0; i < 13; i++){mockBuffer[i] = 0;}
  }
};

#endif