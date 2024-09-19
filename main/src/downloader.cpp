#include "downloader.h"
#include "main.h"
#include <string>
#include <vector>

extern WiFi *wifi;
extern Downloader *downloader;
extern SdCard *sdcard;

FILE *Downloader::file = NULL;
extern System *sys;

static const char *TAG = "BOX_LOGIN";
static uint8_t outputBuff[2048] = {0};

Downloader::Downloader()
{
    ESP_LOGI(TAG, "Available heap: %lu", esp_get_free_heap_size());
}


bool Downloader::begin(char *toy)
{
    memset(this->toy, 0, sizeof(this->toy));
    strcpy(this->toy, toy);

    if (wifi->connect() == 0)
    {
        this->createFolders();
        this->downloadToken();
        this->download_master_json();
        this->process_direction_files();
        bool success = this->process_audio_files("/sdcard/media/audio/");
        memset(this->toy, 0, sizeof(this->toy));
        
        if (!success)
        {
            ESP_LOGE(TAG, "Failed to process audio files. Aborting.");
            // Here you can add any cleanup or error handling code
            return false;
        }
        
        return true;
    }
    return false;
}


void Downloader::createFolders()
{
    char folderName[150];

    sprintf(folderName, "/sdcard/media/toys/%s", this->toy);
    mkdir(folderName, 0775);
}

uint8_t tokenRequestFlag = 0;
char accessToken[40] = {0};
uint8_t validTokenFlag = 0;

void Downloader::downloadToken()
{
    if (validTokenFlag)
    {
        return;
    }
    if (wifi->connect() == 0)
    {
        tokenRequestFlag = 1;
        esp_http_client_config_t config = {
            .url = HTTPS_BOX_LOGIN_URL,
            .auth_type = HTTP_AUTH_TYPE_BASIC,
            .event_handler = Downloader::TokenCallback,
            .buffer_size = DOWNLOAD_CHUNK_SIZE,
            .buffer_size_tx = 1500,
            .crt_bundle_attach = esp_crt_bundle_attach};

        client = esp_http_client_init(&config);

        char postData[100];

        sprintf(postData, "{\"macAddress\": \"%s\", \"boxUniqueId\": \"%s\"}", sys->getMacID(), sys->getUniqueBoxID());
        // sprintf(postData, "{\"macAddress\": \"C0:49:EF:48:FD:A0\", \"boxUniqueId\": \"58be82ed-b3bf-4848-bb7f-02da479e7a1f\"}");
        ESP_LOGW("tokenPost", "\n\nPosting data:\n%s\n\n", postData);

        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");

        esp_http_client_set_post_field(client, postData, strlen(postData));

        esp_err_t err = esp_http_client_perform(client);

        int httpResponseCode = 0;
        if (err != ESP_OK)
        {
            // error
        }
        else
        {
            httpResponseCode = esp_http_client_get_status_code(client);
            ESP_LOGI("LOG", "HTTP POST Status = %d, content_length = %" PRId64, httpResponseCode, esp_http_client_get_content_length(client));
        }
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        vTaskDelay(SYSTEM_TASK_DELAY);

        if ((httpResponseCode < 200) || (httpResponseCode > 299)) // if http returned error
        {
            char errUrl[200];
            sprintf(errUrl, "/sdcard/media/toys/%s/err.cubbies", this->toy);
            file = fopen(errUrl, "w");
            if (file != NULL)
            {
                uint8_t err = 1;
                sdcard->write((uint8_t *)&err, sizeof(int), file);
                fclose(file);
            }
        }

        tokenRequestFlag = 0;
    }
}

esp_err_t Downloader::TokenCallback(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;

    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;

    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;

    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;

    case HTTP_EVENT_ON_HEADER:
        if (tokenRequestFlag && !validTokenFlag)
        {
            if (memcmp(evt->header_key, "x-cubbies-box-token", 19) == 0)
            {
                if (strlen(evt->header_value) > 40)
                {
                    ESP_LOGI("Error", "Token too long. Max limit = 40 characters");
                    break;
                }
                ESP_LOGI("HeaderFound", "key=%s, value=%s", evt->header_key, evt->header_value);
                strncpy(accessToken, evt->header_value, sizeof(accessToken) - 1);
                accessToken[sizeof(accessToken) - 1] = '\0';
                ESP_LOGI("accessToken", "value=%s", accessToken);
                validTokenFlag = 1;
            }
        }
        break;

    case HTTP_EVENT_ON_DATA:
        if (evt->data_len > sizeof(outputBuff) - 1)
        {
            ESP_LOGE(TAG, "Received data length exceeds buffer size");
            break;
        }
        memcpy(outputBuff, evt->data, evt->data_len);
        outputBuff[evt->data_len] = '\0';
        ESP_LOGI(TAG, "Response Data: %s", outputBuff);
        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");

        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

// ---------------------------MasterJSON Section---------------------------

static const char *TAG2 = "MASTER_JSON_DOWNLOAD";

static char *response_buffer = NULL;
static int response_buffer_size = 0;

// Store direction file names
char **direction_file_names = NULL;
int direction_file_count = 0;

// Store N values from media array
char **N_server = NULL;
int N_count = 0;
cJSON *media_json = NULL;

// Global variables for access token and request body
char baseUrl[256] = {0}; // Initialize the baseUrl variable
char request_body[500];

esp_err_t Downloader::create_directory(const char *path)
{
    char temp_path[256];
    char *p = NULL;
    size_t len;

    snprintf(temp_path, sizeof(temp_path), "%s", path);
    len = strlen(temp_path);
    if (temp_path[len - 1] == '/')
        temp_path[len - 1] = 0;
    for (p = temp_path + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            if (mkdir(temp_path, S_IRWXU) != 0 && errno != EEXIST)
            {
                ESP_LOGE(TAG2, "Failed to create directory %s: %s", temp_path, strerror(errno));
                return ESP_FAIL;
            }
            *p = '/';
        }
    }
    if (mkdir(temp_path, S_IRWXU) != 0 && errno != EEXIST)
    {
        ESP_LOGE(TAG2, "Failed to create directory %s: %s", temp_path, strerror(errno));
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t Downloader::MasterjsonCallback(esp_http_client_event_t *evt, void *context)
{
    static FILE *file = NULL;
    static char file_path[256] = {0};
    Downloader *downloader = static_cast<Downloader *>(context);

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG2, "HTTP_EVENT_ERROR");
        break;

    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG2, "HTTP_EVENT_REDIRECT");
        break;

    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG2, "HTTP_EVENT_ON_CONNECTED");
        break;

    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG2, "HTTP_EVENT_HEADER_SENT");
        break;

    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG2, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;

    case HTTP_EVENT_ON_DATA:
        if (evt->data_len > 0)
        {
            response_buffer = (char *)realloc(response_buffer, response_buffer_size + evt->data_len + 1);
            if (response_buffer == NULL)
            {
                ESP_LOGE(TAG2, "Failed to allocate memory for response buffer");
                return ESP_FAIL;
            }
            memcpy(response_buffer + response_buffer_size, evt->data, evt->data_len);
            response_buffer_size += evt->data_len;
            response_buffer[response_buffer_size] = '\0';

            if (file == NULL)
            {
                // Create the directory path
                sprintf(file_path, "/sdcard/media/toys/%s/", tagUid_qbus);

                // Create the directory structure if it does not exist
                if (create_directory(file_path) != ESP_OK)
                {
                    ESP_LOGE(TAG2, "Failed to create directories");
                    return ESP_FAIL;
                }

                // Append the file name to the directory path
                strcat(file_path, "metadata.cubbies");

                // Open the file for writing
                file = fopen(file_path, "w");
                if (file == NULL)
                {
                    ESP_LOGE(TAG2, "Failed to open file for writing: %s", strerror(errno));
                    return ESP_FAIL;
                }
            }

            fwrite(evt->data, 1, evt->data_len, file);
        }
        break;

    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG2, "HTTP_EVENT_ON_FINISH");
        ESP_LOGI(TAG2, "Response Data: %s", response_buffer);

        if (file != NULL)
        {
            fclose(file);
            file = NULL;
            ESP_LOGI(TAG2, "Response Data written to metadata.txt file successfully");

            // Call the function to parse JSON response and store metadata
            downloader->parse_and_store_metadata(response_buffer, response_buffer_size);
        }

        free(response_buffer);
        response_buffer = NULL;
        response_buffer_size = 0;

        break;

    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG2, "HTTP_EVENT_DISCONNECTED");
        if (response_buffer)
        {
            free(response_buffer);
            response_buffer = NULL;
            response_buffer_size = 0;
        }
        break;
    }
    return ESP_OK; // Ensure the function always returns ESP_OK
}

void Downloader::download_master_json()
{
    esp_http_client_config_t config = {};
    config.url = HTTPS_GET_USER_TOY_MEDIA_URL;
    config.event_handler = [](esp_http_client_event_t *evt)
    {
        return Downloader::MasterjsonCallback(evt, static_cast<Downloader *>(evt->user_data));
    };
    config.user_data = this;
    config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Modified request body to include userIds as an array
    sprintf(request_body, "{\"rfid\":\"%s\",\"userIds\":[\"%s\"],\"macAddress\":\"%s\"}",
            this->toy, sys->getUniqueBoxID(), sys->getMacID());

    // Log the request body for debugging
    ESP_LOGI(TAG2, "Request body: %s", request_body);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "x-cubbies-box-token", accessToken);
    esp_http_client_set_post_field(client, request_body, strlen(request_body));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG2, "HTTP POST Status = %d, content_length = %" PRId64,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG2, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    esp_http_client_close(client);
}

void Downloader::update_N_server(char **sd_files, int sd_file_count)
{
    char **new_N_server = NULL;
    int new_N_count = 0;

    // Allocate memory for the new N_server array
    new_N_server = (char **)malloc(N_count * sizeof(char *));
    if (new_N_server == NULL)
    {
        ESP_LOGE(TAG2, "Failed to allocate memory for new N_server");
        return;
    }

    // Compare and copy unique values
    for (int i = 0; i < N_count; i++)
    {
        int found = 0;
        for (int j = 0; j < sd_file_count; j++)
        {
            if (strcmp(N_server[i], sd_files[j]) == 0)
            {
                found = 1;
                break;
            }
        }
        if (!found)
        {
            new_N_server[new_N_count] = strdup(N_server[i]);
            if (new_N_server[new_N_count] == NULL)
            {
                ESP_LOGE(TAG2, "Failed to allocate memory for new N_server value");
                // Free allocated memory before returning
                for (int k = 0; k < new_N_count; k++)
                {
                    free(new_N_server[k]);
                }
                free(new_N_server);
                return;
            }
            new_N_count++;
        }
    }

    // Free the old N_server array
    free_N_server();

    // Update N_server with the new values
    N_server = new_N_server;
    N_count = new_N_count;

    ESP_LOGI(TAG2, "Updated N_server array. New count: %d", N_count);
}

void Downloader::free_N_server()
{
    if (N_server != NULL)
    {
        for (int i = 0; i < N_count; i++)
        {
            free(N_server[i]);
        }
        free(N_server);
        N_server = NULL;
        N_count = 0;
    }
}

const char *Downloader::get_direction_file_name(int index)
{
    if (index >= 0 && index < direction_file_count)
    {
        return direction_file_names[index];
    }
    return NULL;
}

const char *Downloader::get_N_value(int index)
{
    if (index >= 0 && index < N_count)
    {
        return N_server[index];
    }
    return NULL;
}

// -------------------------------------------------Processing Metadata--------------------------

esp_err_t Downloader::create_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "w");
    if (file == NULL)
    {
        ESP_LOGE(TAG2, "Failed to open file for writing: %s", strerror(errno));
        return ESP_FAIL;
    }
    fprintf(file, "%s", content);
    fclose(file);
    return ESP_OK;
}

// Helper function to write an array to a file
esp_err_t Downloader::write_array_to_file(const char *file_path, const int *array, int size)
{
    FILE *file = fopen(file_path, "w");
    if (file == NULL)
    {
        ESP_LOGE(TAG2, "Failed to open file for writing: %s", strerror(errno));
        return ESP_FAIL;
    }
    for (int i = 0; i < size; i++)
    {
        if (i > 0)
        {
            fprintf(file, ",");
        }
        fprintf(file, "%d", array[i]);
    }
    fclose(file);
    return ESP_OK;
}

void Downloader::parse_and_store_metadata(const char *response_buffer, size_t response_buffer_size)
{
    cJSON *json_response = cJSON_ParseWithLength(response_buffer, response_buffer_size);
    if (json_response == NULL)
    {
        ESP_LOGE(TAG2, "Failed to parse JSON response");
        return;
    }

    // Parse JSON and store baseURL
    cJSON *base_url_json = cJSON_GetObjectItem(json_response, "baseUrl");
    if (base_url_json != NULL && cJSON_IsString(base_url_json))
    {
        strncpy(baseUrl, base_url_json->valuestring, sizeof(baseUrl) - 1);
        baseUrl[sizeof(baseUrl) - 1] = '\0';
        ESP_LOGI(TAG2, "Stored base URL: %s", baseUrl);
    }
    else
    {
        ESP_LOGE(TAG2, "Failed to get baseUrl from JSON response");
    }

    // Parse JSON and store directionFiles
    cJSON *direction_files_json = cJSON_GetObjectItem(json_response, "directionFiles");
    if (direction_files_json != NULL && cJSON_IsArray(direction_files_json))
    {
        direction_file_count = cJSON_GetArraySize(direction_files_json);
        direction_file_names = (char **)malloc(sizeof(char *) * direction_file_count);

        for (int i = 0; i < direction_file_count; i++)
        {
            cJSON *file_name = cJSON_GetArrayItem(direction_files_json, i);
            if (cJSON_IsString(file_name))
            {
                direction_file_names[i] = strdup(file_name->valuestring);
                ESP_LOGI(TAG2, "Stored direction file name %d: %s", i, direction_file_names[i]);
            }
        }
    }
    else
    {
        ESP_LOGE(TAG2, "Failed to get directionFiles array from JSON response");
    }

    // Extract N values from media array

    // Store media_json
    cJSON *media_json_temp = cJSON_GetObjectItem(json_response, "media");
    if (media_json_temp != NULL && cJSON_IsArray(media_json_temp))
    {
        media_json = cJSON_Duplicate(media_json_temp, 1); // Duplicate media_json to preserve data
        ESP_LOGI(TAG2, "Stored media JSON successfully");
    }
    else
    {
        ESP_LOGE(TAG2, "Failed to get media array from JSON response");
    }
    if (media_json != NULL && cJSON_IsArray(media_json))
    {
        int media_count = cJSON_GetArraySize(media_json);
        N_server = (char **)malloc(sizeof(char *) * media_count * 3); // Allocate space for N, N+C, and N+W

        int server_index = 0; // To track the index in N_server array

        for (int i = 0; i < media_count; i++)
        {
            cJSON *media_item = cJSON_GetArrayItem(media_json, i);
            cJSON *N_value = cJSON_GetObjectItem(media_item, "N");
            cJSON *T_value = cJSON_GetObjectItem(media_item, "T");

            if (cJSON_IsString(N_value) && cJSON_IsNumber(T_value))
            {
                N_server[server_index] = strdup(N_value->valuestring);
                ESP_LOGI(TAG2, "Stored N value %d: %s", server_index, N_server[server_index]);
                server_index++;

                if (T_value->valueint == 1)
                {
                    // Allocate and store N+C
                    size_t len = strlen(N_value->valuestring);
                    N_server[server_index] = (char *)malloc(len + 2); // +1 for 'C' and +1 for '\0'
                    snprintf(N_server[server_index], len + 2, "%sC", N_value->valuestring);
                    ESP_LOGI(TAG2, "Stored N+C value %d: %s", server_index, N_server[server_index]);
                    server_index++;

                    // Allocate and store N+W
                    N_server[server_index] = (char *)malloc(len + 2); // +1 for 'W' and +1 for '\0'
                    snprintf(N_server[server_index], len + 2, "%sW", N_value->valuestring);
                    ESP_LOGI(TAG2, "Stored N+W value %d: %s", server_index, N_server[server_index]);
                    server_index++;
                }
            }
        }
        N_count = server_index; // Update N_count to reflect the actual number of items stored
        ESP_LOGI(TAG2, "Total N_server entries: %d", N_count);
    }
    else
    {
        ESP_LOGE(TAG2, "Failed to get media array from JSON response");
    }

    // Create the base path for metadata files
    char base_path[256];
    snprintf(base_path, sizeof(base_path), "/sdcard/media/toys/%s", tagUid_qbus);

    // Extract version and save to version.cubbies
    cJSON *version_json = cJSON_GetObjectItem(json_response, "version");
    if (version_json != NULL && cJSON_IsNumber(version_json))
    {
        char version_str[16];
        snprintf(version_str, sizeof(version_str), "%d", version_json->valueint);
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/version.cubbies", base_path);
        if (create_file(file_path, version_str) == ESP_OK)
        {
            ESP_LOGI(TAG2, "version.cubbies file created successfully with content: %s", version_str);
        }
    }
    else
    {
        ESP_LOGE(TAG2, "Failed to get version from JSON response");
    }

    // Extract productCode and save to productCode.cubbies
    cJSON *product_code_json = cJSON_GetObjectItem(json_response, "productCode");
    if (product_code_json != NULL && cJSON_IsString(product_code_json))
    {
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/productCode.cubbies", base_path);
        if (create_file(file_path, product_code_json->valuestring) == ESP_OK)
        {
            ESP_LOGI(TAG2, "productCode.cubbies file created successfully with content: %s", product_code_json->valuestring);
        }
    }
    else
    {
        ESP_LOGE(TAG2, "Failed to get productCode from JSON response");
    }

    // Extract recordable and save to recordable.cubbies
    cJSON *recordable_json = cJSON_GetObjectItem(json_response, "recordable");
    if (recordable_json != NULL && cJSON_IsNumber(recordable_json))
    {
        char recordable_str[16];
        snprintf(recordable_str, sizeof(recordable_str), "%d", recordable_json->valueint);
        char file_path[512];
        snprintf(file_path, sizeof(file_path), "%s/recordable.cubbies", base_path);
        if (create_file(file_path, recordable_str) == ESP_OK)
        {
            ESP_LOGI(TAG2, "recordable.cubbies file created successfully with content: %s", recordable_str);
        }
    }
    else
    {
        ESP_LOGE(TAG2, "Failed to get recordable from JSON response");
    }

    // Extract colourTheme and save to colourTheme.cubbies
    cJSON *colour_theme_json = cJSON_GetObjectItem(json_response, "colourTheme");
    if (colour_theme_json != NULL && cJSON_IsArray(colour_theme_json))
    {
        int size = cJSON_GetArraySize(colour_theme_json);
        int *colourThemeArray = (int *)malloc(size * sizeof(int));
        if (colourThemeArray == NULL)
        {
            ESP_LOGE(TAG2, "Failed to allocate memory for colourTheme array");
        }
        else
        {
            for (int i = 0; i < size; i++)
            {
                colourThemeArray[i] = cJSON_GetArrayItem(colour_theme_json, i)->valueint;
            }
            char file_path[512];
            snprintf(file_path, sizeof(file_path), "%s/colourTheme.cubbies", base_path);
            if (write_array_to_file(file_path, colourThemeArray, size) == ESP_OK)
            {
                ESP_LOGI(TAG2, "colourTheme.cubbies file created successfully with %d entries", size);
            }
            else
            {
                ESP_LOGE(TAG2, "Failed to write colourTheme.cubbies");
            }
            free(colourThemeArray);
        }
    }
    else
    {
        ESP_LOGE(TAG2, "Failed to get colourTheme from JSON response");
    }

    cJSON_Delete(json_response);
}

// ------------------------------------------------Downloaging Direction Files---------------------

static char output_buffer[4096]; // Increased buffer size
static int output_len = 0;

esp_err_t Downloader::DirectionCallback(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (output_len + evt->data_len < sizeof(output_buffer))
        {
            memcpy(output_buffer + output_len, evt->data, evt->data_len);
            output_len += evt->data_len;
            output_buffer[output_len] = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Buffer overflow");
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_len > 0)
        {
            ESP_LOGI(TAG, "Response Data: %s", output_buffer);
        }
        output_len = 0; // Reset buffer
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        output_len = 0; // Reset buffer
        break;
    }
    return ESP_OK;
}

void Downloader::save_file_to_sd(const char *direction_name, const char *data)
{
    if (direction_name == NULL || data == NULL)
    {
        ESP_LOGE(TAG, "File name or data is NULL");
        return;
    }

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "/sdcard/media/toys/%s/%s.cubbies", this->toy, direction_name);

    FILE *file = fopen(file_path, "w");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", file_path);
        return;
    }

    fprintf(file, "%s", data);
    fclose(file);

    ESP_LOGI(TAG, "File saved: %s", file_path);
}

void Downloader::download_file(const char *dir_file_name)
{
    if (dir_file_name == NULL)
    {
        ESP_LOGE(TAG, "Direction file name is NULL");
        return;
    }

    // Reset buffer before each request
    memset(output_buffer, 0, sizeof(output_buffer));
    output_len = 0;

    char direction_url[256];
    // direction_url - https://uat.littlecubbie.in/box/v1/download/file-download?fileName=Starter_kit_Box_with_Little_Cubbie_Intro_toy_south.json

    snprintf(direction_url, sizeof(direction_url), "%s%s", baseUrl, dir_file_name);
    // baseURL - https://uat.littlecubbie.in/box/v1/download/file-download?fileName=
    // dir_file_name - Starter_kit_Box_with_Little_Cubbie_Intro_toy_south.json

    esp_http_client_config_t config = {
        .url = direction_url,
        .event_handler = Downloader::DirectionCallback,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "x-cubbies-box-token", accessToken);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "GET request to %s succeeded. Status = %d, content_length = %" PRId64,
                 direction_url,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));

        // Extract the direction from the file name
        char direction[256];
        snprintf(direction, sizeof(direction), "%s", dir_file_name);

        // Find the last occurrence of '_' and extract the part after it
        char *last_underscore = strrchr(direction, '_');
        if (last_underscore != NULL)
        {
            // Extract the direction part
            char *direction_part = last_underscore + 1;
            char *extension_dot = strchr(direction_part, '.');
            if (extension_dot != NULL)
            {
                *extension_dot = '\0'; // Remove the file extension
            }

            // Save the response data to the SD card with direction as the file name
            save_file_to_sd(direction_part, output_buffer);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to parse direction from file name: %s", dir_file_name);
        }
    }
    else
    {
        ESP_LOGE(TAG, "GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    esp_http_client_close(client);
}

void Downloader::process_direction_files()
{
    int file_count = direction_file_count;
    ESP_LOGI(TAG, "Processing %d direction files", file_count);

    for (int i = 0; i < file_count; i++)
    {
        const char *dir_file_name = get_direction_file_name(i);
        if (dir_file_name != NULL)
        {
            ESP_LOGI(TAG, "Direction file: %s", dir_file_name);

            // Send GET request to download the file and save the response data
            download_file(dir_file_name);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to get direction file name for index %d", i);
        }
    }
}

// -------------------------------------Downloading Mp3 Files---------------------------

#define MAX_RETRY_COUNT 3
#define KEEP_ALIVE_ENABLE true
#define TIMEOUT_MS 5000

static FILE *file = NULL;
static int64_t total_download_time = 0;
static int64_t response_body_size = 0;

#define BUFFER_SIZE 4096
static uint8_t download_buffer[BUFFER_SIZE];
static size_t buffer_pos = 0;

void Downloader::flush_buffer()
{
    if (buffer_pos > 0 && file != NULL)
    {
        fwrite(download_buffer, 1, buffer_pos, file);
        buffer_pos = 0;
    }
}

esp_err_t Downloader::Downloadmp3Callback(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (file && evt->data)
        {
            size_t remaining = evt->data_len;
            const uint8_t *data = (const uint8_t *)evt->data;
            while (remaining > 0)
            {
                size_t to_copy = MIN(remaining, BUFFER_SIZE - buffer_pos);
                memcpy(download_buffer + buffer_pos, data, to_copy);
                buffer_pos += to_copy;
                data += to_copy;
                remaining -= to_copy;

                if (buffer_pos == BUFFER_SIZE)
                {
                    flush_buffer();
                }
            }
            response_body_size += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (file)
        {
            flush_buffer();
            fclose(file);
            file = NULL;
            ESP_LOGI(TAG, "File download complete");
            ESP_LOGI(TAG, "Size of response body: %lld bytes", response_body_size);
        }
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        if (file)
        {
            fclose(file);
            file = NULL;
            ESP_LOGE(TAG, "File download interrupted");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

cJSON *Downloader::find_metadata_for_file(const char *mp3_file_name)
{
    if (media_json == NULL)
        return NULL;

    int array_size = cJSON_GetArraySize(media_json);
    for (int i = 0; i < array_size; i++)
    {
        cJSON *item = cJSON_GetArrayItem(media_json, i);
        cJSON *n_value = cJSON_GetObjectItem(item, "N");
        if (n_value && cJSON_IsString(n_value) && strcmp(n_value->valuestring, mp3_file_name) == 0)
        {
            return item;
        }
    }
    return NULL;
}

void Downloader::write_metadata_to_file(FILE *file, cJSON *metadata)
{
    if (metadata == NULL || file == NULL)
        return;

    // Initialize default values
    int t_value = 0;
    int a_value = 0;
    int l_values[3] = {0, 0, 0};
    int r_values[3] = {0, 0, 0};
    int z_values[2] = {0, 0};

    // Extract T value
    cJSON *t = cJSON_GetObjectItem(metadata, "T");
    if (t && cJSON_IsNumber(t))
    {
        t_value = t->valueint;
    }

    // Extract A value if T is 1
    if (t_value == 1)
    {
        cJSON *a = cJSON_GetObjectItem(metadata, "A");
        if (a && cJSON_IsNumber(a))
        {
            a_value = a->valueint;
        }

        // Extract L values
        cJSON *l = cJSON_GetObjectItem(metadata, "L");
        if (l && cJSON_IsArray(l) && cJSON_GetArraySize(l) == 3)
        {
            for (int i = 0; i < 3; i++)
            {
                l_values[i] = cJSON_GetArrayItem(l, i)->valueint;
            }
        }

        // Extract R values
        cJSON *r = cJSON_GetObjectItem(metadata, "R");
        if (r && cJSON_IsArray(r) && cJSON_GetArraySize(r) == 3)
        {
            for (int i = 0; i < 3; i++)
            {
                r_values[i] = cJSON_GetArrayItem(r, i)->valueint;
            }
        }
    }

    // Extract Z values
    cJSON *z = cJSON_GetObjectItem(metadata, "Z");
    if (z && cJSON_IsArray(z) && cJSON_GetArraySize(z) == 2)
    {
        for (int i = 0; i < 2; i++)
        {
            z_values[i] = cJSON_GetArrayItem(z, i)->valueint;
        }
    }

    // Write metadata in fixed format
    fprintf(file, "HEADER_START\n");
    fprintf(file, "T:%03d\n", t_value);
    fprintf(file, "A:%03d\n", a_value);
    fprintf(file, "L:%03d,%03d,%03d\n", l_values[0], l_values[1], l_values[2]);
    fprintf(file, "R:%03d,%03d,%03d\n", r_values[0], r_values[1], r_values[2]);
    fprintf(file, "Z:%03d,%03d\n", z_values[0], z_values[1]);
    fprintf(file, "HEADER_END\n");
}

bool Downloader::download_mp3_file(const char *mp3_file_name)
{
    if (mp3_file_name == NULL)
    {
        ESP_LOGE(TAG, "MP3 file name is NULL");
        return false;
    }

    char mp3_url[260];
    snprintf(mp3_url, sizeof(mp3_url), "%s%s.mp3", baseUrl, mp3_file_name);

    char temp_file_path[256];
    snprintf(temp_file_path, sizeof(temp_file_path), "/sdcard/media/audio/%stemp.mp3", mp3_file_name);

    char final_file_path[256];
    snprintf(final_file_path, sizeof(final_file_path), "/sdcard/media/audio/%s.mp3", mp3_file_name);

    file = fopen(temp_file_path, "w");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", temp_file_path);
        return false;
    }

    cJSON *file_metadata = this->find_metadata_for_file(mp3_file_name);
    if (file_metadata)
    {
        this->write_metadata_to_file(file, file_metadata);
    }
    else
    {
        ESP_LOGW(TAG, "No metadata found for file: %s", mp3_file_name);
        fprintf(file, "0\n0,0\n");
    }

    response_body_size = 0;

    esp_http_client_config_t config = {
        .url = mp3_url,
        .timeout_ms = TIMEOUT_MS,                // Set timeout in milliseconds
        .event_handler = Downloader::Downloadmp3Callback,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .buffer_size = 4096,
        .buffer_size_tx = 1024,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = KEEP_ALIVE_ENABLE,   // Enable Keep-Alive
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "x-cubbies-box-token", accessToken);

    int64_t start_time = esp_timer_get_time();
    esp_err_t err;
    int retry_count = 0;

    // Implement retries on failure
    do {
        err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            break;  // Success, exit retry loop
        }
        ESP_LOGE(TAG, "GET request failed: %s. Retrying... (%d)", esp_err_to_name(err), retry_count);
        retry_count++;
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay before retry
    } while (retry_count < MAX_RETRY_COUNT);
    

    int64_t elapsed_time = esp_timer_get_time() - start_time;
    total_download_time += elapsed_time;

    // Handle the outcome of the request
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "GET request to %s succeeded. Status = %d, content_length = %" PRId64,
                 mp3_url,
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        ESP_LOGI(TAG, "Download time for %s: %lld ms", mp3_file_name, elapsed_time / 1000);

        // Rename the file after successful download
        if (rename(temp_file_path, final_file_path) != 0)
        {
            ESP_LOGE(TAG, "Failed to rename file: %s to %s", temp_file_path, final_file_path);
            esp_http_client_cleanup(client);
            return false;
        }

        esp_http_client_cleanup(client);
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "GET request failed after retries: %s", esp_err_to_name(err));
        fclose(file);
        // remove(temp_file_path);  // Clean up incomplete file
        esp_http_client_cleanup(client);
        return false;
    }

    esp_http_client_cleanup(client);
}

void Downloader::compare_and_update_N_server(const char *path)
{
    int file_count;
    char **file_names = sdcard->list_files(path, &file_count);

    if (file_names != NULL)
    {
        update_N_server(file_names, file_count);
        sdcard->free_file_list(file_names, file_count);
    }
}

bool Downloader::process_audio_files(const char *sd_card_path)
{
    int sd_file_count;
    char **sd_files = sdcard->list_files(sd_card_path, &sd_file_count);

    if (sd_files == NULL)
    {
        ESP_LOGE(TAG, "Failed to get list of files from SD card");
        return false;
    }

    ESP_LOGI(TAG, "Total files in N_server: %d", N_count);

    ESP_LOGI(TAG, "Duplicate files (already on SD card):");
    std::vector<std::string> files_to_download;

    for (int i = 0; i < N_count; i++)
    {
        const char *n_server_file = get_N_value(i);
        bool is_duplicate = false;

        for (int j = 0; j < sd_file_count; j++)
        {
            std::string n_server_name(n_server_file);
            std::string sd_name(sd_files[j]);

            auto n_server_ext = n_server_name.find_last_of('.');
            auto sd_ext = sd_name.find_last_of('.');

            if (n_server_ext != std::string::npos)
                n_server_name = n_server_name.substr(0, n_server_ext);
            if (sd_ext != std::string::npos)
                sd_name = sd_name.substr(0, sd_ext);

            if (n_server_name == sd_name)
            {
                is_duplicate = true;
                ESP_LOGI(TAG, "  %s", n_server_file);
                break;
            }
        }

        if (!is_duplicate)
        {
            files_to_download.push_back(n_server_file);
        }
    }

    ESP_LOGI(TAG, "Processing %d unique audio files", files_to_download.size());
    bool all_downloads_successful = true;

    for (const auto &file_name : files_to_download)
    {
        ESP_LOGI(TAG, "Downloading audio file: %s", file_name.c_str());
        bool success = download_mp3_file(file_name.c_str());
        if (!success)
        {
            all_downloads_successful = false;
            ESP_LOGE(TAG, "Failed to download: %s", file_name.c_str());
            // Break the loop if download fails after max retries
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay after every download
    }

    ESP_LOGI(TAG, "Total download time: %lld ms", total_download_time / 1000);

    if (all_downloads_successful && !files_to_download.empty())
    {
        create_verified_file();
    }

    sdcard->free_file_list(sd_files, sd_file_count);
    free_N_server();

    return all_downloads_successful;
}

void Downloader::create_verified_file()
{
    char file_path[256];
    sprintf(file_path, "/sdcard/media/toys/%s/verify.txt", tagUid_qbus);

    FILE *verified_file = fopen(file_path, "w");
    if (verified_file == NULL)
    {
        ESP_LOGE(TAG, "Failed to create verify.txt file");
        return;
    }

    fprintf(verified_file, "All files downloaded successfully at: %lld\n", esp_timer_get_time() / 1000000); // Current time in seconds
    fclose(verified_file);

    ESP_LOGI(TAG, "Created verify.txt file in %s", file_path);
}
