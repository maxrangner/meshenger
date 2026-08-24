#pragma once

#include "device_state.h"

namespace mesh {

bool load_device_settings(DeviceState* state);
bool save_device_settings(const DeviceState& state);

}
