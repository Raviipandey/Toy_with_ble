#include "sdcard.h"
#include <ff.h>
#include <dirent.h>
#include <sys/stat.h>
#include <esp_log.h>
#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "periph_sdcard.h"
#include "esp_log.h"

esp_periph_set_handle_t set;
static const char *TAG = "SD_CARD";
#define MAX_FILES 100

void SdCard::open() {
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    set = esp_periph_set_init(&periph_cfg);
    audio_board_sdcard_init(set, SD_MODE_1_LINE);
    setupFiles();
}

void SdCard::close() {

}

int SdCard::write(uint8_t* data, size_t len, FILE* file) {
    return fwrite(data, 1, len, file);
}

int SdCard::read(uint8_t* data, size_t len, FILE* file) {
    return fread(data, 1, len, file);
}

uint64_t SdCard::getFreeMemory() {
    FATFS *fs;
	DWORD freeCluster, freeSector;

	if (f_getfree("/sdcard/", &freeCluster, &fs)) {
		return 0;
	}
	freeSector = freeCluster * fs->csize;
	uint64_t freeBytes = (uint64_t)freeSector * FF_SS_SDCARD;

    return freeBytes;
}

bool SdCard::exist(char *toy) {
    char path[50];
    sprintf(path, "/sdcard/media/toys/%s", toy);
    DIR *directory = opendir(path);
    if(directory) 
    {
        closedir(directory);
        return true;
    }
    return false;
}


void SdCard::setupFiles() {
    mkdir("/sdcard/media", 0775);
    mkdir("/sdcard/data", 0775);
    mkdir("/sdcard/media/audio", 0775);
    mkdir("/sdcard/media/toys", 0775);
    vTaskDelay(100);

    //ESP_LOGW("\n removing folderrrr ", "%d", remove("/sdcard/massDelete2"));
    //deleteEntireFolder("/sdcard/massDelete");
    //deleteEntireFolder("/sdcard/massDelete2");
    //deleteEntireFolder("/sdcard/massDelete3");
    //eleteEntireFolder("/sdcard/massDelete4");
}


void deleteEntireFolder(char *folderName) 
{
    
    DIR *theFolder = opendir(folderName);
    struct dirent *next_file;
    char filepath[300];

    if(theFolder == NULL)
    {
        ESP_LOGW("\n folder ", "%s does not exist",folderName);
        return;
    }


    while ( (next_file = readdir(theFolder)) != NULL )
    {
        // build the path for each file in the folder
        sprintf(filepath, "%s/%s", folderName, next_file->d_name);
        if(next_file->d_type == 2)      //if its a  directory within the folder
        {
            ESP_LOGW("\n recursive call", "%s, %d", filepath, next_file->d_type);
            deleteEntireFolder(filepath); //recursive call 
        }
        else
        {
            ESP_LOGW("\n removing ", "%s, %d", filepath, next_file->d_type);
            remove(filepath);
        }
    }
    closedir(theFolder);
    ESP_LOGW("\n removing ", "folder %s, status %d", folderName, remove(folderName));

}

char** SdCard::list_files(const char *path, int *file_count)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
    {
        ESP_LOGE(TAG, "Failed to open directory: %s", path);
        *file_count = 0;
        return NULL;
    }

    char **file_names = (char **)malloc(MAX_FILES * sizeof(char *));
    if (file_names == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for file names");
        closedir(dir);
        *file_count = 0;
        return NULL;
    }

    struct dirent *entry;
    int count = 0;
    ESP_LOGI(TAG, "Listing files in directory: %s", path);
    while ((entry = readdir(dir)) != NULL && count < MAX_FILES)
    {
        if (entry->d_type == DT_REG) // If the entry is a regular file
        {
            file_names[count] = strdup(entry->d_name);
            if (file_names[count] == NULL)
            {
                ESP_LOGE(TAG, "Failed to allocate memory for file name");
                break;
            }
            count++;
        }
        else if (entry->d_type == DT_DIR) // If the entry is a directory
        {
            ESP_LOGI(TAG, "Directory: %s", entry->d_name);
        }
    }

    closedir(dir);
    *file_count = count;
    return file_names;
}

// Add this function to free the memory allocated for file names
void SdCard::free_file_list(char **file_names, int file_count)
{
    if (file_names != NULL)
    {
        for (int i = 0; i < file_count; i++)
        {
            free(file_names[i]);
        }
        free(file_names);
    }
}

