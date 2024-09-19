#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "main.h"
#include "cJSON.h"
#include "common.h"

void SystemTask(void *);

#define USER_ID1_SDCARD_LOCATION "/sdcard/data/user1.cubbies"
#define USER_ID2_SDCARD_LOCATION "/sdcard/data/user2.cubbies"
#define API_KEY_SDCARD_LOCATION "/sdcard/data/apikey.cubbies"
#define BOX_ID_SDCARD_LOCATION "/sdcard/data/boxid.cubbies"
#define BOX_NAME_SDCARD_LOCATION "/sdcard/data/boxname.cubbies"

#define VOLUME_SDCARD_LOCATION "/sdcard/data/volume.cubbies"
#define BRIGHTNESS_SDCARD_LOCATION "/sdcard/data/brightness.cubbies"
#define STARTUP_TONE_SDCARD_LOCATION "/sdcard/media/startup.mp3"
#define TOY_DETECT_TONE_SDCARD_LOCATION "/sdcard/media/toydetect.mp3"
#define DOWNLOAD_START_TONE_SDCARD_LOCATION "/sdcard/media/downloading.mp3"
#define SHUTDOWN_TONE_SDCARD_LOCATION "/sdcard/media/shutdown.mp3"
#define BAT_LOW_TONE_SDCARD_LOCATION "/sdcard/media/batterylow.mp3"
#define INVALID_WIFI_CREDS_TONE_SDCARD_LOCATION "/sdcard/media/unabletoconnect.mp3"
#define PLEASE_ENTER_WIFI_CREDS_TONE_SDCARD_LOCATION "/sdcard/media/plsenterwifi.mp3"

typedef enum
{
    SYS_POST,
    SYS_INIT,
    SYS_IDLE,
    SYS_INIT_WIFI_CHECK,
    SYS_INIT_USER_ID_CHECK,
    SYS_TOY_DETECTED,
    SYS_PLAY_TOY,
    SYS_WAIT_FOR_PLAYER,
    SYS_PLAYING_COMPLETE,
    SYS_PLAY_NEXT,
    SYS_PLAY_PREVIOUS,
    SYS_RESUME_PLAYING,
    SYS_GET_DIRECTION,
    SYS_ERROR,
    SYS_READ_AUDIO_HEADER,
    SYS_PLAY_ACTIVITY,
    SYS_WAIT_ACTIVITY_RESP,
    SYS_PLAY_QNA,
    SYS_WAIT_QNA_RESP,
    SYS_INCORRECT_ANS_TRY1,
    SYS_CORRECT_ANS,
    SYS_PLAY_NORMAL_AUDIO,
    SYS_DOWNLOAD_WIFI_CONN,
    SYS_DOWNLOAD_TOY,
    SYS_WIFI_CONNECT_ERR,
    SYS_WIFI_CRED_MISSING_ERR,
    SYS_SHUT_ME_DOWN,
    SYS_TURN_OFF
} system_states_et;

typedef enum
{
    eNORTH = 0,
    eEAST,
    eWEST,
    eSOUTH,
    eBLANK
} turn_table_direction_et;

typedef enum
{
    eNORMAL_AUDIO = 0,
    eQNA_AUDIO,
    eACTIVITY_AUDIO
} audio_types_et;

class System
{
public:
    System();
    const char *SystemStateToString(system_states_et state);

    bool ReadBoxUniqueID();
    bool ReadBoxName();
    char *GetBoxName();
    void WriteBoxName(char *);

    void setToy(char *);
    void resetToy();

    void handler();

    char *getMacID();
    char *getUniqueBoxID();

    void readUserIDs();

    bool verifyUser(uint8_t *);

    void setChargingStatus(uint8_t state);
    uint8_t getChargingStatus();

    void setPowerStatus(uint8_t state);
    uint8_t getPowerStatus();

    void setBatteryLevel(uint8_t level);
    uint8_t getBatteryLevel();

    turn_table_direction_et getTurnTable();
    void setTurnTable(turn_table_direction_et);

    char *getUserID(int);
    bool isBoxInitialized();

    void startFota(char *);

    bool getLeftEarButton();
    bool getRightEarButton();
    void setLeftEarButton(bool state);
    void setRightEarButton(bool state);

    void writeUserID(int, char *);
    void setState(system_states_et);
    cJSON *json;

    system_states_et state = SYS_POST;
    system_states_et playerGoBackToState;

    char toy[20];
    char previousToy[20];

    
    void initialize(char* ssid, char* password, char* user1);
    void verifyUserID(char* userToCheck, uint8_t userNo);
    bool verifyUserIdWithServer();
    static esp_err_t InitializerCallback(esp_http_client_event_t *evt);

    turn_table_direction_et turnTable = eNORTH;
    turn_table_direction_et previousTurntable = eBLANK;

private:
    char macID[20];
    char uniqueBoxID[40] = {0};
    char boxName[100];
    char sys_user1[40] = {0};
    char sys_user2[40] = {0};
    uint32_t idleSinceTick = 0;
    uint32_t toyRemovalTick = 0;
    char init_ssid[32];
    char init_password[64];
    char init_user1[50] = {0};
    char init_user2[50] = {0};
    static uint8_t outputBuff[1024];

    bool toyPresenseFlag = false;
    bool toyPlacedOnce = false; // Toy has been placed at least once (false - NO, true - YES)

    char fotaUrl[150];

    uint8_t chargingState;
    uint8_t powerState;
    uint8_t batteryLevel;

    bool leftEarButtonState, rightEarButtonState;
};

#endif