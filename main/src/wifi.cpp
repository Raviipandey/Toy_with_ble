#include "wifi.h"

extern SdCard *sdcard;
extern WiFi *wifi;
// extern Log *log;
extern BLE *ble;
extern LED *led;

bool WiFi::connectFlag = false;
EventGroupHandle_t WiFi::eventGroup;
uint8_t wifiInitDoneFlag = 0;

uint8_t WiFi::init()
{
    if (wifiInitDoneFlag == 1)
    {
        return 0; // to avoid initialisng multiple times
    }

    eventGroup = xEventGroupCreate();

    esp_netif_init();

    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, NULL, &instance_got_ip));

    wifiInitDoneFlag = 1;
    return 0;
}

uint8_t WiFi::readCredentials()
{

    memset((char *)credentials.ssid, 0, sizeof(credentials.ssid));
    memset((char *)credentials.password, 0, sizeof(credentials.password));

    FILE *file = fopen(CREDENTIALS_SDCARD_LOCATION, "r");
    if (file != NULL)
    {
        ESP_LOGW("TEST_LOG", "CREDENTIALS READ SUCCESS");
        sdcard->read((uint8_t *)&credentials, sizeof(credentials_t), file);
        ESP_LOGW("WIFI", "Credentials: %s  |  %s", credentials.ssid, credentials.password);
        fclose(file);

        return 0; // success
    }
    else
    {
        memset((char *)credentials.ssid, 0, sizeof(credentials.ssid));
        memset((char *)credentials.password, 0, sizeof(credentials.password));
        ESP_LOGW("TEST_LOG", "CREDENTIALS READ FAILED");
        return 1; // error
    }
}

uint8_t WiFi::ConfigSdCardCredentials()
{
    if (readCredentials()) // returns error if file is missing
    {
        return 1; // error
    }
    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, credentials.ssid);
    strcpy((char *)wifi_config.sta.password, credentials.password);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI("TAG", "wifi_init_sta finished. %s  %s", wifi_config.sta.ssid, wifi_config.sta.password);
    return 0;
}

void WiFi::eventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (connectFlag)
        {
            esp_wifi_connect();
        }
        else
        {
            xEventGroupSetBits(eventGroup, WIFI_DISCONNECTED_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(eventGroup, WIFI_CONNECTED_BIT);
    }
}

uint8_t WiFi::connect()
{
    if (!strlen(credentials.ssid))
    {
        ESP_LOGW("WiFi", "credentials empty");
        return 1; // error
    }
    count++;
    if (connected)
    {
        ESP_LOGW("WiFi", "already connected");
        return 0; // success
    }
    else
    {
        ESP_LOGW("WiFi", "initiating connection");
        return __connect();
    }
}

uint8_t WiFi::__connect()
{
    ESP_LOGW("WiFi", "Connecting WiFi");
    // led->setPadState(LP_LED_WIFI_CONNECTION);
    connectFlag = true;
    esp_wifi_connect();
    EventBits_t bits = xEventGroupWaitBits(eventGroup, WIFI_CONNECTED_BIT | WIFI_FAILED_BIT, pdTRUE, pdFALSE, WIFI_WAIT_TIME);
    if (bits & WIFI_CONNECTED_BIT)
    {
        connected = true;
        // log->logEvent(eLOG_WIFI_CONNECTED);
        ESP_LOGW("WiFi", "Wifi connect success");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        return 0; // success
    }
    else if (bits & WIFI_FAILED_BIT)
    {
        ESP_LOGW("WiFi", "Wifi connect failed");
        return 1; // error
    }
    else
    {
        ESP_LOGW("WaitBit", "%lu", bits);
    }
    return 1; // error
}

bool WiFi::disconnect()
{
    if (!strlen(credentials.ssid))
    {
        return false;
    }
    count--;
    if (!count)
    {
        return __disconnect();
    }
    return false;
}

bool WiFi::__disconnect()
{
    ESP_LOGW("WiFi", "Disconnecting WiFi");
    connectFlag = false;
    esp_wifi_disconnect();
    EventBits_t bits = xEventGroupWaitBits(eventGroup, WIFI_DISCONNECTED_BIT | WIFI_FAILED_BIT, pdTRUE, pdFALSE, WIFI_WAIT_TIME);
    if (bits & WIFI_DISCONNECTED_BIT)
    {
        connected = false;
        return true;
    }
    else if (bits & WIFI_FAILED_BIT)
    {
        return false;
    }
    return false;
}

bool WiFi::isConnected()
{
    return this->connected;
}

void WiFi::writeCredentials(char *ssid, char *password)
{
    memset(this->credentials.ssid, 0, sizeof(this->credentials.ssid));
    memset(this->credentials.password, 0, sizeof(this->credentials.password));

    strcpy(this->credentials.ssid, ssid);
    strcpy(this->credentials.password, password);

    FILE *file = fopen(CREDENTIALS_SDCARD_LOCATION, "w");
    if (file != NULL)
    {
        sdcard->write((uint8_t *)&this->credentials, sizeof(credentials_t), file);
        ESP_LOGW("WIFI", "Credentials: %s  |  %s", this->credentials.ssid, this->credentials.password);
        fclose(file);
    }
}

bool WiFi::bleWriteCredentials(char *newSsid, char *newPassword)
{
    if (isConnected())
    {
        ESP_LOGW("BLEwrite", "Already connected- disconnecting");
        __disconnect();
        esp_wifi_stop();
    }

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, newSsid);
    strcpy((char *)wifi_config.sta.password, newPassword);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    ESP_LOGW("BLEwrite", "Starting wifi");
    esp_wifi_start();

    if (__connect() == 0)
    {
        if (!count)
        {
            __disconnect();
        }
        writeCredentials(newSsid, newPassword);
        ble->notify("11"); // do not send this response to avoid multiple replies
        // ble->notify("CredentialsSuccess");
        return true;
    }
    else
    {
        ble->notify("12");
        // ble->notify("WiFiConnectionFailed");
        if (count)
        {
            wifi_config_t wifi_config = {};
            strcpy((char *)wifi_config.sta.ssid, credentials.ssid);
            strcpy((char *)wifi_config.sta.password, credentials.password);
            wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

            esp_wifi_set_mode(WIFI_MODE_STA);
            esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            esp_wifi_start();
            __connect();
        }
        return false;
    }
}