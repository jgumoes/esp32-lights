#ifndef __METADATA_FILE_READER_HPP__
#define __METADATA_FILE_READER_HPP__

#include <Arduino.h>
#include "moduleIDs.h"
#include "errorCodes.h"
#include "StorageAdapter.hpp"
#include "MetadataPacket.hpp"

class MetadataFileReader; // forward declaration

/**
 * @brief iterates over the metadata by index
 * 
 */
class MetadataFileIterator{
  private:
  uint8_t currentIndex = 0;
  MetadataFileReader* meta;
  bool _hasMore = false;
  public:
  MetadataFileIterator(MetadataFileReader* fileReader) : meta(fileReader){}

  MetadataPacketReader getNext();

  bool hasMore();
};

class MetadataFileReader{
private:
  byte *_sizeArray;
  byte *_metadata;  // matches the metadata array in storage

  const uint8_t _packetSize = MetadataPacketWriter::size;
  byte emptyPacket[MetadataPacketWriter::size] = {0, 0, 0, 0};

public:
  MetadataFileReader(byte *sizeArray, byte *metadata) : _sizeArray(sizeArray), _metadata(metadata) {}

  MetadataFileIterator getIterator(){
    return MetadataFileIterator(this);
  }
  
  /**
   * @brief Get the size set in the file. if the sizes don't match, returns 0
   * 
   * @return uint8_t max number of metadata packets that can be stored
   */
  uint8_t getSize(){
    const uint8_t size = _sizeArray[0];
    const uint8_t compSize = _sizeArray[1];
    if(
      ((size & compSize) != 0)
      || ((size ^ compSize) != UINT8_MAX)
    ){
      return 0;
    }
    return size;
  }

  /**
   * @brief Get the size set in the file. if the sizes don't match, returns 0
   * 
   * @return uint16_t size of the stored metadata array in bytes
   */
  uint16_t getSizeInBytes(){return getSize()*_packetSize;}

  /**
   * @brief Set the size array with no checks
   * 
   * @param newSize 
   */
  void setSize(const uint8_t newSize){
    _sizeArray[0] = newSize;
    _sizeArray[1] = ~newSize;
  }

  /**
   * @brief searches metadata array for a packet location. if there are duplicate packets with the same ID, return the last packet address. if none are found, returns UINT8_MAX
   * 
   * @param ID 
   * @return uint8_t 
   */
  uint8_t getMetaPacketIndex(ModuleID ID){
    const uint8_t metaSize = getSize();
    if(metaSize == 0){return UINT8_MAX;}

    uint8_t foundIndex = UINT8_MAX;
    for(uint16_t index = 0; index < metaSize; index++){
      if(
        static_cast<ModuleID>(_metadata[index*_packetSize]) == ID
        && (isPacketValid(index) == errorCode_t::success)
      ){
        foundIndex = index;
      }
    }
    return foundIndex;
  }
  
  errorCode_t isPacketValid(MetadataPacketReader& packet, uint8_t intendedIndex){
    if(!isValidModuleID(packet.intID())){
      return errorCode_t::badID;
    }
    if(intendedIndex != packet.metaIndex()){
      return errorCode_t::badValue;
    }
    if(storageOutOfBounds <= packet.address()){
      return errorCode_t::outOfBounds;
    }
    if(getSize() <= packet.metaIndex()){
      return errorCode_t::outOfBounds;
    }
    return errorCode_t::success;
  }

  /**
   * @brief checks that a packet is valid. checks ID, index, and address isn't out of bounds
   * 
   * @param packet 
   * @param intendedIndex 
   * @return true 
   * @return false 
   */
  errorCode_t isPacketValid(byte* packet, uint8_t intendedIndex){
    MetadataPacketReader mpStruct(packet);
    return isPacketValid(mpStruct, intendedIndex);
  }
  
  /**
   * @brief checks that a packet in the metadata array is valid. checks ID, index, and address isn't out of bounds
   * 
   * @param index 
   * @return true 
   * @return false 
   */
  errorCode_t isPacketValid(uint8_t index){
    return isPacketValid(&_metadata[index*_packetSize], index);
  }
  
  /**
   * @brief searches metadata for the last packet with a valid ID, and returns the index after it. return UINT8_MAX if a location couldn't be found
   * 
   * @return uint8_t 
   */
  uint8_t findNextMetaIndex(){
    const uint8_t size = getSize();
    // if(size == 0){return UINT8_MAX;}

    uint8_t nextIndex = 0;

    for(uint8_t index = 0; index < size; index++){
      errorCode_t error = isPacketValid(index);
      if(
        (errorCode_t::success == error)
        && index >= nextIndex
      ){
        nextIndex = index + 1;
      }
    }
    return nextIndex;
  }

  /**
   * @brief Get packet by index. returns an empty packet if index is out of bounds or the packet is invalid
   * 
   * @param index 
   * @return MetadataPacketReader 
   */
  MetadataPacketReader getPacket(uint8_t index){
    MetadataPacketReader mpStruct(emptyPacket);
    if(
      (index < getSize())
      && isPacketValid(index)
    ){
      mpStruct.setPacket(&_metadata[index*_packetSize]);
    }
    return mpStruct;
  }
  
  /**
   * @brief searches metadata for a given packet and returns a reference struct. if none is found, returns an empty packet
   * 
   * @param ID 
   * @return const MetadataPacket 
   */
  MetadataPacketReader getPacket(ModuleID ID){
    const uint8_t index = getMetaPacketIndex(ID);
    return getPacket(index);
  }

  /**
   * @brief finds the metadata packet and copies it to buffer
   * 
   * @param ID 
   * @param buffer 
   * @return true 
   * @return false 
   */
  bool getPacket(ModuleID ID, byte* buffer){
    MetadataPacketReader packet = getPacket(ID);
    packet.copyPacket(buffer);
    return packet.intID() != 0;
  }
  
  /**
   * @brief Set a metadata packet with some checks. If the packet is new, checks the destination is empty. If the packet is an update, checks the destination has the same ID.
   * 
   * @param newPacket 
   * @return errorCode_t 
   */
  errorCode_t setMetadataPacket(byte *newPacket){
    MetadataPacketReader mpStruct(newPacket);

    errorCode_t error = isPacketValid(newPacket, mpStruct.metaIndex());
    if(errorCode_t::success != error){return error;}
    
    // const uint16_t location = mpStruct.metaIndex() * _packetSize;
    const uint16_t location = mpStruct.metadataAddress();
    
    // either: there shouldn't be a packet, or you're overwriting the address for the same config
    const bool doesPacketAlreadyExist = (isPacketValid(mpStruct.metaIndex()) == errorCode_t::success);
    const bool isPacketTheSame = (mpStruct.intID() == _metadata[location]);
    if(
      doesPacketAlreadyExist && !isPacketTheSame
    ){
      return errorCode_t::illegalAddress;
    }
    
    // write the new packet
    memcpy(&_metadata[location], newPacket, _packetSize);
    return errorCode_t::success;
  }

  errorCode_t setMetadataPacket(ModuleID ID, storageAddress_t configAddress, uint8_t metaIndex){
    byte buffer[_packetSize];
    MetadataPacketWriter writer(buffer);
    writer.setID(ID);
    writer.setAddress(configAddress);
    writer.setMetaIndex(metaIndex);
    return setMetadataPacket(buffer);
  }

  MetadataPacketReader getLargestDataAddress(){
    for(uint8_t i = 0; i < _packetSize; i++){emptyPacket[i] = 0;}

    MetadataPacketReader largestPacket(emptyPacket);
    MetadataPacketReader currentPacket(emptyPacket);
    for(uint8_t index = 0; index < getSize(); index++){
      currentPacket.setPacket(&_metadata[index*_packetSize]);
      if(
        (isPacketValid(currentPacket, index) == errorCode_t::success)
        && (currentPacket.address() >= largestPacket.address())
      ){
        largestPacket.setPacket(&_metadata[index*_packetSize]);
      }
    }
    return largestPacket;
  }
};




inline MetadataPacketReader MetadataFileIterator::getNext(){
  MetadataPacketReader packet = meta->getPacket(currentIndex);
  currentIndex++;
  return packet;
}

inline bool MetadataFileIterator::hasMore(){
  errorCode_t error = errorCode_t::failed;
  for(currentIndex; currentIndex < meta->getSize(); currentIndex++){
    if(errorCode_t::success == meta->isPacketValid(currentIndex)){
      return true;
    }
  }
  return false;
}

#endif