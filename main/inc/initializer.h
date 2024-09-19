#ifndef __INITIALIZER_H__

#define __INITIALIZER_H__

#include "main.h"

#define             HTTPS_VERIFY_USERID_URL             "https://uat.littlecubbie.in/box/v1/verify"

typedef enum {
    eINIT_IDLE = 0,
    eINIT_WIFI_CHECK,
    eINIT_USER_ID_CHECK
}init_states_et;

void InitializeTask(void *params);

class Initializer
{
public:
    ~Initializer();
    void initialize(char*, char*, char*);
    void verifyUserID(char*, uint8_t);

    void handler();
    bool verifyUserIdWithServer();

    static esp_err_t InitializerCallback(esp_http_client_event_t *evt);

private:
    TaskHandle_t xHandle;
    init_states_et initState;
    static uint8_t outputBuff[8*1024];
    char ssid[32];
    char password[64];
    char user1[50], user2[50];
};

#endif