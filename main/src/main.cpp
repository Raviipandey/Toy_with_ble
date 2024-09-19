#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"

// From inc
#include "main.h"
#include <led_app.h>
#include "player.h"
static const char *TAG = "MAIN";

LED *led = new LED();
System *sys = new System();
SdCard *sdcard = new SdCard();
WiFi *wifi = new WiFi();
Player *player = new Player();
BLE *ble = new BLE();
// Initializer *init = new Initializer();
// MP3Player player;

// Forward declaration for download task
void download_task(void *pvParameters);
void player_task(void *pvParameters);
// void player_state_task(void *pvParameters);

extern "C" void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (xTaskCreate(SystemTask, "system", 8192, NULL, 20, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create SystemTask");
    }
    else
    {
        ESP_LOGI(TAG, "SystemTask created successfully");
    }

    if (xTaskCreate(QbusTask, "qbus", 8192, NULL, 20, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create Qbus task");
    }
    else
    {
        ESP_LOGI(TAG, "Qbus task created successfully");
    }

    sdcard->open();
    
    sys->readUserIDs(); // uncomment this later

    if (wifi->init() == 0) // returns error(1) if creds are missing
    {
        if (wifi->ConfigSdCardCredentials() == 0)
        {
            if (wifi->connect() == 0)
            {
                ESP_LOGW("\n wifi connect", "success");
            }
        }
    }

    // Wait for Wi-Fi connection before starting the HTTP task
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    // ble->deinit();
    // ESP_LOGW("\n Verifying user id", "from main");
    // sys->ReadBoxUniqueID();
    // init->verifyUserIdWithServer();
    // Initialize SD card
    // ret = init_sd_card();
    // if (ret != ESP_OK)
    // {
    //     ESP_LOGE(TAG, "Failed to initialize SD card");
    //     return;
    // }

    // player.init(tagUid_qbus);
    // Create a task for updating the MP3 player and handling events
    // xTaskCreate(&player_task, "player_task", 4096, NULL, 5, NULL);

    // Replace the existing player task with:
    // xTaskCreate(&player_state_task, "player_state_task", 4096, NULL, 5, NULL);

    // Create a task for the initial login HTTP POST request
    // xTaskCreate(&http_post_task, "http_post_task", 8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "Main application started");
}

// void download_task(void *pvParameters)
// {
//     char *accessToken = (char *)pvParameters;
//     download_master_json();
//     process_direction_files();
//     process_audio_files("/sdcard/media/audio/");
//     vTaskDelay(pdMS_TO_TICKS(2000));
//     // abhishek();
//     vTaskDelete(NULL);
// }