
idf_component_register(
    SRCS
        "app_main.cpp"
        "CDS1307.cpp"
    PRIV_REQUIRES
        spi_flash
    REQUIRES
        driver
        esp_timer
        log
        freertos
    INCLUDE_DIRS
        ""
)
