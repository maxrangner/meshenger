idf_component_register(
    SRCS "mesh_service.cpp" "packet_screener.cpp"
    INCLUDE_DIRS "."
    REQUIRES protocol radio_service app_message
)
