idf_component_register(SRCS "app_main.cpp" "DHT22.cpp"
                    PRIV_REQUIRES spi_flash
                    REQUIRES log freertos driver esp_timer
                    INCLUDE_DIRS ""
                    )