#pragma once

#include "device_config.h"

namespace mesh {

bool loadDeviceSettings(DeviceConfig* settings);
bool saveDeviceSettings(const DeviceConfig& settings);

}
