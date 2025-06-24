#include <unity.h>

#include <etl/list.h>
#include <etl/map.h>

#include "../../nativeMocksAndHelpers/GlobalTestHelpers.hpp"
#include "ConfigStorageClass.hpp"
#include "../testObjects.hpp"
#include "../MetadataFileReader.hpp"

#include "test_badConfigs.hpp"

#define ETL_CHECK_PUSH_POP

/*
When adding new configs or modules, ctrl+f the line below
"add new configs here"
*/




void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

namespace ConfigManagerTestObjects
{
  void TEST_EQUAL_TIME_CONFIGS(TimeConfigsStruct &expected, TimeConfigsStruct &actual, std::string message = ""){
    TEST_ASSERT_EQUAL_MESSAGE(expected.DST, actual.DST, message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(expected.timezone, actual.timezone, message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(expected.maxSecondsBetweenSyncs, actual.maxSecondsBetweenSyncs, message.c_str());

    TEST_ASSERT_EQUAL_MESSAGE(expected.ID, actual.ID, message.c_str());
    TEST_ASSERT_EQUAL_MESSAGE(expected.rawDataSize, actual.rawDataSize, message.c_str());
    {
      const uint8_t _bufferSize = expected.rawDataSize + 1;
      uint8_t _expectedBuffer[_bufferSize];
      ConfigStructFncs::serialize(_expectedBuffer, expected);
      uint8_t _actualBuffer[_bufferSize];
      ConfigStructFncs::serialize(_actualBuffer, actual);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(_expectedBuffer, _actualBuffer, _bufferSize, message.c_str());
    }
  };

  TestHelperStruct onlyUserWasNotified(
    const ModuleID notifiedUser,
    GenericUsersStruct &genericUsers,
    const std::string message
  ){
    for(
      auto userIT = genericUsers.userMap.begin();
      userIT != genericUsers.userMap.end();
      userIT++
    ){
      if(userIT->first == notifiedUser){
        if(userIT->second.getNewConfigsCount() == 0){
          return TestHelperStruct{
            .success = false,
            .message = message + "; user " + moduleIDToString(userIT->first) + "wasn't notified"
          };
        }
        if(userIT->second.getNewConfigsCount() > 1){
          return TestHelperStruct{
            .success = false,
            .message = message + "; user " + moduleIDToString(userIT->first) + "was notified too many times (" + std::to_string(userIT->second.getNewConfigsCount()) + ")"
          };
        }
      }
      else if(userIT->second.getNewConfigsCount() != 0){
        return TestHelperStruct{
          .success = false,
          .message = message + "; user " + moduleIDToString(userIT->first) + " was notified " + std::to_string(userIT->second.getNewConfigsCount()) + " times but shouldn't have been"
        };
      }
    }
    return TestHelperStruct{.success = true};
  }
} // namespace ConfigTestHelpers


void testTheTestHelpers(){
  // Who Tests the Testers?
  // me, I do
  using namespace ConfigManagerTestObjects;

  {
    TimeConfigsStruct defaultStruct;
    byte serialized[maxConfigSize];
    ConfigStructFncs::serialize(serialized, defaultStruct);
    TEST_ASSERT_EQUAL(defaultStruct.ID, static_cast<ModuleID>(serialized[0]));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(serialized, defaultConfigs.at(defaultStruct.ID).data(), getConfigPacketSize(defaultStruct.ID));
  }
  {  
    EventManagerConfigsStruct defaultStruct;
    byte serialized[maxConfigSize];
    ConfigStructFncs::serialize(serialized, defaultStruct);
    TEST_ASSERT_EQUAL(defaultStruct.ID, static_cast<ModuleID>(serialized[0]));

    byte copyBuffer[maxConfigSize];
    defaultConfigs.at(defaultStruct.ID).copy(copyBuffer);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(serialized, defaultConfigs.at(defaultStruct.ID).data(), getConfigPacketSize(defaultStruct.ID));
  }
  {  
    ModalConfigsStruct defaultStruct;
    byte serialized[maxConfigSize];
    ConfigStructFncs::serialize(serialized, defaultStruct);
    TEST_ASSERT_EQUAL(defaultStruct.ID, static_cast<ModuleID>(serialized[0]));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(serialized, defaultConfigs.at(defaultStruct.ID).data(), getConfigPacketSize(defaultStruct.ID));
  }
  {  
    OneButtonConfigStruct defaultStruct;
    byte serialized[maxConfigSize];
    ConfigStructFncs::serialize(serialized, defaultStruct);
    TEST_ASSERT_EQUAL(defaultStruct.ID, static_cast<ModuleID>(serialized[0]));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(serialized, defaultConfigs.at(defaultStruct.ID).data(), getConfigPacketSize(defaultStruct.ID));
  }
  
  // make sure the goodConfigs don't match the default
  {
    TEST_ASSERT_EQUAL(goodConfigs.size(), defaultConfigs.size());
    for(auto pair : goodConfigs){
      std::string message = moduleIDToString(pair.first);

      // test the config is valid
      errorCode_t error = pair.second.isValid(pair.second.data());
      TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, error, addErrorToMessage(error, message).c_str());
      
      // test the configs are more different than the same
      GenericConfigStruct def = defaultConfigs.at(pair.first);
      int nDifferences = 0;
      for(uint16_t i = 1; i < pair.second.packetSize; i++){
        nDifferences += (
          (def.data()[i] == pair.second.data()[i])
          && (def.data()[i] != 0)
        ) ? -1 : 1;
      }
      TEST_ASSERT_TRUE_MESSAGE(nDifferences > 0, message.c_str());
    }
  }

  // make sure the moreGoodConfigs don't match the default
  {
    TEST_ASSERT_EQUAL(moreGoodConfigs.size(), defaultConfigs.size());
    for(auto pair : moreGoodConfigs){
      std::string message = moduleIDToString(pair.first);

      // test the config is valid
      errorCode_t error = pair.second.isValid(pair.second.data());
      TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, error, addErrorToMessage(error, message).c_str());
      
      // test the configs are more different than the same
      GenericConfigStruct def = defaultConfigs.at(pair.first);
      int nDifferences = 0;
      for(uint16_t i = 1; i < pair.second.packetSize; i++){
        nDifferences += (
          (def.data()[i] == pair.second.data()[i])
          && (def.data()[i] != 0)
        ) ? -1 : 1;
      }
      TEST_ASSERT_TRUE_MESSAGE(nDifferences > 0, message.c_str());
    }
  }

  // make sure badConfigs are indeed bad
  {
    for(auto pair : badConfigs){
      std::string message = moduleIDToString(pair.first);

      errorCode_t error = pair.second.isValid(pair.second.data());
      TEST_ASSERT_NOT_EQUAL_MESSAGE(errorCode_t::success, error, message.c_str());
    }
  }

  // make sure test configs are filled
  {
    auto numberOfConfigsFunc = [](){
      uint8_t count = 0;
      for(uint8_t i = 1; i < static_cast<uint8_t>(ModuleID::max); i++){
        if(getConfigPacketSize(i) > 0){count++;}
      }
      return count;
    };
    const uint8_t expectedSize = numberOfConfigsFunc();
    
    TEST_ASSERT_EQUAL(expectedSize, defaultConfigs.size());
    TEST_ASSERT_EQUAL(expectedSize, goodConfigs.size());
    TEST_ASSERT_EQUAL(expectedSize, moreGoodConfigs.size());
    TEST_ASSERT_EQUAL(expectedSize, badConfigs.size());
  }
}

void testMetadata(){
  
  // test the reader
  {
    byte metaArray[16] = {
      1, 2, 3, 0,
      4, 2, 6, 1,
      9, 5, 2, 2,
      5, 4, 7, 3
    };
  
    const uint8_t packetSize = 4;
  
    auto configAddress = [](byte b1, byte b2){
      byte buf[2] = {b1, b2};
      return *reinterpret_cast<storageAddress_t*>(buf);
    };
    // MetadataPacketReader reader(metaArray);
    for(uint16_t index = 0; index < 4; index++){
      std::string message = "index = " + std::to_string(index);
      const uint8_t metaAddress = index * packetSize;
      byte *packetStart = &metaArray[metaAddress];
      MetadataPacketReader reader{packetStart};
      reader.setPacket(packetStart);
  
      // ID
      TEST_ASSERT_EQUAL_MESSAGE(static_cast<ModuleID>(packetStart[0]), reader.ID(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(packetStart[0], reader.intID(), message.c_str());
  
      // address
      const storageAddress_t expectedAddress = configAddress(packetStart[1], packetStart[2]);
      TEST_ASSERT_EQUAL_MESSAGE(expectedAddress, reader.address(), message.c_str());
  
      // meta index
      TEST_ASSERT_EQUAL_MESSAGE(index, reader.metaIndex(), message.c_str());
  
      // meta address
      TEST_ASSERT_EQUAL_MESSAGE(metaAddress, reader.metadataAddress(), message.c_str());
  
      // copy packet
      byte packetBuffer[packetSize];
      reader.copyPacket(packetBuffer);
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(packetStart, packetBuffer, packetSize, message.c_str());
  
      TEST_ASSERT_EQUAL_MESSAGE(packetSize, reader.size(), message.c_str());
    }
  }

  // reader and writer
  {
    struct MetadataStruct{
      ModuleID ID = ModuleID::null;
      uint8_t intID(){return static_cast<uint8_t>(ID);}
      storageAddress_t address = storageOutOfBounds;
      uint8_t metaIndex = 0;

      MetadataStruct(uint8_t id, storageAddress_t confAddress, uint8_t mIndex) :
        ID(static_cast<ModuleID>(id)),
        address(confAddress),
        metaIndex(mIndex){}
    };
    
    auto buildMetaList = [](){
      etl::list<MetadataStruct, 255> buildingList;
      storageAddress_t address = 0;
      for(uint8_t i = 0; i < maxNumberOfModules; i++){
        buildingList.push_back(MetadataStruct(i+1, address, i));
        address += 10;
      }
      return buildingList;
    };

    const etl::list<MetadataStruct, 255> metaList = buildMetaList();

    byte metaBuffer[metaList.size() * MetadataPacketWriter::size];
    MetadataPacketWriter writer(metaBuffer);
    MetadataPacketReader reader(metaBuffer);
    uint16_t index = 0;
    for(auto it = metaList.begin(); it != metaList.end(); it++){
      std::string message = "index = " + std::to_string(index);
      const storageAddress_t metaAddress = index * MetadataPacketWriter::size;
      writer.setPacket(&metaBuffer[metaAddress]);
      reader.setPacket(&metaBuffer[metaAddress]);
      // MetadataPacketReader reader(writer.packet);
      
      // ID
      writer.setID(it->ID);
      TEST_ASSERT_EQUAL_MESSAGE(it->ID, writer.ID(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(it->ID, reader.ID(), message.c_str());

      // address
      writer.setAddress(it->address);
      TEST_ASSERT_EQUAL_MESSAGE(it->address, writer.address(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(it->address, reader.address(), message.c_str());
  
      // meta index
      writer.setMetaIndex(it->metaIndex);
      TEST_ASSERT_EQUAL_MESSAGE(index, it->metaIndex, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(it->metaIndex, reader.metaIndex(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(it->metaIndex, reader.metaIndex(), message.c_str());
  
      // meta address
      TEST_ASSERT_EQUAL_MESSAGE(metaAddress, writer.metadataAddress(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(metaAddress, reader.metadataAddress(), message.c_str());

      index++;
    }
  }

  // test metadata file reader with random packets
  {
    using namespace ConfigManagerTestObjects;
    const uint8_t metadataSize = maxNumberOfModules * MetadataPacketWriter::size;
    byte metadataArray[metadataSize];
    MetadataFileReader metaFile(metadataSize, metadataArray);

    TEST_ASSERT_EQUAL(maxNumberOfModules, metaFile.getEndIndex());
    const byte expectedMetaArray[4][4] = {
      {1, 2, 3, 0},
      {4, 2, 6, 1},
      {2, 5, 2, 2},
      {5, 4, 7, 3}
    };
    for(uint16_t index = 0; index < 4; index++){
      std::string message = "index= " + std::to_string(index);
      const uint8_t metaAddress = index*MetadataPacketWriter::size;
      byte packet[4];
      memcpy(packet, expectedMetaArray[index], 4);
      errorCode_t error = metaFile.setMetadataPacket(packet);
      TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, error, addErrorToMessage(error, message).c_str());

      MetadataPacketReader expectedReader(packet);
      MetadataPacketReader actualReader(&metadataArray[metaAddress]);
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.ID(), actualReader.ID(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.address(), actualReader.address(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(*reinterpret_cast<storageAddress_t*>(&metadataArray[metaAddress+1]), actualReader.address(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.metaIndex(), actualReader.metaIndex(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(metaAddress, actualReader.metadataAddress(), message.c_str());
    }
  }

  // test metadata file with default configs
  {
    using namespace ConfigManagerTestObjects;
    const uint8_t metadataSize = maxNumberOfModules * MetadataPacketWriter::size;
    byte metadataArray[metadataSize];
    MetadataFileReader metaFile(metadataSize, metadataArray);

    struct PacketContainer{
      byte packet[4];
    };
    etl::list<PacketContainer, 255> expectedList;

    uint8_t index = 0;
    storageAddress_t configAddress = 0;
    for(auto it : defaultConfigs){
      const ModuleID type = it.first;
      GenericConfigStruct config = it.second;
      std::string message = "config = " + moduleIDToString(type) + "; index = " + std::to_string(index);
      const uint8_t metaAddress = index*MetadataPacketWriter::size;

      std::string debugMessage = "packet at index " + std::to_string(index);
      byte *packetStart = &metadataArray[metaAddress];
      
      errorCode_t error = metaFile.setMetadataPacket(config.ID, configAddress, index);
      TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, error, addErrorToMessage(error, message).c_str());
      
      PacketContainer packetContainer;
      MetadataPacketWriter expectedWriter(packetContainer.packet);
      {
        expectedWriter.setID(config.ID);
        expectedWriter.setAddress(configAddress);
        expectedWriter.setMetaIndex(index);
        expectedList.push_back(packetContainer);
      }

      MetadataPacketReader actualReader(&metadataArray[metaAddress]);
      TEST_ASSERT_EQUAL_MESSAGE(expectedWriter.ID(), actualReader.ID(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedWriter.address(), actualReader.address(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(*reinterpret_cast<storageAddress_t*>(&metadataArray[metaAddress+1]), actualReader.address(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedWriter.metaIndex(), actualReader.metaIndex(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(metaAddress, actualReader.metadataAddress(), message.c_str());

      index++;
      configAddress += config.packetSize;
    }

    for(auto it : expectedList){
      MetadataPacketReader expectedReader(it.packet);
      std::string message = "module = " + moduleIDToString(expectedReader.ID());

      MetadataPacketReader actualReader = metaFile.getPacket(expectedReader.ID());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.ID(), actualReader.ID(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.address(), actualReader.address(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.metaIndex(), actualReader.metaIndex(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.metadataAddress(), actualReader.metadataAddress(), message.c_str());
    }

    uint16_t expMetaIndex = 0;
    auto metaIterator = metaFile.getIterator();
    auto expectedListIterator = expectedList.begin();
    while(metaIterator.hasMore()){
      MetadataPacketReader actualReader = metaIterator.getNext();
      MetadataPacketReader expectedReader(expectedListIterator->packet);
      std::string message = "module = " + moduleIDToString(expectedReader.ID());

      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.ID(), actualReader.ID(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.address(), actualReader.address(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.metaIndex(), actualReader.metaIndex(), message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(expectedReader.metadataAddress(), actualReader.metadataAddress(), message.c_str());

      expectedListIterator++;
    }
  }

  // MetadataPacketContainer should construct in the null state
  {
    MetadataPacketContainer testpacket;
    TEST_ASSERT_EQUAL(ModuleID::null, testpacket.getReader().ID());
    TEST_ASSERT_EQUAL(storageOutOfBounds, testpacket.getReader().address());
    TEST_ASSERT_EQUAL(255, testpacket.getReader().metaIndex());
  }
}

/**
 * @brief every single set of configs will need to be serialized for storage and communication, and because serialized byte arrays is the only generic iterable approach that C++ is happy with
 * 
 */
void testConfigStructSerialization(){
  using namespace ConfigStructFncs;

  // for every config tested below, they should add their ID to the bitflag. the final test in this suite will be comparing this bitflag to the GenericUsersStruct bitflag.
  uint32_t configsTestedBitflag = 0;
  
  // maxConfigSize is correct
  {
    bool maxConfigFound = false;
    for(
      uint8_t m = 1;
      m < static_cast<uint8_t>(ModuleID::max);
      m++
    ){
      packetSize_t size = getConfigPacketSize(static_cast<ModuleID>(m));
      TEST_ASSERT_TRUE(size <= maxConfigSize);
      maxConfigFound |= (size == maxConfigSize);
    }
    TEST_ASSERT_TRUE(maxConfigFound);
  }
  
  // Device Time Configs
  {
    TimeConfigsStruct testConfig{
      .timezone = -69,
      .DST = 420,
      .maxSecondsBetweenSyncs = 80085
    };
    configsTestedBitflag |= moduleIDBit(testConfig.ID);
    
    // test ID matches
    TEST_ASSERT_EQUAL(ModuleID::deviceTime, testConfig.ID);

    // size
    const packetSize_t expectedRawSize = sizeof(testConfig.timezone) + sizeof(testConfig.DST) + sizeof(testConfig.maxSecondsBetweenSyncs);
    const packetSize_t expectedSize = sizeof(ModuleID) + expectedRawSize;
    TEST_ASSERT_EQUAL(expectedRawSize, testConfig.rawDataSize);
    TEST_ASSERT_EQUAL(expectedSize, getConfigPacketSize(testConfig.ID));

    // serialization
    uint8_t serialized[expectedSize + sizeof(ModuleID)];
    serialize(serialized, testConfig);
    {
      uint8_t expected[expectedSize + sizeof(ModuleID)];
      expected[0] = static_cast<uint8_t>(testConfig.ID);
      
      uint8_t i1 = sizeof(ModuleID);
      memcpy(&expected[i1], &testConfig.timezone, sizeof(testConfig.timezone));
      uint8_t i2 = i1 + sizeof(testConfig.timezone);
      memcpy(&expected[i2], &testConfig.DST, sizeof(testConfig.DST));
      uint8_t i3 = i2 + sizeof(testConfig.DST);
      memcpy(&expected[i3], &testConfig.maxSecondsBetweenSyncs, sizeof(testConfig.maxSecondsBetweenSyncs));
      
      uint8_t iEnd = i3 + sizeof(testConfig.maxSecondsBetweenSyncs);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, serialized, iEnd);
    }

    // deserialization
    {
      TimeConfigsStruct destConfig;
      deserialize(serialized, destConfig);

      TEST_ASSERT_EQUAL(testConfig.rawDataSize, destConfig.rawDataSize);
      TEST_ASSERT_EQUAL(testConfig.ID, destConfig.ID);
      TEST_ASSERT_EQUAL(testConfig.timezone, destConfig.timezone);
      TEST_ASSERT_EQUAL(testConfig.DST, destConfig.DST);
      TEST_ASSERT_EQUAL(testConfig.maxSecondsBetweenSyncs, destConfig.maxSecondsBetweenSyncs);
    }
  }

  // Event Manager configs
  {
    EventManagerConfigsStruct testConfig{
      .defaultEventWindow_S = 8008135
    };
    configsTestedBitflag |= moduleIDBit(testConfig.ID);

    // test ID matches
    TEST_ASSERT_EQUAL(ModuleID::eventManager, testConfig.ID);

    // size
    const packetSize_t expectedRawSize = sizeof(testConfig.defaultEventWindow_S);
    const packetSize_t expectedSize = sizeof(ModuleID) + expectedRawSize;
    TEST_ASSERT_EQUAL(expectedRawSize, testConfig.rawDataSize);
    TEST_ASSERT_EQUAL(expectedSize, getConfigPacketSize(testConfig.ID));

    // serialization
    uint8_t serialized[expectedSize + sizeof(ModuleID)];
    serialize(serialized, testConfig);
    {
      uint8_t expected[expectedSize + sizeof(ModuleID)];
      expected[0] = static_cast<uint8_t>(testConfig.ID);
      
      uint8_t i1 = sizeof(ModuleID);
      memcpy(&expected[i1], &testConfig.defaultEventWindow_S, sizeof(testConfig.defaultEventWindow_S));
      
      uint8_t iEnd = i1 + sizeof(testConfig.defaultEventWindow_S);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, serialized, iEnd);
    }

    // deserialization
    {
      EventManagerConfigsStruct destConfig;
      deserialize(serialized, destConfig);

      TEST_ASSERT_EQUAL(testConfig.rawDataSize, destConfig.rawDataSize);
      TEST_ASSERT_EQUAL(testConfig.ID, destConfig.ID);
      TEST_ASSERT_EQUAL(testConfig.defaultEventWindow_S, destConfig.defaultEventWindow_S);
    }
  }

  // Modal Lights Configs
  {
    ModalConfigsStruct testConfig{
      .minOnBrightness = 69,
      .softChangeWindow = 13,
      .defaultOnBrightness = 37
    };
    configsTestedBitflag |= moduleIDBit(testConfig.ID);

    // test ID matches
    TEST_ASSERT_EQUAL(ModuleID::modalLights, testConfig.ID);

    // size
    const packetSize_t expectedRawSize = sizeof(testConfig.minOnBrightness) + sizeof(testConfig.softChangeWindow) + sizeof(testConfig.defaultOnBrightness);
    const packetSize_t expectedSize = sizeof(ModuleID) + expectedRawSize;
    TEST_ASSERT_EQUAL(expectedRawSize, testConfig.rawDataSize);
    TEST_ASSERT_EQUAL(expectedSize, getConfigPacketSize(testConfig.ID));

    // serialization
    uint8_t serialized[expectedSize + sizeof(ModuleID)];
    serialize(serialized, testConfig);
    {
      uint8_t expected[expectedSize + sizeof(ModuleID)];
      expected[0] = static_cast<uint8_t>(testConfig.ID);
      
      uint8_t i1 = sizeof(ModuleID);
      memcpy(&expected[i1], &testConfig.minOnBrightness, sizeof(testConfig.minOnBrightness));
      uint8_t i2 = i1 + sizeof(testConfig.minOnBrightness);
      memcpy(&expected[i2], &testConfig.softChangeWindow, sizeof(testConfig.softChangeWindow));
      uint8_t i3 = i2 + sizeof(testConfig.softChangeWindow);
      memcpy(&expected[i3], &testConfig.defaultOnBrightness, sizeof(testConfig.defaultOnBrightness));
      
      uint8_t iEnd = i3 + sizeof(testConfig.defaultOnBrightness);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, serialized, iEnd);
    }

    // deserialization
    {
      ModalConfigsStruct destConfig;
      deserialize(serialized, destConfig);

      TEST_ASSERT_EQUAL(testConfig.rawDataSize, destConfig.rawDataSize);
      TEST_ASSERT_EQUAL(testConfig.ID, destConfig.ID);
      TEST_ASSERT_EQUAL(testConfig.minOnBrightness, destConfig.minOnBrightness);
      TEST_ASSERT_EQUAL(testConfig.softChangeWindow, destConfig.softChangeWindow);
      TEST_ASSERT_EQUAL(testConfig.defaultOnBrightness, destConfig.defaultOnBrightness);
    }
  }

  // OneButtonConfigs
  {
    OneButtonConfigStruct testConfig{
      .timeUntilLongPress_mS = 42069,
      .longPressWindow_mS = 58008
    };
    configsTestedBitflag |= moduleIDBit(testConfig.ID);

    // test ID matches
    TEST_ASSERT_EQUAL(ModuleID::oneButtonInterface, testConfig.ID);

    // size
    const packetSize_t expectedRawSize = sizeof(testConfig.timeUntilLongPress_mS) + sizeof(testConfig.longPressWindow_mS);
    const packetSize_t expectedSize = sizeof(ModuleID) + expectedRawSize;
    TEST_ASSERT_EQUAL(expectedRawSize, testConfig.rawDataSize);
    TEST_ASSERT_EQUAL(expectedSize, getConfigPacketSize(testConfig.ID));

    // serialization
    uint8_t serialized[expectedSize + sizeof(ModuleID)];
    serialize(serialized, testConfig);
    {
      uint8_t expected[expectedSize + sizeof(ModuleID)];
      expected[0] = static_cast<uint8_t>(testConfig.ID);
      
      uint8_t i1 = sizeof(ModuleID);
      memcpy(&expected[i1], &testConfig.timeUntilLongPress_mS, sizeof(testConfig.timeUntilLongPress_mS));
      uint8_t i2 = i1 + sizeof(testConfig.timeUntilLongPress_mS);
      memcpy(&expected[i2], &testConfig.longPressWindow_mS, sizeof(testConfig.longPressWindow_mS));
      
      uint8_t iEnd = i2 + sizeof(testConfig.longPressWindow_mS);
      TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, serialized, iEnd);
    }

    // deserialization
    {
      OneButtonConfigStruct destConfig;
      deserialize(serialized, destConfig);

      TEST_ASSERT_EQUAL(testConfig.rawDataSize, destConfig.rawDataSize);
      TEST_ASSERT_EQUAL(testConfig.ID, destConfig.ID);
      TEST_ASSERT_EQUAL(testConfig.timeUntilLongPress_mS, destConfig.timeUntilLongPress_mS);
      TEST_ASSERT_EQUAL(testConfig.longPressWindow_mS, destConfig.longPressWindow_mS);
    }
  }
  
  /* add new configs here */

  // TODO: add more configs as they're defined
  
  // test config bitflag
  {
    using namespace ConfigManagerTestObjects;
    for(uint8_t i = 1; i < maxNumberOfModules; i++){
      std::string message = "i = " + std::to_string(i);
      uint16_t one = 1;
      TEST_ASSERT_EQUAL_MESSAGE(one << (i-1), moduleIDBit(static_cast<ModuleID>(i)), message.c_str());
    }
    
    const uint32_t actualBitflag = configsTestedBitflag;
    uint32_t expectedBitflag1 = 0;

    const uint32_t one = 1;
    for(
      uint16_t i = 1;
      i < static_cast<uint8_t>(ModuleID::max);
      i++
    ){
      ModuleID configType = static_cast<ModuleID>(i);
      if(getConfigPacketSize(configType) != 0){
        std::string message = "configType = " + moduleIDToString(configType);
        TEST_ASSERT_EQUAL_MESSAGE(
          one << (i-1), moduleIDBit(configType), message.c_str()
        );

        expectedBitflag1 |= moduleIDBit(configType);
      }
    }

    // test against getConfigPacketSize()
    TEST_ASSERT_EQUAL(expectedBitflag1, actualBitflag);

    // test against registered users in ConfigStorageClass
    std::shared_ptr<IStorageAdapter> storageHal = std::make_shared<FakeStorageAdapter>();
    ConfigStorageClass configClass(storageHal);
    const GenericUsersStruct genericUsers(configClass, "testConfigStructSerialization");
    
    TEST_ASSERT_EQUAL(configClass.getRegisteredClasses(), actualBitflag);
  }
}

void testReadConfigs(){
  using namespace ConfigManagerTestObjects;
  // TODO: test with and without checksum
  
  // read configs from storage
  {
    const std::string testName = "read configs from storage";
    auto storageHal = std::make_shared<FakeStorageAdapter>(goodConfigs, true, "testReadConfigs");
    ConfigStorageClass configClass(storageHal);
    TEST_ASSERT_EQUAL(0, storageHal->closeCount);
    TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
    GenericUsersStruct genericUsers(configClass, testName);
    configClass.loadAllConfigs();

    // shouldn't be any writes
    TEST_ASSERT_EQUAL(0, storageHal->totalWrites());

    // the lock should have been released
    TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

    // check the metadata size bytes
    {
      const uint8_t metaSize = storageHal->getReservationSize(ModuleID::configStorage).metadataSize;
      TEST_ASSERT_EQUAL(maxNumberOfModules*MetadataPacketWriter::size, metaSize);
    }

    for(auto userIterator : genericUsers.userMap){
      const ModuleID configType = userIterator.second.type;
      const std::string message = testName + ": " + moduleIDToString(configType);
      
      // default config from ConfigStorageClass should match default from genericUsers
      {
        const GenericConfigStruct expected = defaultConfigs.at(configType);
        // expected config should be valid
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, expected.isValid(expected.data()), message.c_str());
  
        byte actualBuffer[maxConfigSize];
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, configClass.getDefaultConfig(configType, actualBuffer), message.c_str());
      }
  
      // shouldn't be any writes
      TEST_ASSERT_EQUAL(0, storageHal->totalWrites());
      
      // configClass should read expected config
      {
        const GenericConfigStruct expected = goodConfigs.at(configType);
        // expected config should be valid
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, expected.isValid(expected.data()), message.c_str());
        
        byte actualBuffer[maxConfigSize];
        TEST_ASSERT_SUCCESS(configClass.loadConfig(configType, actualBuffer), message);
  
        // config should be valid
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, expected.isValid(expected.data()), message.c_str());
  
        // configs should be equal
        TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected.data(), actualBuffer, expected.packetSize, message.c_str());
      }

      // there shouldn't be any writes
      TEST_ASSERT_EQUAL_MESSAGE(0, storageHal->writeCount, message.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(0, storageHal->metadataWriteCount, message.c_str());
    }
  }

  // if configs aren't in storage, return default (but don't write default to storage)
  {
    // note: this is extremely important, as it's a mechanism for only storing the configs that are needed by the program
    const std::string testName = "read configs from storage";
    auto storageHal = std::make_shared<FakeStorageAdapter>();
    ConfigStorageClass configClass(storageHal);
    TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());

    GenericUsersStruct genericUsers(configClass, testName);
    configClass.loadAllConfigs();

    // the lock should have been released
    TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

    // check the metadata size bytes
    {
      const uint8_t metaSize = storageHal->getReservationSize(ModuleID::configStorage).metadataSize;
      TEST_ASSERT_EQUAL(maxNumberOfModules*MetadataPacketWriter::size, metaSize);
    }

    storageHal->resetCounts();

    for(auto userIterator : genericUsers.userMap){
      const ModuleID configType = userIterator.second.type;
      const std::string message = testName + ": " + moduleIDToString(configType);
      
      // default config from ConfigStorageClass should match default from genericUsers
      {
        const GenericConfigStruct expected = defaultConfigs.at(configType);
        // expected config should be valid
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, expected.isValid(expected.data()), message.c_str());
  
        byte actualBuffer[maxConfigSize];
        genericUsers.userMap.at(configType).getConfigs(actualBuffer);
        TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected.data(), actualBuffer, expected.packetSize, message.c_str());
      }
  
      // configClass should read default config
      {
        const GenericConfigStruct expected = defaultConfigs.at(configType);
        
        byte actualBuffer[maxConfigSize];
        errorCode_t error = configClass.loadConfig(configType, actualBuffer);
        std::string errorMessage = message + "; error code = " + errorCodeToString(error);
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::notFound, error, errorMessage.c_str());
  
        // config should be valid
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::success, expected.isValid(expected.data()), message.c_str());
  
        // config isn't in metadata so no reads
        TEST_ASSERT_EQUAL_MESSAGE(0, storageHal->readCount, message.c_str());
        
        // configs should be equal
        TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected.data(), actualBuffer, expected.packetSize, message.c_str());
      }

      // there shouldn't be any writes
      TEST_ASSERT_EQUAL_MESSAGE(0, storageHal->totalWrites(), message.c_str());
    }
  }
  
  // test reading invalid configs
  {
    const std::string testName = "reading invalid configs";
    auto storageHal = std::make_shared<FakeStorageAdapter>(badConfigs, false, testName);
    ConfigStorageClass configClass(storageHal);
    TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());

    GenericUsersStruct genericUsers(configClass, testName);
    configClass.loadAllConfigs();

    // the lock should have been released
    TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

    // make sure the configs are in storage
    {
      for(auto confIT : badConfigs){
        std::string loopMessage = moduleIDToString(confIT.first);
        const byte* storedConfig = storageHal->getConfig(confIT.first);
        TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
          confIT.second.data(), storedConfig, confIT.second.packetSize, loopMessage.c_str()
        );
      }
    }

    // TODO: network was notified that stored configs are invalid
    
    
    for(auto userIterator : genericUsers.userMap){
      const ModuleID configType = userIterator.second.type;
      const std::string message = testName + ": " + moduleIDToString(configType);
      storageHal->resetCounts();
      
      // users should have default configs
      {
        const GenericConfigStruct expected = defaultConfigs.at(configType);
  
        byte actualBuffer[maxConfigSize];
        genericUsers.userMap.at(configType).getConfigs(actualBuffer);
        TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected.data(), actualBuffer, expected.packetSize, message.c_str());
      }
  
      // configClass should read default config
      {
        const GenericConfigStruct expected = defaultConfigs.at(configType);
        
        byte actualBuffer[maxConfigSize];
        errorCode_t error = configClass.loadConfig(configType, actualBuffer);
        std::string errorMessage = message + "; error code = " + errorCodeToString(error);
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::notFound, error, errorMessage.c_str());
  
        // invalid config was ignored during loading phase
        TEST_ASSERT_EQUAL_MESSAGE(0, storageHal->readCount, message.c_str());
        
        TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected.data(), actualBuffer, expected.packetSize, message.c_str());
      }

      // there shouldn't be any writes
      TEST_ASSERT_EQUAL_MESSAGE(0, storageHal->totalWrites(), message.c_str());
    }
  }

  // TODO: test reading corrupt configs (checksum fail)
  
  TEST_IGNORE_MESSAGE("outstanding TODOs");
}

void setConfigsTest(){
  using namespace ConfigManagerTestObjects;
  // for valid config changes, the user is notified and config is written to storage
  {
    const std::string testName = "setting good configs storage";
    auto storageHal = std::make_shared<FakeStorageAdapter>();
    ConfigStorageClass configClass(storageHal);
    TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
    GenericUsersStruct genericUsers(configClass, testName);
    configClass.loadAllConfigs();
    TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

    for(
      auto mapIT = genericUsers.userMap.begin();
      mapIT != genericUsers.userMap.end();
      mapIT++
    ){
      const ModuleID currentID = mapIT->first;
      GenericUser *user = &mapIT->second;
      std::string loopMessage = testName + "; " + moduleIDToString(currentID);

      {
        const GenericConfigStruct defaultConfig = defaultConfigs.at(currentID);
        TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(defaultConfig.data(), user->configStruct.data(), defaultConfig.packetSize, loopMessage.c_str());
      }

      // user->resetCounts();
      genericUsers.resetCounts();
      storageHal->metadataWriteCount = 0;
      storageHal->writeCount = 0;
      
      const GenericConfigStruct expected = goodConfigs.at(currentID);
      TEST_ASSERT_SUCCESS(configClass.setConfig(expected.data()), loopMessage);

      // user was notified and config updated
      TEST_WRAPPER(onlyUserWasNotified(user->type, genericUsers, loopMessage));
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected.data(), user->configStruct.data(), expected.packetSize, loopMessage.c_str());

      // config and metadata was written
      TEST_ASSERT_EQUAL_MESSAGE(1, storageHal->writeCount, loopMessage.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(1, storageHal->metadataWriteCount, loopMessage.c_str());

      // read config from storage
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
        expected.data(), storageHal->getConfig(currentID), expected.packetSize, loopMessage.c_str()
      );

      TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());
    }

    // read back the configs, just to make sure they're not overlapping
    for(auto confIT : goodConfigs){
      std::string loopMessage = testName + "; checking config " + moduleIDToString(confIT.first);
      GenericConfigStruct expected = confIT.second;
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
        expected.data(), storageHal->getConfig(confIT.first),
        expected.packetSize, loopMessage.c_str()
      );
    }
  }

  // test updating valid configs
  {
    const std::string testName = "test updating valid configs";
    auto storageHal = std::make_shared<FakeStorageAdapter>(goodConfigs, true, testName);
    ConfigStorageClass configClass(storageHal);
    TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
    GenericUsersStruct genericUsers(configClass, testName);
    configClass.loadAllConfigs();
    TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

    for(
      auto mapIT = genericUsers.userMap.begin();
      mapIT != genericUsers.userMap.end();
      mapIT++
    ){
      const ModuleID currentID = mapIT->first;
      GenericUser *user = &mapIT->second;
      std::string loopMessage = testName + "; " + moduleIDToString(currentID);

      {
        const GenericConfigStruct initialConfigs = goodConfigs.at(currentID);
        TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(initialConfigs.data(), user->configStruct.data(), initialConfigs.packetSize, loopMessage.c_str());
      }

      genericUsers.resetCounts();
      storageHal->metadataWriteCount = 0;
      storageHal->writeCount = 0;
      
      const GenericConfigStruct expected = moreGoodConfigs.at(currentID);
      TEST_ASSERT_SUCCESS(configClass.setConfig(expected.data()), loopMessage);

      // user was notified and config updated
      TEST_WRAPPER(onlyUserWasNotified(user->type, genericUsers, loopMessage));
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expected.data(), user->configStruct.data(), expected.packetSize, loopMessage.c_str());

      // read config from storage
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
        expected.data(), storageHal->getConfig(currentID), expected.packetSize, loopMessage.c_str()
      );

      // config was written but metadata should stay the same
      TEST_ASSERT_EQUAL_MESSAGE(1, storageHal->writeCount, loopMessage.c_str());
      TEST_ASSERT_EQUAL_MESSAGE(0, storageHal->metadataWriteCount, loopMessage.c_str());

      TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());
    }

    // read back the configs, just to make sure they're not overlapping
    for(auto confIT : moreGoodConfigs){
      std::string loopMessage = testName + "; checking config " + moduleIDToString(confIT.first);
      GenericConfigStruct expected = confIT.second;
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
        expected.data(), storageHal->getConfig(confIT.first),
        expected.packetSize, loopMessage.c_str()
      );
    }
  }

  // configs for modules that aren't used get rejected
  {
    const std::string testName = "configs for modules that aren't used get rejected";
    auto storageHal = std::make_shared<FakeStorageAdapter>(goodConfigs, true, "testReadConfigs");
    ConfigStorageClass configClass(storageHal);
    TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
    
    // only device time is registered
    GenericUser timeClass(configClass, makeGenericConfigStruct(TimeConfigsStruct{}), testName + "; timeClass");

    configClass.loadAllConfigs();
    // the lock should have been released
    TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

    storageHal->resetCounts();

    for(auto confPair : goodConfigs){
      const ModuleID type = confPair.first;
      const GenericConfigStruct config = confPair.second;
      std::string loopMessage = testName + "; config type = " + moduleIDToString(type);

      if(type != ModuleID::deviceTime){
        std::string loopMessage = testName + "; config type = " + moduleIDToString(type);
        errorCode_t error = configClass.setConfig(config.data());
        TEST_ASSERT_EQUAL_MESSAGE(errorCode_t::IDNotInUse, error, addErrorToMessage(error, loopMessage).c_str());

        TEST_ASSERT_EQUAL_MESSAGE(0, storageHal->totalCounts(), loopMessage.c_str());

        TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());
      }
    }
  }

  // TODO: for invalid config changes, the user isn't notified and the config isn't written to storage (maybe do this in a seperate file?)
  
  // TODO: if a new & accepted config fails to write, it's written to a different section of EEPROM and metadata is updated

  // TODO: if a stored config is invalid, it gets ignored. the metadata stays, and a valid config can be written to the same location

  // TODO: if a metadata write fails, it gets written to the next availlable location

  // TODO: if a "new" config is already in storage, don't write or alert users

  TEST_IGNORE_MESSAGE("outstanding TODOs");
}

void testSetTimeConfigs(){
  // DeviceTime is a special case that needs to set timezone and offset
  using namespace ConfigManagerTestObjects;

  const std::string testName = "setting time configs";
  auto storageHal = std::make_shared<FakeStorageAdapter>();
  ConfigStorageClass configClass(storageHal);
  TEST_ASSERT_EQUAL(ModuleID::configStorage, storageHal->getLock());
  GenericUsersStruct genericUsers(configClass, testName);
  configClass.loadAllConfigs();
  TEST_ASSERT_EQUAL(ModuleID::null, storageHal->getLock());

  // reset all notify counts
  for(auto userIT : genericUsers.userMap){
    userIT.second.resetCounts();
  }
  
  // Ladies and Gentlmen, our main event!
  GenericUser deviceTime = genericUsers.userMap.at(ModuleID::deviceTime);
  TimeConfigsStruct newTimeConfigs = {.timezone = -8*60*60, .DST=30*60};
  byte newConfigs[maxConfigSize];
  ConfigStructFncs::serialize(newConfigs, newTimeConfigs);

  // test new configs doesn't match old configs
  {
    byte oldConfigs[maxConfigSize];
    deviceTime.configStruct.copy(oldConfigs);
    bool isEqual = true;
    for(
      uint8_t i = 0;
      i < getConfigPacketSize(ModuleID::deviceTime);
      i++
    ){
      isEqual &= (newConfigs[i] == oldConfigs[i]);
    }
    TEST_ASSERT_FALSE(isEqual);
  }

  // set the new config
  deviceTime.configStruct.newDataPacket(newConfigs);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(newConfigs, deviceTime.configStruct.data(), getConfigPacketSize(ModuleID::deviceTime));
  
  configClass.setTimeConfigs(&deviceTime);

  // test config was written without notifying users
  const byte *storedConfig = storageHal->getConfig(ModuleID::deviceTime);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(newConfigs, storedConfig, getConfigPacketSize(ModuleID::deviceTime));
  
  for(auto userIT : genericUsers.userMap){
    std::string loopMessage = testName + "; user = " + moduleIDToString(userIT.second.type);
    TEST_ASSERT_EQUAL_MESSAGE(0, userIT.second.getNewConfigsCount(), loopMessage.c_str());
  }
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
  RUN_TEST(testTheTestHelpers);
  RUN_TEST(testMetadata);
  RUN_TEST(testConfigStructSerialization);
  RUN_TEST(testReadConfigs);
  RUN_TEST(setConfigsTest);
  RUN_TEST(testSetTimeConfigs);

  RUN_TEST(ConfigStorage_badConfigsTests::run_badConfigsTests);
  UNITY_END();
}

#ifdef native_env
void WinMain(){
  RUN_UNITY_TESTS();
}
#endif