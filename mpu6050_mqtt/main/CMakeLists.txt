idf_component_register(SRCS "app_main.c"
                            "wifi_handler.c"
                            "mqtt_handler.c"
                            "sensor_task.c"
                            "mpu6050.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif esp_wifi driver
                    INCLUDE_DIRS ".")
