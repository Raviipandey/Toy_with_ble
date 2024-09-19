#include "initializer.h"
#include "ble_app.h"

extern WiFi *wifi;
extern System *sys;
extern BLE *ble;
extern Initializer *init;
extern char accessToken[1500];

uint8_t Initializer::outputBuff[];

void InitializeTask(void *params) {
    ESP_LOGW("TEST", "TASK CREATED");
    while(1) {
        init->handler();
    }
}


Initializer::~Initializer() {
    vTaskDelete(this->xHandle);
}

void Initializer::initialize(char* ssid, char* password, char* user1) {
    memset(this->ssid, 0, sizeof(this->ssid));
    memset(this->password, 0, sizeof(this->password));
    memset(this->user1, 0, sizeof(this->user1));
    memset(this->user2, 0, sizeof(this->user2));

    ESP_LOGW("TEST", "SSID: %s, PASSWORD: %s, USER: %s", ssid, password, user1);

    strcpy(this->ssid, ssid);
    strcpy(this->password, password);
    strcpy(this->user1, user1);

    xTaskCreate(InitializeTask, "initialize", 6000, NULL, 21, &xHandle);
    initState = eINIT_WIFI_CHECK;
}

void Initializer::verifyUserID(char* userToCheck, uint8_t userNo) {
    

    if(userNo == 1)
    {
        ESP_LOGW("TEST", "verifyingUserID1");
        memset(this->user1, 0, sizeof(this->user1));
        strcpy(this->user1, userToCheck);
    }
    else if(userNo == 2)
    {
        ESP_LOGW("TEST", "verifyingUserID2");
        memset(this->user2, 0, sizeof(this->user2));
        strcpy(this->user2, userToCheck);
    }
    
    xTaskCreate(InitializeTask, "initialize", 6000, NULL, 21, &xHandle);
    initState = eINIT_USER_ID_CHECK;
}

void Initializer::handler() {
    switch(initState) {
        case eINIT_IDLE:
        break;

        case eINIT_WIFI_CHECK:
            ESP_LOGW("TEST", "CHECKING CREDENTIALS");
            ESP_LOGW("Credentials", " %s  |  %s", ssid, password);
            if(wifi->bleWriteCredentials(ssid, password)) {
                initState = eINIT_USER_ID_CHECK;
            }
            else {
                initState = eINIT_IDLE;
            }
        break;

        case eINIT_USER_ID_CHECK:
            
            verifyUserIdWithServer();
            initState = eINIT_IDLE;
            vTaskDelete(xHandle);
        break;
    }
}

extern uint8_t tokenRequestFlag;
extern char accessToken[1500];
extern uint8_t validTokenFlag;

bool Initializer::verifyUserIdWithServer() {

    if(!validTokenFlag)
    {
        Downloader *downloader = new Downloader();
        downloader->downloadToken();
    }
    
    if(wifi->connect() == 0) {
        esp_http_client_config_t config = {
            .url = HTTPS_VERIFY_USERID_URL,
            .auth_type = HTTP_AUTH_TYPE_BASIC,
            .event_handler = Initializer::InitializerCallback,
            .buffer_size = DOWNLOAD_CHUNK_SIZE,    
            .buffer_size_tx = 1500,  //newly added line    
            .crt_bundle_attach = esp_crt_bundle_attach
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);

        char postData[150];
        sprintf(postData, "{\"macAddress\": \"%s\",\"user1Id\": \"%s\",\"user2Id\": \"%s\"}", sys->getMacID(), user1, user2);
        ESP_LOGW("sfbs", "\n\nPosting data:\n%s\n\n", postData);

        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "x-cubbies-box-token", accessToken);
        esp_http_client_set_post_field(client, postData, strlen(postData));

        esp_err_t err = esp_http_client_perform(client);
        if(err != ESP_OK) {
            wifi->disconnect();
            return false;
        }  
        esp_http_client_cleanup(client);

        cJSON *json = cJSON_Parse((char*)outputBuff);
        if(json == NULL) {
            ESP_LOGW("Testing", "INVALID JSON");
            return false;
        }
        
        
        if(strlen(user1)) {
            ESP_LOGW("Test", "USER1");
            cJSON *userJson = cJSON_GetObjectItemCaseSensitive(json, "user1");
            if(cJSON_IsTrue(userJson)) {
                ESP_LOGW("Test", "USER1 VERIFIED");
                sys->writeUserID(1, user1);
                ble->notify("9");
                //ble->notify("VerificationSuccess");
            }
            else {
                ESP_LOGW("Test", "USER1 FAILED");
                ble->notify("10");
                //ble->notify("VerificationFailed");
            }
        }
        else if(strlen(user2)) {
            ESP_LOGW("Test", "USER2");
            cJSON *userJson = cJSON_GetObjectItemCaseSensitive(json, "user2");
            if(cJSON_IsTrue(userJson)) {
                ESP_LOGW("Test", "USER2 VERIFIED");
                sys->writeUserID(2, user2);
                ble->notify("9");
                //ble->notify("VerificationSuccess");
            }
            else {
                ESP_LOGW("Test", "USER2 FAILED");
                ble->notify("10");
                //ble->notify("VerificationFailed");
            }
        }
    }
    wifi->disconnect();
    return true;
}

esp_err_t Initializer::InitializerCallback(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD("LOG", "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD("LOG", "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD("LOG", "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD("LOG", "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD("LOG", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;

        case HTTP_EVENT_ON_DATA:
            ESP_LOGW("LOG", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            memcpy(outputBuff, evt->data, evt->data_len);
            ESP_LOGW("LOG", "%s", outputBuff);
        break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGW("LOG", "HTTP_EVENT_ON_FINISH");
        break;

        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGW("LOG", "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) 
            {
                ESP_LOGW("LOG", "Last esp error code: 0x%x", err);
                ESP_LOGW("LOG", "Last mbedtls failure: 0x%x", mbedtls_err);
            }
        break;
    }
    return ESP_OK;
}




