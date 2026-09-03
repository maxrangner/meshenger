idf_component_register(
    SRCS "mesh_log.cpp" "mesh_service.cpp" "packet_screener.cpp" "node_state_storage.cpp" "mesh_core.cpp"
    INCLUDE_DIRS "."
    REQUIRES protocol radio_service app_event nvs_flash
)
