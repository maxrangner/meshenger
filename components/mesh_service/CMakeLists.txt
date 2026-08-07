idf_component_register(
    SRCS "mesh_service.cpp" "packet_screener.cpp" "device_config_storage.cpp"
    INCLUDE_DIRS "."
    REQUIRES protocol radio_service app_message utils nvs_flash
)
