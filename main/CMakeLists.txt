

# Include NimBLE in idf_component_register
idf_component_register(SRCS "./src/main.cpp"
                            "./src/led_app.cpp"
                            "./src/iot_qbus_port.cpp"
                            "./src/iot_qbus.cpp"
                            "./src/qbus_s.cpp"
                            "./src/time_spent.cpp"
                            "./src/crc16.cpp"
                            "./src/system.cpp"
                            "./src/sdcard.cpp"
                            "./src/wifi.cpp"
                            "./src/downloader.cpp"
                            "./src/player.cpp"
                            "./src/ble_app.cpp"
                            "${NIMBLE_DIR}/src/NimBLEDevice.cpp"
                            "${NIMBLE_DIR}/src/NimBLEServer.cpp"
                            # Add other NimBLE source files as needed
                    INCLUDE_DIRS "./inc")
                    