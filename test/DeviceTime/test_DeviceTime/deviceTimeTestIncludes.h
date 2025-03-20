#pragma once

#define BUILD_TIMESTAMP 637609298000000

#include <unity.h>

#include "../RTCMocksAndHelpers/RTCMockWire.h"
#include "../RTCMocksAndHelpers/setTimeTestArray.h"
#include "../../nativeMocksAndHelpers/mockConfig.h"
#include "../RTCMocksAndHelpers/helpers.h"

#include "DeviceTime.h"
#include "onboardTimestamp.h"

// #include <iostream>

std::shared_ptr<ConfigManagerClass> globalConfigs;