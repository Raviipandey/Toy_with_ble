#include "ble_app.h"
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEService.h>

extern WiFi *wifi;
extern BLE *ble;
extern System *sys;
extern SdCard *sdcard;
extern Initializer *init;
extern Player *player;

static const char *TAG = "BLE_APP";

NimBLECharacteristic *notifyCmdCharac = nullptr;

class BLECallback : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks
{
public:
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        ESP_LOGI(TAG, "Client connected");
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        ESP_LOGI(TAG, "Client disconnected, reason: %d", reason);
        pServer->startAdvertising();
    }

    void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        const NimBLEUUID &uuid = pCharacteristic->getUUID();
        char responseBuff[80] = {0};

        ESP_LOGI(TAG, "Reading characteristic with UUID: %s", uuid.toString().c_str());

        if (uuid == READ_MAC_ID_UUID)
        {
            sprintf(responseBuff, "{\"macID\":\"%s\"}", sys->getMacID());
        }
        else if (uuid == READ_BATTERY_UUID)
        {
            uint8_t batVoltage = sys->getBatteryLevel();
            uint8_t batPercent = (batVoltage > 36) ? ((batVoltage - 36) * 100) / (42 - 36) : 0;
            sprintf(responseBuff, "{\"battery\":%d}", batVoltage);
        }
        else if (uuid == READ_STORAGEINFO_UUID)
        {
            float gb = (sdcard->getFreeMemory() / 1024.0) / 1024.0;
            sprintf(responseBuff, "{\"freeBytes\":\"%f\"}", gb);
        }
        else if (uuid == READ_WIFISTAT_UUID)
        {
            sprintf(responseBuff, "{\"wifi\":\"%s\"}", wifi->isConnected() ? "connected" : "notConnected");
        }
        else if (uuid == READ_VOLUME_UUID)
        {
            sprintf(responseBuff, "{\"V\":%d, \"maxV\":%d}", player->getVolume(), player->getMaxVolume());
        }
        else if (uuid == READ_USER1)
        {
            sprintf(responseBuff, "{\"user1\":\"%s\"}", sys->getUserID(1));
        }
        else if (uuid == READ_USER2)
        {
            sprintf(responseBuff, "{\"user2\":\"%s\"}", sys->getUserID(2));
        }
        else if (uuid == READ_BOX_ID)
        {
            sys->ReadBoxUniqueID(); // void SystemTask(void *params) YE USMEI THA
            sprintf(responseBuff, "{\"boxid\":\"%s\"}", sys->getUniqueBoxID());
        }
        else if (uuid == READ_LAMPINFO_UUID)
        {
            // Hardcoded lamp data for testing
            sprintf(responseBuff, "{\"state\":1,\"duration\":300,\"color\":[255,128,0]}");
        }
        else
        {
            ESP_LOGW(TAG, "Unknown UUID in onRead: %s", uuid.toString().c_str());
            return;
        }

        pCharacteristic->setValue(responseBuff);
        ESP_LOGI(TAG, "Response set for characteristic UUID: %s, Value: %s", uuid.toString().c_str(), responseBuff);
    }

    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        uint8_t dataBuff[200];
        size_t length = pCharacteristic->getValue().length();

        // Check if the received data fits in our buffer
        if (length >= sizeof(dataBuff))
        {
            ESP_LOGE("BLE", "Received data too long: %d bytes", length);
            return;
        }

        strncpy((char *)dataBuff, pCharacteristic->getValue().c_str(), sizeof(dataBuff) - 1);
        dataBuff[sizeof(dataBuff) - 1] = '\0'; // Ensure null-termination

        ESP_LOGI("BLE", "Received data: %s", dataBuff);

        cJSON *json = cJSON_Parse((char *)dataBuff);
        if (!json)
        {
            ESP_LOGE("BLE", "Failed to parse JSON: %s", dataBuff);
            ble->notify("JsonParseFailed");
            return;
        }

        const NimBLEUUID &uuid = pCharacteristic->getUUID();
        ESP_LOGI(TAG, "Writing to characteristic with UUID: %s, Data: %s", uuid.toString().c_str(), dataBuff);

        if (uuid == WRITE_INITIALIZE_UUID)
        {
            ESP_LOGI(TAG, "Initializing with received packet");
            sys->initialize(
                cJSON_GetObjectItem(json, "ssid")->valuestring,
                cJSON_GetObjectItem(json, "password")->valuestring,
                cJSON_GetObjectItem(json, "newUser")->valuestring);
        }
        else if (uuid == WRITE_COMMAND_UUID)
        {
            ESP_LOGI("BLE", "Processing WRITE_COMMAND_UUID");

            cJSON *msgTypeJson = cJSON_GetObjectItem(json, "msgTyp");
            if (!msgTypeJson || !cJSON_IsNumber(msgTypeJson))
            {
                ESP_LOGE("BLE", "Invalid or missing msgTyp in JSON");
                cJSON_Delete(json);
                return;
            }
            int msgType = msgTypeJson->valueint;

            cJSON *userIDJson = cJSON_GetObjectItem(json, "userID");
            if (!userIDJson || !cJSON_IsString(userIDJson))
            {
                ESP_LOGE("BLE", "Invalid or missing userID in JSON");
                cJSON_Delete(json);
                return;
            }
            const char *userID = userIDJson->valuestring;

            if (!sys->verifyUser((uint8_t *)userID))
            {
                ESP_LOGW("BLE", "Invalid user ID: %s", userID);
                ble->notify("InvalidUserID");
                cJSON_Delete(json);
                return;
            }
            switch ((ble_message_types_et)msgType)
            {
            case eBLE_WRITE_ADD_USER:
                ESP_LOGI(TAG, "Adding new user: %s", cJSON_GetObjectItem(json, "newUser")->valuestring);
                sys->verifyUserID(
                    cJSON_GetObjectItem(json, "newUser")->valuestring,
                    cJSON_GetObjectItem(json, "updateUserNo")->valueint);
                break;

            case eBLE_WRITE_CREDENTIALS:
                ESP_LOGI(TAG, "Updating WiFi credentials");
                wifi->bleWriteCredentials(
                    cJSON_GetObjectItem(json, "ssid")->valuestring,
                    cJSON_GetObjectItem(json, "password")->valuestring);
                break;

            case eBLE_WRITE_MAX_VOLUME:
            {
                int volume = cJSON_GetObjectItem(json, "volume")->valueint;
                if (volume >= 0 && volume <= 100)
                {
                    ESP_LOGI(TAG, "Setting max volume: %d", volume);
                    // player->setMaxVolume(volume);
                    ble->notify("MaxVolumeSet");
                }
                else
                {
                    ESP_LOGW(TAG, "Invalid volume value: %d", volume);
                    ble->notify("MaxVolumeValueError");
                }
            }
            break;

            case eBLE_WRITE_LAMP_DATA:
            {
                int state = cJSON_GetObjectItem(json, "state")->valueint;
                int duration = cJSON_GetObjectItem(json, "duration")->valueint;
                int r = cJSON_GetArrayItem(cJSON_GetObjectItem(json, "color"), 0)->valueint;
                int g = cJSON_GetArrayItem(cJSON_GetObjectItem(json, "color"), 1)->valueint;
                int b = cJSON_GetArrayItem(cJSON_GetObjectItem(json, "color"), 2)->valueint;

                // if (led->bleWriteLampData(state, duration, r, g, b))
                // {
                //     ESP_LOGI(TAG, "Lamp data written successfully");
                //     player->writeBrightness(r, g, b, duration);
                //     ble->notify("LampDataSet");
                // }
                // else
                // {
                //     ESP_LOGW(TAG, "Failed to write lamp data");
                //     ble->notify("LampDataValueError");
                // }
            }
            break;

            case eBLE_WRITE_BOX_NAME:
                ESP_LOGI(TAG, "Setting box name: %s", cJSON_GetObjectItem(json, "boxName")->valuestring);
                sys->WriteBoxName(cJSON_GetObjectItem(json, "boxName")->valuestring);
                ble->notify("BoxNameSet");
                break;

            case eBLE_WRITE_FOTA_URL:
                ESP_LOGI(TAG, "Starting FOTA with URL: %s", cJSON_GetObjectItem(json, "url")->valuestring);
                // sys->startFota(cJSON_GetObjectItem(json, "url")->valuestring);
                break;

            default:
                ESP_LOGW(TAG, "Unknown message type: %d", msgType);
                break;
            }
        }

        cJSON_Delete(json);
    }
};

BLECallback bleCallback;

void BLE::init()
{
    // Initialize the BLE device
    NimBLEDevice::init("ESP32_Cubbies");

    // Create a BLE server
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&bleCallback); // Set server callbacks
    ESP_LOGI(TAG, "BLE Server created");

    // Create a BLE service
    NimBLEService *service = pServer->createService(CUBBIES_SVC_UUID);
    ESP_LOGI(TAG, "BLE Service created with UUID: %s", CUBBIES_SVC_UUID.toString().c_str());

    // Lambda function to create characteristics
    auto createCharacteristic = [&](NimBLEUUID uuid, uint32_t properties)
    {
        NimBLECharacteristic *pCharacteristic = service->createCharacteristic(uuid, properties);
        pCharacteristic->setValue("Initial Value");  // Set an initial value (optional)
        pCharacteristic->setCallbacks(&bleCallback); // Set characteristic callbacks
        ESP_LOGI(TAG, "Characteristic created with UUID: %s", uuid.toString().c_str());
        return pCharacteristic;
    };

    // Creating characteristics with the lambda function
    createCharacteristic(WRITE_COMMAND_UUID, NIMBLE_PROPERTY::WRITE);
    createCharacteristic(WRITE_INITIALIZE_UUID, NIMBLE_PROPERTY::WRITE);
    createCharacteristic(READ_MAC_ID_UUID, NIMBLE_PROPERTY::READ);
    createCharacteristic(READ_BATTERY_UUID, NIMBLE_PROPERTY::READ);
    createCharacteristic(READ_STORAGEINFO_UUID, NIMBLE_PROPERTY::READ);
    createCharacteristic(READ_WIFISTAT_UUID, NIMBLE_PROPERTY::READ);
    createCharacteristic(READ_VOLUME_UUID, NIMBLE_PROPERTY::READ);
    createCharacteristic(READ_LAMPINFO_UUID, NIMBLE_PROPERTY::READ);
    createCharacteristic(READ_USER1, NIMBLE_PROPERTY::READ);
    createCharacteristic(READ_USER2, NIMBLE_PROPERTY::READ);
    createCharacteristic(READ_BOX_ID, NIMBLE_PROPERTY::READ);

    notifyCmdCharac = createCharacteristic(NOTIFY_COMMAND_UUID, NIMBLE_PROPERTY::NOTIFY);

    // Start the service
    service->start();
    ESP_LOGI(TAG, "BLE Service started");

    // Start advertising
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(service->getUUID());
    pAdvertising->start();
    ESP_LOGI(TAG, "BLE Advertising started");
}

void BLE::deinit()
{
    ESP_LOGI(TAG, "Deinitializing BLE");

    // Stop advertising
    NimBLEDevice::getAdvertising()->stop();
    ESP_LOGI(TAG, "BLE Advertising stopped");

    // Get the server instance
    NimBLEServer* pServer = NimBLEDevice::getServer();
    if (pServer != nullptr) {
        // Remove our service
        NimBLEService* pService = pServer->getServiceByUUID(CUBBIES_SVC_UUID);
        if (pService != nullptr) {
            pServer->removeService(pService);
            ESP_LOGI(TAG, "BLE Service removed");
        }
    }

    // Deinitialize the BLE stack
    NimBLEDevice::deinit();
    ESP_LOGI(TAG, "BLE Stack deinitialized");

    // Reset pointers
    notifyCmdCharac = nullptr;

    ESP_LOGI(TAG, "BLE completely deinitialized");
}




void BLE::notify(const char *notification)
{
    if (notifyCmdCharac != nullptr)
    {
        notifyCmdCharac->setValue(notification);
        notifyCmdCharac->notify();
        ESP_LOGI("BLE", "Notification sent: %s", notification);
    }
    else
    {
        ESP_LOGW("BLE", "Notification characteristic is not initialized");
    }
}