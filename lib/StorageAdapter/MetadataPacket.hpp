#ifndef __METADATA_PACKET_HPP__
#define __METADATA_PACKET_HPP__

#include <cstdint>

#include "ProjectDefines.h"
#include "StorageAdapter.hpp"

/**
 * @brief this is the single source of truth for metadata packet structure
 * 
 */
struct MetadataPacketWriter{
  private:
  byte *packet;
  public:
  static const uint8_t IDLocation = 0;
  static const uint8_t addressLocation = sizeof(ModuleID);
  static const uint8_t indexLocation = addressLocation + sizeof(storageAddress_t);
  static const uint8_t size = indexLocation + 1;
  
  ModuleID ID(){return *reinterpret_cast<ModuleID*>(&packet[IDLocation]);}
  uint8_t intID(){return *reinterpret_cast<uint8_t*>(&packet[IDLocation]);}
  storageAddress_t address(){return *reinterpret_cast<storageAddress_t*>(&packet[addressLocation]);}
  uint8_t metaIndex(){return *reinterpret_cast<uint8_t*>(&packet[indexLocation]);}

  void setID(ModuleID newID){memcpy(&packet[IDLocation], &newID, sizeof(newID));};
  void setID(uint8_t newID){memcpy(&packet[IDLocation], &newID, sizeof(newID));};
  void setAddress(storageAddress_t newAddress){memcpy(&packet[addressLocation], &newAddress, sizeof(newAddress));};
  void setMetaIndex(uint8_t newMetaIndex){memcpy(&packet[indexLocation], &newMetaIndex, sizeof(newMetaIndex));};

  MetadataPacketWriter(byte *packet) : packet(packet){};
  
  MetadataPacketWriter* setPacket(byte *newPacket){packet = newPacket; return this;};
  byte *getPacket(){return packet;}

  /**
   * @brief returns the metadata index * packet size
   * 
   * @return storageAddress_t 
   */
  storageAddress_t metadataAddress(){
    return metaIndex()*size;
  }

  void copyPacket(byte dstBuffer[size]){
    memcpy(dstBuffer, packet, size);
  }
};

struct MetadataPacketReader{
private:
  MetadataPacketWriter writer;
public:
  ModuleID ID(){return writer.ID();}
  uint8_t intID(){return writer.intID();}
  storageAddress_t address(){return writer.address();}
  uint8_t metaIndex(){return writer.metaIndex();}

  /**
   * @brief returns the metadata index * packet size
   * 
   * @return storageAddress_t 
   */
  storageAddress_t metadataAddress(){return writer.metadataAddress();}

  MetadataPacketReader(byte *packet) : writer(packet){};
  MetadataPacketReader() : writer(nullptr){};

  MetadataPacketReader* setPacket(byte *newPacket){writer.setPacket(newPacket); return this;}
  void copyPacket(byte dstBuffer[MetadataPacketWriter::size]){writer.copyPacket(dstBuffer);}
  uint8_t size(){return MetadataPacketWriter::size;}
  const byte* getPacket(){return writer.getPacket();}
};

struct MetadataArrayIterator{
  private:
  uint16_t _nextAddress = 0;
  byte *_metadataArray;
  public:
  const uint16_t byteSize;  // size of the metadata array in bytes

  bool hasMore(){
    return (byteSize > (_nextAddress + MetadataPacketWriter::size));
  }

  /**
   * @brief Gets a reader to the next array position. if it's at the end of the array, just returns the final element
   * 
   * @return MetadataPacketReader 
   */
  MetadataPacketReader getNext(){
    uint16_t currentAddress = _nextAddress;
    if(hasMore()){
      _nextAddress += MetadataPacketWriter::size;
    }
    return MetadataPacketReader(&_metadataArray[currentAddress]);
  }
  
  MetadataArrayIterator(byte *metadataArray, uint16_t arraySizeInBytes)
    : _metadataArray(metadataArray), byteSize(arraySizeInBytes){}
};


struct MetadataPacketContainer{
public:
  // Even if the packet grows larger than 4 bytes, 4 0s should be enough to mark it as null
  byte packet[MetadataPacketWriter::size] = {0, 255, 255, 255};

  MetadataPacketReader getReader(){
    return MetadataPacketReader(packet);
  }
  
  MetadataPacketContainer(MetadataPacketReader packetReader){
    packetReader.copyPacket(packet);
  }

  MetadataPacketContainer(){};
};

#endif