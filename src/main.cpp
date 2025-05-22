// third-party libraries
#include <Arduino.h>
#include <driver/touch_pad.h>

#include "DeviceTime.h"
#include <touchSwitch.hpp>
#include <ModalLights.h>
#include <complimentaryPWM.hpp>

const uint8_t pollPin = D6;

class HardcodedConfigs : public ConfigAbstractHAL{
  private:
    ConfigsStruct _configs;
  
  public:
    ConfigsStruct getAllConfigs(){
      return _configs;
    }

    bool setConfigs(ConfigsStruct configs){
      return false;
    }

    bool reloadConfigs(){
      return true;
    }
};

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

void setup(){
  // Serial.begin(115200);
  // delay(1000);
  // Serial.println("Setting up...");

  pinMode(pollPin, OUTPUT);
  digitalWrite(pollPin, false);

  // Serial.println("constructing config manager");
  auto configHAL = makeConcreteConfigHal<HardcodedConfigs>();
  std::shared_ptr<ConfigManagerClass> configManager = std::make_shared<ConfigManagerClass>(std::move(configHAL));

  OnboardTimestamp onboardTime;
  RTCConfigsStruct configsStruct = {0, 0};
  configManager->setRTCConfigs(configsStruct);

  // Serial.println("constructing device time");
  auto deviceTime = std::make_shared<DeviceTimeClass>(configManager);

  // Serial.println("constructing data storage");
  auto storageHAL = std::make_shared<HardcodedStorage>();
  auto dataStorage = std::make_shared<DataStorageClass>(storageHAL);
  
  ModalConfigsStruct modalConfigs = {
    .defaultOnBrightness = 255
  };
  configManager->setModalConfigs(modalConfigs);
  
  // Serial.println("constructing modal lights");
  auto modalLights = std::make_shared<ModalLightsController>(
    concreteLightsClassFactory<ComplimentaryPWM>(),
    deviceTime,
    dataStorage, 
    configManager
  );

  modalLights->setBrightnessLevel(255);
  
  // Serial.println("constructing touch button");
  S3TouchButton touchSwitch(deviceTime, modalLights);
  
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
};

bool pinState = true;

void loop(){
  // TODO: reboot because this should be innaccessible
  // Serial.println("loop...");
  delay(200);
};

