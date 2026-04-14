idf_component_register(SRCS "app_main.cpp" "DHT22.cpp"  "VMA320.cpp"
                    PRIV_REQUIRES spi_flash
                    REQUIRES log freertos driver esp_timer esp_adc
                    INCLUDE_DIRS ""
                    )