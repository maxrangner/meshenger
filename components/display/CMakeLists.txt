idf_component_register(
    SRCS "oled_display.cpp" "epaper_display.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_spi
        esp_lcd
)
