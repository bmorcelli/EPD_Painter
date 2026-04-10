idf_component_register(
    SRC_DIRS "src"
    INCLUDE_DIRS "src"
    REQUIRES driver nvs_flash esp_timer freertos esp_rom soc esp_hw_support hal
)
