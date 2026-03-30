idf_component_register(SRCS "CButton.cpp" "app_main.cpp" "CLed.cpp"
                    PRIV_REQUIRES spi_flash
                    REQUIRES driver esp_timer
                    INCLUDE_DIRS ""
                    )