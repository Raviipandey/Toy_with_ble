#ifndef __ESP_INCLUDES_H__
#define __ESP_INCLUDES_H__


//#include <driver/adc.h>

#include <sys/param.h>

#include "esp_adc/adc_oneshot.h" //Include the new ADC oneshot driver
#include "esp_adc/adc_continuous.h" //Include the new ADC continuous driver


#include "esp_mac.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/timers.h"

#include "esp_sntp.h"

#include "esp_ota_ops.h"
#include "esp_https_ota.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "fatfs_stream.h"
#include "raw_stream.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "periph_sdcard.h"
#include "board.h"

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#else
#define ESP_IDF_VERSION_VAL(major, minor, patch) 1
#endif

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif


#include "lwip/err.h"
#include "lwip/sys.h"

#include "audio_mem.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "filter_resample.h"

#include "esp_vfs_fat.h"


#include "esp_spiffs.h"

#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "periph_touch.h"
#include "periph_button.h"
#include "input_key_service.h"
#include "periph_adc_button.h"
#include "board.h"

#include "sdcard_list.h"
#include "sdcard_scan.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "esp_timer.h"

#include <stdio.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#include "driver/sdmmc_host.h"
#endif


#include <stdlib.h>
#include <string.h>

#include "esp_system.h"
#include "driver/spi_master.h"


#include <stdint.h>
#include <stdbool.h>
#include "driver/uart.h"
#include "esp_intr_alloc.h"
#include "soc/uart_reg.h"
#include "esp32/rom/uart.h"

#include "driver/touch_pad.h"


#include "http_stream.h"

#include "raw_stream.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "periph_sdcard.h"


#include "esp_netif.h"

#include "esp_tls.h"

#include "esp_crt_bundle.h"
#include <cJSON.h>
#include "esp_http_client.h"


#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#else
#define ESP_IDF_VERSION_VAL(major, minor, patch) 1
#endif

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif






#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOSConfig.h"
/* BLE */
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"



#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"


#include "nimble/ble.h"
#include "modlog/modlog.h"


#include <dirent.h>

#include <NimBLEDevice.h>

#include "esp_heap_trace.h"
#include "esp_heap_caps.h"

#endif