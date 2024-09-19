// #include "box_login.h"
// #include "download_direction.h"
// #include "download_master_json.h"
// #include "download_mp3.h"
#include "sdcard.h"
// // #include "wifi_config.h"
#include "wifi.h"
// #include "metadata.h"
// #include "player.h"
// #include "uart.h"

#include "downloader.h"
#include "esp_includes.h"
#include "crc16.h"
#include "qbus_s.h"
#include "iot_qbus_port.h"
#include "iot_qbus.h"
#include "time_spent.h"
#include "led_app.h"
#include "system.h"
#include "ble_app.h"
#include "initializer.h"
#include "player.h"


// #include "clock.h"
// #include "test_player.h"

#include "driver/uart.h"
#include "soc/uart_reg.h"
#include "esp32/rom/uart.h"
#include <cJSON.h>

#define SYSTEM_TASK_DELAY 200/portTICK_PERIOD_MS
using namespace std;