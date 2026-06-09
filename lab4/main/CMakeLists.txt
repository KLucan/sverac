idf_component_register(
    SRCS
        "app_main.cpp"

    PRIV_REQUIRES
        spi_flash

    REQUIRES
        bt
        nvs_flash
        log
        freertos

    INCLUDE_DIRS
        ""
)
