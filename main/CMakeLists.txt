idf_component_register(
    SRCS "app_log.cpp" "application.cpp" "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        mesh_service
        radio_service
        protocol
        app_event
        display
        ui
        esp-idf-button-driver
        nvs_flash
        esp_app_format
        esp_driver_i2c
        esp_driver_gpio
        esp_driver_spi

)
