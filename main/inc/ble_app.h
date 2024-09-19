#ifndef _BLE_APP_H_
#define _BLE_APP_H_

#include "main.h"

#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include "esp_log.h"

// #define DEVICE_NAME "Cubbies_temp"
// #define CUBBIES_SVC_UUID NimBLEUUID((uint16_t)0xAF85)
// #define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define     CUBBIES_SVC_UUID        NimBLEUUID((uint16_t)0xAF85)

#define     TESTING_UUID            NimBLEUUID((uint16_t)0xAB12) //

#define     WRITE_COMMAND_UUID      NimBLEUUID((uint16_t)0xE683)
#define     WRITE_INITIALIZE_UUID   NimBLEUUID((uint16_t)0xEAF9) //
#define     READ_MAC_ID_UUID        NimBLEUUID((uint16_t)0x9DBD) //
#define     READ_BATTERY_UUID       NimBLEUUID((uint16_t)0x506D)
#define     READ_STORAGEINFO_UUID   NimBLEUUID((uint16_t)0x506E)
#define     READ_WIFISTAT_UUID      NimBLEUUID((uint16_t)0x506F)
#define     READ_VOLUME_UUID        NimBLEUUID((uint16_t)0x5070)
#define     READ_LAMPINFO_UUID      NimBLEUUID((uint16_t)0x5071)
#define     READ_USER1              NimBLEUUID((uint16_t)0x5072)
#define     READ_USER2              NimBLEUUID((uint16_t)0x5073)
#define     READ_BOX_ID             NimBLEUUID((uint16_t)0x5074)

#define     NOTIFY_COMMAND_UUID     NimBLEUUID((uint16_t)0x841A) //

typedef enum
{
    eBLE_WRITE_CREDENTIALS_AND_USERID = 0,
    eBLE_WRITE_CREDENTIALS,
    eBLE_WRITE_MAX_VOLUME,
    eBLE_WRITE_LAMP_DATA,
    eBLE_WRITE_BOX_NAME,
    eBLE_WRITE_FOTA_URL,
    eBLE_WRITE_ADD_USER
} ble_message_types_et;

typedef enum
{
    eNOTIFY_WIFI_INIT = 0,
    eNOTIFY_USER_ID_API_KEY,
    eNOTIFY_INVALID_USER,
    eNOTIFY_CREDENTIALS,
    eNOTIFY_FOTA_UPDATE
} ble_notify_messages_t;

class BLE
{
public:
    void init();
    void notify(const char *notification);
    void deinit(); 
    // static void serverCallbacks(NimBLEServer *pServer);
};

#endif