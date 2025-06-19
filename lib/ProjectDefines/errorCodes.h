#ifndef __PROJECT_ERROR_CODES__
#define __PROJECT_ERROR_CODES__

#include <stdint.h>

/**
 * TODO: use esp error type
 * 
 */
enum errorCode_t : uint8_t{
  failed = 0,
  success,

  badValue,
  bad_time,
  badID,  // i.e. the ID is out of range
  bad_uuid, // i.e. the uuid is duplicated
  
  event_not_found,
  modeNotFound,
  IDNotInUse,
  notFound,

  readFailed,   // multiple read attempts returned different values
  writeFailed,  // multiple write-read attempts returned different values
  wearAttemptLimitReached,  // limit reached for finding a new storage location
  storage_full,
  outOfBounds,
  illegalAddress,

  checksumFailed,
  busy,
};

#endif