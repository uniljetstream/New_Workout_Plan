idf_component_register(SRCS "airmouse.c"
                            "app_main.c"
                            "wifi_handler.c"
                            "mqtt_handler.c"
                            "sensor_task.c"
                            "mpu6050.c"
                            "button_handler.c"
                            "vibration_motor.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif esp_wifi driver esp_timer
                    INCLUDE_DIRS ".")
