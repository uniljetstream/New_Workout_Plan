idf_component_register(
    SRCS "wifi.c" "main.c" "hardware.c" "ui.c" "sensor.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl st7789 cst816s driver esp_timer esp_wifi esp_event esp_netif nvs_flash esp_system
)
