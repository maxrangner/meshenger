#pragma once

#include "device_state.h"

namespace mesh {

bool loadDeviceSettings(DeviceState* state);
bool saveDeviceSettings(const DeviceState& state);

}
