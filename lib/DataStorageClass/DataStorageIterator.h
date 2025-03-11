#ifndef __DATA_STORAGE_ITERATOR__
#define __DATA_STORAGE_ITERATOR__

#include "ProjectDefines.h"
#include "storageHAL.h"

enum class StoredDataTypes {
  events = 0,
  modes = 1
};

template<typename numberType, typename Struct_t>
class IterableCollection{
  private:
    numberType _numberStored;
    numberType _chunkNumber = 0;
    numberType _nPacketsInBuffer = 0;
    Struct_t _buffer[DataPreloadChunkSize];

    std::shared_ptr<StorageHALInterface> _storageHAL;

    void _fetchChunk(numberType chunkNumber);
  public:
    IterableCollection(
      std::shared_ptr<StorageHALInterface> storageHAL,
      numberType numberStored
    ) : _storageHAL(storageHAL), _numberStored(numberStored){
      _fetchChunk(0);
    };

    Struct_t getObjectAt(numberType objectNumber);

    numberType getNumberStored(){return _numberStored;}
};

template <typename numberType, typename Struct_t>
inline void IterableCollection<numberType, Struct_t>::_fetchChunk(numberType objectNumber){
  _chunkNumber = objectNumber / DataPreloadChunkSize;
  _nPacketsInBuffer = _storageHAL->fillChunk(_buffer, objectNumber);
}

template <typename numberType, typename Struct_t>
inline Struct_t IterableCollection<numberType, Struct_t>::getObjectAt(numberType objectNumber){
  if(objectNumber >= _numberStored
    || objectNumber < 0
  ){
    // TODO: throw an out-of-bounds error
    Struct_t emptyStruct;
    return emptyStruct;
  }
  if(objectNumber < _chunkNumber*DataPreloadChunkSize
    || objectNumber >= (_chunkNumber+1)*DataPreloadChunkSize
  ){
    // if requested object isn't in the current chunk,
    // load the correct chunk
    _fetchChunk(objectNumber);
  }
  return _buffer[objectNumber % DataPreloadChunkSize];
}

template<typename numberType, typename Struct_t>
class DataStorageIterator : public IterableCollection<numberType, Struct_t>{
  private:
    numberType _currentNumber = 0;

  public:
  /**
   * @brief Construct a new Data Storage Iterator object
   * 
   * @param storageHAL shared_ptr to the storage HAL
   * @param numberStored number of stored objects
   */
  DataStorageIterator(std::shared_ptr<StorageHALInterface> storageHAL, numberType numberStored): IterableCollection<numberType, Struct_t>(storageHAL, numberStored){
    // _fetchChunk(0);
    };

    Struct_t getNext(){
      if(!hasMore()){
        // TODO: throw an error maybe? or return a null object?
        // return early if all the structs have been loaded
        Struct_t emptyStruct;
        return emptyStruct;
      };
      _currentNumber++; // increment early

      return this->getObjectAt(_currentNumber - 1);
    };

    bool hasMore(){return this->getNumberStored() > _currentNumber;};

    void reset(){_currentNumber = 0;};
};

#endif