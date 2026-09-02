#include "node_state_storage.h"

#include "esp_log.h"
#include "nvs.h"

constexpr char TAG[] = "settings storage";

namespace mesh {

bool load_node_state(LocalNodeState* state) {
    if (state == nullptr) {
        return false;
    }

    LocalNodeState loaded_config{};

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("device", NVS_READONLY, &nvs_handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No saved device settings");
        return false;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open read failed: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_get_u8(nvs_handle, "proto_ver", &loaded_config.protocol_version);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        ESP_LOGW(TAG, "nvs_get failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_get_u8(nvs_handle, "dict_ver", &loaded_config.phrase_dictionary_version);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        ESP_LOGW(TAG, "nvs_get failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_get_u64(nvs_handle, "group_id", &loaded_config.group_id);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        ESP_LOGW(TAG, "nvs_get failed: %s", esp_err_to_name(err));
        return false;
    }
    err = nvs_get_u64(nvs_handle, "origin_id", &loaded_config.device_id);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        ESP_LOGW(TAG, "nvs_get failed: %s", esp_err_to_name(err));
        return false;
    }

    nvs_close(nvs_handle);

    // if (loaded_config.protocol_version != kDeviceSettingsVersion) {
    //     ESP_LOGW(TAG, "Saved settings version mismatch: %u", loaded_config.version);
    //     return false;
    // }

    *state = loaded_config;
    return true;
}

bool save_node_state(const LocalNodeState& state) {
    // LocalNodeState config_to_save = state;
    // config_to_save.protocol_version = 1;

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("device", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open write failed: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_u8(nvs_handle, "proto_ver", state.protocol_version);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set failed: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    err = nvs_set_u8(nvs_handle, "dict_ver", state.phrase_dictionary_version);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set failed: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    err = nvs_set_u64(nvs_handle, "group_id", state.group_id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set failed: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    err = nvs_set_u64(nvs_handle, "origin_id", state.device_id);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_set failed: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_commit failed: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return false;
    }
    nvs_close(nvs_handle);

    return true;
}

}
