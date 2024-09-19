#ifndef __WIFI_H__
#define __WIFI_H__


#include "main.h"

#define         CREDENTIALS_SDCARD_LOCATION         "/sdcard/data/credentials.cubbies"
#define         WIFI_WAIT_TIME                      (20000/portTICK_PERIOD_MS)  


#define             WIFI_CONNECTED_BIT          BIT0
#define             WIFI_DISCONNECTED_BIT       BIT1
#define             WIFI_FAILED_BIT             BIT2

typedef struct {
    char ssid[32];
    char password[64];
}credentials_t;

class WiFi {
public:
    uint8_t init();
    uint8_t connect();
    bool disconnect();

    void writeCredentials(char*, char*);

    bool isConnected();

    bool bleWriteCredentials(char*, char*);
    uint8_t readCredentials();
    uint8_t ConfigSdCardCredentials();

private:
    int count = 0;
    bool connected = false;
    static bool connectFlag;
    credentials_t credentials;
    static EventGroupHandle_t eventGroup;
    
   
    uint8_t __connect();
    bool __disconnect();
    static void eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
};





#endif