idf_component_register(
    SRCS "radio_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES RadioLib esp_driver_gpio protocol app_event
)
