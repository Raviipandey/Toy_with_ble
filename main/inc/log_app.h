#ifndef __LOG_APP_H__
#define __LOG_APP_H__

#include "main.h"

#define     LOG_FILE_SDCARD_LOCATION        "/sdcard/data/log.cubbies"
#define     HTTPS_POST_LOG_METRIX_URL       "https://littlecubbie.xyz/box/v1/upload/metrix"

#define     MAX_LOG_FILE_SIZE               3*1024

typedef enum {
    eLOG_POWER_ON = 0,
    eLOG_WIFI_CONNECTED,
    eLOG_NTP_CONNECTED,
    eLOG_SHUTDOWN,
    eLOG_LOW_BATTERY_CUTOFF,
    eLOG_TOY_DETECTED,
    eLOG_TOY_REMOVED,
    eLOG_PLAYING,
    eLOG_RESUME,
    eLOG_PAUSE,
    eLOG_EAR_RESPONSE,
    eLOG_NEXT,
    eLOG_PREVIOUS,
    eLOG_TURNTABLE_CHANGED,
    eLOG_USER_ADDED, 
    eLOG_TESTING
}log_events_et;

void LogTask(void *);
esp_err_t LogUploadCallback(esp_http_client_event_t *);

class Log {
public:
    void init();
    void logEvent(log_events_et);
    void logEvent(log_events_et, char *);
    void logEvent(log_events_et, char *, int);
    void logHeader();
    uint32_t getLogFileSize();
    void postLog();
    bool __post(char *);

private:

};


#endif