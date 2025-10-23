idf_component_register(SRCS "app_main.c"
                            "wifi_handler.c"
                            "mqtt_handler.c"
                            "vibration_motor.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif esp_wifi driver esp_timer
                    INCLUDE_DIRS ".")
