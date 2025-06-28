// third-party libraries
#include <Arduino.h>
#include <driver/touch_pad.h>

// no modes or events yet, but etl doesn't like a max size of 0
#define MAX_NUMBER_OF_EVENTS 1
#define MAX_NUMBER_OF_MODES 1

#include "DeviceTime.h"
#include "touchSwitch.hpp"
#include "ModalLights.h"
#include "complimentaryPWM.hpp"
#include "NoStorageAdapter.hpp"

const uint8_t pollPin = D6;

class HardcodedStorage : public StorageHALInterface{
  public:
    // TODO: accept array of modes and events
    HardcodedStorage(){};

    void getModeIDs(storedModeIDsMap_t& storedIDs){
      storedIDs.clear();
    };

    void getEventIDs(storedEventIDsMap_t& storedIDs){
      storedIDs.clear();
    };

    bool getModeAt(nModes_t position, uint8_t buffer[modePacketSize]){
      return false;
    };

    nModes_t getNumberOfStoredModes(){
      return 0;
    };

    EventDataPacket getEventAt(nEvents_t position){
      EventDataPacket emptyEvent;
      return emptyEvent;
    };

    nEvents_t getNumberOfStoredEvents(){
      return 0;
    };

    nEvents_t fillChunk(EventDataPacket (&buffer)[DataPreloadChunkSize], nEvents_t eventNumber){
      for(uint8_t i = 0; i < DataPreloadChunkSize; i++){
        EventDataPacket emptyEvent;
        buffer[i] = emptyEvent;
      }
      return 0;
    };
};

void run_CurtainLights(){
  // Serial.begin(115200);
  // delay(1000);
  // Serial.println("Setting up...");

  pinMode(pollPin, OUTPUT);
  digitalWrite(pollPin, false);

  etl::vector<NoStorageConfigWrapper, maxNumberOfModules> configs = {
    makeNoStorageConfig(TimeConfigsStruct{
      .timezone = 0,
      .DST = 60*60
    }),
    makeNoStorageConfig(ModalConfigsStruct{
      .defaultOnBrightness = 255
    })
  };

  // Serial.println("constructing config manager");
  auto storageAdapter = std::make_shared<NoStorageAdapter>(
    configs,
    etl::vector<ModeDataStruct, MAX_NUMBER_OF_MODES>{},
    etl::vector<EventDataPacket, MAX_NUMBER_OF_EVENTS>{}
  );
  auto configStorage = std::make_shared<ConfigStorageClass>(storageAdapter);

  // Serial.println("constructing device time");
  auto deviceTime = std::make_shared<DeviceTimeClass>(configStorage);

  // Serial.println("constructing data storage");
  auto storageHAL = std::make_shared<HardcodedStorage>();
  auto dataStorage = std::make_shared<DataStorageClass>(storageHAL);
    
  // Serial.println("constructing modal lights");
  auto modalLights = std::make_shared<ModalLightsController>(
    concreteLightsClassFactory<ComplimentaryPWM>(),
    deviceTime,
    dataStorage, 
    configStorage
  );

  modalLights->setBrightnessLevel(255);
  
  // Serial.println("constructing touch button");
  S3TouchButton touchSwitch(deviceTime, modalLights, configStorage);
  
  // Serial.println("Setup complete");

  while(true){
    // digitalWrite(pollPin, HIGH);
    touchSwitch.update();
    modalLights->updateLights();
    // digitalWrite(pollPin, LOW);
    
    // touchSwitch.printValues();
    // // Serial.println();
    // // Serial.print("touch_pad_get_status(): "); // Serial.println(((touch_pad_get_status() & BIT(TOUCH_PIN)) !=0));
    delay(20);
  }
}