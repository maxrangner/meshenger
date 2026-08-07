#include "device_config_storage.h"

#include "esp_log.h"
#include "nvs.h"

static const char* TAG = "settings storage";

namespace mesh {

bool loadDeviceSettings(DeviceConfig* config) {
    if (config == nullptr) {
        return false;
    }

    DeviceConfig loaded_config{};

    nvs_handle_t handle;
    esp_err_t err = nvs_open("device", NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved device settings");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open read failed: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_get_u8(handle, "proto_ver", &loaded_config.protocol_version);
    if (err != ESP_OK) {
        nvs_close(handle);
        ESP_LOGW(TAG, "nvs_get failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_get_u8(handle, "dict_ver", &loaded_config.dictionary_version);
    if (err != ESP_OK) {
        nvs_close(handle);
        ESP_LOGW(TAG, "nvs_get failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_get_u64(handle, "group_id", &loaded_config.group_id);
    if (err != ESP_OK) {
        nvs_close(handle);
        ESP_LOGW(TAG, "nvs_get failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_get_u64(handle, "origin_id", &loaded_config.origin_device_id);
    if (err != ESP_OK) {
        nvs_close(handle);
        ESP_LOGW(TAG, "nvs_get failed: %s", esp_err_to_name(err));
        return false;
    }

    nvs_close(handle);

    // if (loaded_config.protocol_version != kDeviceSettingsVersion) {
    //     ESP_LOGW(TAG, "Saved settings version mismatch: %u", loaded_config.version);
    //     return false;
    // }

    *config = loaded_config;
    return true;
}

bool saveDeviceSettings(const DeviceConfig& config) {
    DeviceConfig config_to_save = config;
    config_to_save.protocol_version = 1;

    nvs_handle_t handle;
    esp_err_t err = nvs_open("device", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open write failed: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_u8(handle, "proto_ver", config_to_save.protocol_version);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    err = nvs_set_u8(handle, "dict_ver", config_to_save.dictionary_version);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    err = nvs_set_u64(handle, "group_id", config_to_save.group_id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    err = nvs_set_u64(handle, "origin_id", config_to_save.origin_device_id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    nvs_close(handle);

    return true;
}

}
