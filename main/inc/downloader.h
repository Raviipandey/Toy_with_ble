#ifndef __DOWNLOADER_H__
#define __DOWNLOADER_H__

#include "main.h"

#define HTTPS_BOX_LOGIN_URL "https://uat.littlecubbie.in/auth/v1/box/login"
#define HTTPS_GET_USER_TOY_MEDIA_URL "https://uat.littlecubbie.in/box/v1/download/masterJson"
#define DOWNLOAD_CHUNK_SIZE 2 * 1024

// esp_err_t DownloaderCallback(esp_http_client_event_t *evt);

class Downloader
{
public:
    Downloader();
    bool begin(char *toy);
    static FILE *file;

    static esp_err_t TokenCallback(esp_http_client_event_t *evt);
    static esp_err_t create_directory(const char *path);
    static esp_err_t MasterjsonCallback(esp_http_client_event_t *evt, void *context);
    static esp_err_t DirectionCallback(esp_http_client_event_t *evt);
    void download_master_json();
    void free_N_server();
    void update_N_server(char **sd_files, int sd_file_count);
    void downloadToken();
    const char *get_direction_file_name(int index);
    const char *get_N_value(int index);
    static esp_err_t create_file(const char *path, const char *content);
    static esp_err_t write_array_to_file(const char *file_path, const int *array, int size);
    void parse_and_store_metadata(const char *response_buffer, size_t response_buffer_size);
    void save_file_to_sd(const char *direction_name, const char *data);
    void download_file(const char *dir_file_name);
    void process_direction_files();
    static void flush_buffer();
    static esp_err_t Downloadmp3Callback(esp_http_client_event_t *evt);
    cJSON *find_metadata_for_file(const char *mp3_file_name);
    void write_metadata_to_file(FILE *file, cJSON *metadata);
    bool download_mp3_file(const char *mp3_file_name);
    void compare_and_update_N_server(const char *path);
    bool process_audio_files(const char *sd_card_path);

    void create_verified_file();

    bool was_download_successful;

private:
    esp_http_client_handle_t client;
    cJSON *json;
    char toy[20] = {0};
    char baseURL[50];
    uint8_t downloadAudio(int);
    void createFolders();
    uint8_t downloadMetadata();

    uint8_t parseMetadata();
    uint8_t download(char *url, char *fileName, char *tempFileName, uint8_t *header, uint8_t headerSize, uint8_t renameReqdFlag);
};

#endif