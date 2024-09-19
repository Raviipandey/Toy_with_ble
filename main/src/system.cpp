#include "system.h"
#include "player.h"
#include "esp_log.h"
#include "common.h"
#include "wifi.h"
#include "downloader.h"
#include "ble_app.h"
#include "cJSON.h"

extern SdCard *sdcard;
extern Player *player;
extern System *sys;
extern LED *led;
extern WiFi *wifi;
extern Downloader *downloader;
extern BLE *ble;

extern uint32_t responseWaitTick;
uint8_t indexx;
uint8_t qnaAttempt;
uint8_t themeColor[3] = {0};
extern uint8_t tokenRequestFlag;
extern char accessToken[1500];
extern uint8_t validTokenFlag;

uint8_t System::outputBuff[];

// extern int A_value;
// int T_value = 0;
extern Header headerData;

void SystemTask(void *params)
{

    ESP_LOGW("SystemTask", "Task started");

    // UBaseType_t uxHighWaterMark;

    // sys->ReadBoxUniqueID();
    while (1)
    {
        sys->handler();

        vTaskDelay(SYSTEM_TASK_DELAY);
    }
}
System::System()
{
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    sprintf(this->macID, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    this->state = SYS_POST;
    this->powerState = 0;
}

void System::initialize(char *ssid, char *password, char *user1)
{
    memset(init_ssid, 0, sizeof(init_ssid));
    memset(init_password, 0, sizeof(init_password));
    memset(init_user1, 0, sizeof(init_user1));

    strcpy(init_ssid, ssid);
    strcpy(init_password, password);
    strcpy(init_user1, user1);

    setState(SYS_INIT_WIFI_CHECK);
}

void System::verifyUserID(char *userToCheck, uint8_t userNo)
{
    if (userNo == 1)
    {
        memset(init_user1, 0, sizeof(init_user1));
        strcpy(init_user1, userToCheck);
    }
    else if (userNo == 2)
    {
        memset(init_user2, 0, sizeof(init_user2));
        strcpy(init_user2, userToCheck);
    }
    setState(SYS_INIT_USER_ID_CHECK);
}

const char *System::SystemStateToString(system_states_et state)
{
    switch (state)
    {
    case SYS_POST:
        return "SYS_POST";
    case SYS_INIT:
        return "SYS_INIT";
    case SYS_IDLE:
        return "SYS_IDLE";
    case SYS_WAIT_FOR_PLAYER:
        return "SYS_WAIT_FOR_PLAYER";
    case SYS_PLAY_NEXT:
        return "SYS_PLAY_NEXT";
    case SYS_RESUME_PLAYING:
        return "SYS_RESUME_PLAYING";
    case SYS_TOY_DETECTED:
        return "SYS_TOY_DETECTED";
    case SYS_PLAY_TOY:
        return "SYS_PLAY_TOY";
    case SYS_GET_DIRECTION:
        return "SYS_GET_DIRECTION";
    case SYS_READ_AUDIO_HEADER:
        return "SYS_READ_AUDIO_HEADER";
    case SYS_PLAY_ACTIVITY:
        return "SYS_PLAY_ACTIVITY";
    case SYS_PLAY_QNA:
        return "SYS_PLAY_QNA";
    case SYS_WAIT_QNA_RESP:
        return "SYS_WAIT_QNA_RESP";
    case SYS_PLAY_NORMAL_AUDIO:
        return "SYS_PLAY_NORMAL_AUDIO";
    case SYS_DOWNLOAD_WIFI_CONN:
        return "SYS_DOWNLOAD_WIFI_CONN";
    case SYS_DOWNLOAD_TOY:
        return "SYS_DOWNLOAD_TOY";
    case SYS_WIFI_CONNECT_ERR:
        return "SYS_WIFI_CONNECT_ERR";
    case SYS_WIFI_CRED_MISSING_ERR:
        return "SYS_WIFI_CRED_MISSING_ERR";
    case SYS_SHUT_ME_DOWN:
        return "SYS_SHUT_ME_DOWN";
    case SYS_TURN_OFF:
        return "SYS_TURN_OFF";
    case SYS_ERROR:
        return "SYS_ERROR";
    default:
        return "UNKNOWN_STATE";
    }
}

void System::handler()
{
    // Get the current battery level
    int batteryLevel = sys->getBatteryLevel();
    // Get the current charging status
    int chargingStatus = this->getChargingStatus();

    // This log is causing some issues to led and QnA because they are printed continuously
    //  ESP_LOGW("TAG", "Current State: %s", SystemStateToString(this->state)); // Log current state as string

    switch (this->state)
    { // switch starts here

    case SYS_POST: // Power On Self Test
    {

        ESP_LOGW("TAG", "\nBattery Level: %d, \nCharging Status: %d", batteryLevel, chargingStatus); // Debug log

        // If battery level is 35% or higher, initialize the system
        if (batteryLevel >= 35)
        {
            setState(SYS_INIT);
        }

        // If battery level is between 28% and 34% inclusive
        else if (batteryLevel >= 28)
        {

            // If the device is charging, initialize the system and log a warning
            if (chargingStatus > 0)
            {
                setState(SYS_INIT);
                ESP_LOGW("\n battery low", "charging");
            }

            // If the device is not charging, log a warning PLAY BATTERYLOW TONE and shut down the system
            else
            {
                // Play battery low tone here
                ESP_LOGW("\n battery low", "shutting down");

                FILE *file = fopen(BAT_LOW_TONE_SDCARD_LOCATION, "r");
                if (file != NULL)
                {
                    ESP_LOGW("BATTERYLOW", "Playing tone");
                    fclose(file);

                    player->play(BAT_LOW_TONE_SDCARD_LOCATION);
                    playerGoBackToState = SYS_SHUT_ME_DOWN;
                    setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
                }
                else
                {
                    ESP_LOGW("BATTERYLOW", "Tone not found");
                    setState(SYS_SHUT_ME_DOWN);
                }
            }
        }

        // If battery level is below 28
        else
        {
            ESP_LOGW("\n critical battery", "shutting down immediately");
            // Play shutdown tone here
            setState(SYS_SHUT_ME_DOWN);
        }
    }
    break;

    case SYS_INIT:

    {
        led->setLeftEarColor(80, 80, 80);
        led->setRightEarColor(80, 80, 80);
        ESP_LOGW("Player", "init");
        // log->logEvent(eLOG_TESTING, "playerInit");
        player->init();
        ble->init();
        vTaskDelay(100);

        if (1) // realPowerONFlag //to avoid playing tones during accidental resets of ESP
        {
            FILE *file = fopen(STARTUP_TONE_SDCARD_LOCATION, "r");
            if (file != NULL)
            {
                ESP_LOGW("STARTUP", "Playing tone");
                fclose(file);

                player->play(STARTUP_TONE_SDCARD_LOCATION);
                playerGoBackToState = SYS_IDLE;
                // playerGoBackToState = SYS_POST; //This is only for checking
                setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
            }
            else
            {
                ESP_LOGW("STARTUP", "Tone not found");
                setState(SYS_IDLE);
                // setState(SYS_POST);
            }
        }
        else
        {
            setState(SYS_IDLE);
            // setState(SYS_POST);

            // setState(SYS_DOWNLOAD_TOY);
        }
    }
    break;

    case SYS_IDLE:

    {

        // First, check if the system has been idle for 5 minutes, as this is the longer timeout.
        if (TimeSpent(idleSinceTick, 300000))
        {
            setState(SYS_SHUT_ME_DOWN);
            break;
        }

        // Check if the battery level is between 28 % and 35 %

        if (batteryLevel >= 28 && batteryLevel < 35)
        {

            // If the device is charging, initialize the system and log a warning
            if (chargingStatus > 0)
            {
                setState(SYS_INIT);
                ESP_LOGW("\n battery low", "charging");
            }

            // If the device is not charging, log a warning PLAY BATTERYLOW TONE and shut down the system
            else
            {
                // Play battery low tone here
                ESP_LOGW("\n battery low", "shutting down");

                FILE *file = fopen(BAT_LOW_TONE_SDCARD_LOCATION, "r");
                if (file != NULL)
                {
                    ESP_LOGW("BATTERYLOW", "Playing tone");
                    fclose(file);

                    player->play(BAT_LOW_TONE_SDCARD_LOCATION);
                    playerGoBackToState = SYS_SHUT_ME_DOWN;
                    setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
                }
                else
                {
                    ESP_LOGW("BATTERYLOW", "Tone not found");
                    setState(SYS_SHUT_ME_DOWN);
                }
            }
        }

        // If battery level is below 28
        else if (batteryLevel < 28)
        {
            ESP_LOGW("\n critical battery", "shutting down immediately");
            // Play shutdown tone here
            setState(SYS_SHUT_ME_DOWN);
        }

        // if (sys->getBatteryLevel() <= 34)
        // {
        //     led->setPadState(BAT_LOW);
        //     ESP_LOGW("\n battery", "low %d", sys->getBatteryLevel());
        //     break;
        // }

        // Check for toy presence
        if (this->toyPresenseFlag) // else It will stay in SYS_IDLE
        {
            ESP_LOGW("System", "Tag detected");
            // log->logEvent(eLOG_TESTING, "tag detect");

            // Toy has been placed at least once
            this->toyPlacedOnce = true;

            // Play "Toy Detect" tone only if previous and current toy are different
            if (strcmp(this->toy, this->previousToy) != 0) // 0-same, 1-different
            {
                FILE *file = fopen(TOY_DETECT_TONE_SDCARD_LOCATION, "r");
                if (file != NULL)
                {
                    ESP_LOGW("toy detect", "Playing tone");
                    fclose(file);

                    player->play(TOY_DETECT_TONE_SDCARD_LOCATION);
                    playerGoBackToState = SYS_TOY_DETECTED;
                    setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
                }
                else
                {
                    ESP_LOGW("toy detect", "Tone not found");
                    setState(SYS_TOY_DETECTED);
                }
            }
            else
            {
                ESP_LOGW("System", "Same toy detected, not playing tone");
                setState(SYS_TOY_DETECTED);
            }
        }

        // If the toy is removed, start tracking the time only if the toy was placed at least once
        if (!this->toyPresenseFlag && this->toyPlacedOnce && toyRemovalTick == 0)
        {
            toyRemovalTick = GetStartTime();                      // Start the timer when toy is removed
            printf("Toy removed at tick: %lu\n", toyRemovalTick); // Log the time when the toy is removed
        }

        // Check if toy was removed and hasn't been replaced in 10 seconds
        if (!this->toyPresenseFlag && this->toyPlacedOnce && TimeSpent(toyRemovalTick, 10000))
        {
            // The toy was removed for more than 10 seconds
            ESP_LOGW("System", "Toy removed for more than 10 seconds, waiting for toy to be placed again to restart playback");
            // Stop the audio pipeline
            player->stopPipeline();
            player->waitForPipelineStop();
        }
    }

    break;

    case SYS_INIT_WIFI_CHECK:
        ESP_LOGW("TEST", "CHECKING CREDENTIALS");
        ESP_LOGW("Credentials", " %s  |  %s", init_ssid, init_password);
        if (wifi->bleWriteCredentials(init_ssid, init_password))
        {
            setState(SYS_INIT_USER_ID_CHECK);
        }
        else
        {
            setState(SYS_IDLE);
        }
        break;

    case SYS_INIT_USER_ID_CHECK:
        if (verifyUserIdWithServer())
        {
            setState(SYS_IDLE);
        }
        else
        {
            ESP_LOGW("BLE", "Verification failed!!");
            // Handle verification failure
            // setState(SYS_ERROR);
        }
        break;

    case SYS_WAIT_FOR_PLAYER:
    {
        // led->setLandingPadColor(255, 255, 255); // Only for check if player is in this state when logs are disabled

        if (playerGoBackToState == SYS_WAIT_QNA_RESP)
        {
            if (sys->getLeftEarButton() || sys->getRightEarButton())
            {
                player->playPauseToggle(); // Stop the current audio
                setState(SYS_WAIT_QNA_RESP);
                break;
            }
        }
        else if (playerGoBackToState == SYS_WAIT_ACTIVITY_RESP)
        {
            if (sys->getLeftEarButton() || sys->getRightEarButton())
            {
                player->playPauseToggle(); // Stop the current audio
                setState(SYS_PLAY_NEXT);
                break;
            }
        }

        if (player->CheckPlayingCompleted())
        {
            // led->setLandingPadColor(254, 13, 0); // ORANGE Only for check if player is in this state when logs are disabled

            responseWaitTick = GetStartTime();
            ESP_LOGW("\n returning", "to playerGoBackToState %d", playerGoBackToState);
            setState(playerGoBackToState);
        }
    }
    break;

    case SYS_TOY_DETECTED:
    {

        // check if toy is same as previously placed one
        if (strcmp(this->toy, this->previousToy) == 0) // 0-same, 1-different
        {
            ESP_LOGW("same", "toy id placed again");

            if (sdcard->exist(this->toy)) // folder exist? if yes- check for verify.txt, NO-SYS_DOWNLOAD_WIFI_CONN
            {
                if (player->checkDownloadStatus(this->toy)) // returns 1 on successful verification
                // if (1) // returns 1 on successful verification
                {
                    ESP_LOGW("download", "verified :%s", this->toy);
                    // --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
                    if (previousTurntable == sys->getTurnTable()) // verify.txt present so check turntabledirection, same-SYS_RESUME_TOY, different- Play ToyDetectTone then SYS_PLAY_TOY
                    {

                        // Check if toy was replaced within 10 seconds
                        if (TimeSpent(toyRemovalTick, 10000))
                        {
                            ESP_LOGW("System", "Toy replaced after 10 seconds, restarting playback from the beginning");
                            setState(SYS_PLAY_TOY);
                        }
                        else
                        {
                            ESP_LOGW("System", "Toy replaced within 10 seconds, resuming playback");
                            setState(SYS_RESUME_PLAYING);
                        }

                        // // log->logEvent(eLOG_TESTING, "same rfid placed again - resume playing");
                        // ESP_LOGW("same", "turntable, resume playing");
                        // setState(SYS_RESUME_PLAYING);
                        // break;
                    }
                    else // direction is different
                    {

                        setState(SYS_PLAY_TOY);
                    }
                }
                else
                {
                    ESP_LOGE("VERIFY", "Verify.txt not found");
                    setState(SYS_DOWNLOAD_WIFI_CONN); // since verify.txt not exist
                }
            }
            else
            {
                ESP_LOGE("SDCARD", "Folder not found");
                setState(SYS_DOWNLOAD_WIFI_CONN); // since folder not exist
            }
            // Reset toy removal tick
            toyRemovalTick = 0;
        }

        else // previous and current toy are different
        {
            ESP_LOGW("Different", "new toy id placed");

            if (sdcard->exist(this->toy)) //  folder exist? if yes- check for verify.txt, NO-SYS_DOWNLOAD_WIFI_CONN
            {
                if (player->checkDownloadStatus(this->toy)) // returns 1 on successful verification
                // if (1) // returns 1 on successful verification
                {
                    ESP_LOGW("download", "verified :%s", this->toy);

                    ESP_LOGW("System", "new toy, fresh start");

                    setState(SYS_PLAY_TOY);
                }
                else
                {
                    ESP_LOGE("VERIFY", "Verify.txt not found");
                    setState(SYS_DOWNLOAD_WIFI_CONN); // since verify.txt not exist
                }
            }
            else
            {
                ESP_LOGE("SDCARD", "Folder not found");
                setState(SYS_DOWNLOAD_WIFI_CONN); // since folder not exist
            }
        }
    }

    break;

    case SYS_PLAY_TOY:
    {
        ESP_LOGW("PLAYER", "PLAYING AUDIO FOR :%s", this->toy);
        // led->setPadState(ESP_PLAYING_AUDIO);
        // log->logEvent(eLOG_TESTING, "fresh play");
        setState(SYS_GET_DIRECTION);
    }

    case SYS_ERROR:

    {
    }

    break;

    case SYS_GET_DIRECTION:
    {
        // Get the current direction from the turntable
        turnTable = sys->getTurnTable();
        FILE *file = NULL;
        char location[100];
        char jsonBuff[1024] = {0}; // Initialize the buffer to avoid garbage data

        // Construct the file path based on the direction of the turntable
        const char *direction = nullptr;
        switch (turnTable)
        {
        case eNORTH:
            direction = "north";
            break;
        case eSOUTH:
            direction = "south";
            break;
        case eEAST:
            direction = "east";
            break;
        case eWEST:
            direction = "west";
            break;
        default:
            ESP_LOGW("Invalid direction", "Unknown turntable direction");
            // setState(SYS_DOWNLOAD_WIFI_CONN); // Set to an error state if direction is unknown
            setState(SYS_ERROR); // Set to an error state if direction is unknown
            return;
        }

        // Format the location path based on the direction and toy ID
        sprintf(location, "/sdcard/media/toys/%s/%s.cubbies", this->toy, direction);
        ESP_LOGW("Direction", "%s direction", direction);

        // Attempt to open the file corresponding to the direction
        file = fopen(location, "r");
        if (file != NULL)
        {
            // Use the sdcard->read method to read the content of the file into the buffer
            int bytesRead = sdcard->read((uint8_t *)jsonBuff, sizeof(jsonBuff) - 1, file);
            jsonBuff[bytesRead] = '\0'; // Null-terminate the buffer
            ESP_LOGW("jsonBuff", "%s", jsonBuff);

            fclose(file); // Close the file after reading

            // Parse the JSON content from the buffer
            json = cJSON_Parse(jsonBuff);
            indexx = 0; // Initialize indexx for further processing

            // Handle battery levels and device charging status
            if (batteryLevel >= 28 && batteryLevel < 35)
            {
                if (chargingStatus > 0)
                {
                    // Device is charging, continue processing
                    setState(SYS_READ_AUDIO_HEADER);
                    ESP_LOGW("Battery low", "Charging");
                }
                else
                {
                    // Device is not charging, play the battery low tone and shut down
                    ESP_LOGW("Battery low", "Shutting down");

                    file = fopen(BAT_LOW_TONE_SDCARD_LOCATION, "r");
                    if (file != NULL)
                    {
                        ESP_LOGW("BATTERYLOW", "Playing tone");
                        fclose(file);

                        player->play(BAT_LOW_TONE_SDCARD_LOCATION);
                        playerGoBackToState = SYS_SHUT_ME_DOWN;
                        setState(SYS_WAIT_FOR_PLAYER); // Update state after playing the tone
                    }
                    else
                    {
                        ESP_LOGW("BATTERYLOW", "Tone not found");
                        setState(SYS_SHUT_ME_DOWN);
                    }
                }
            }
            else if (batteryLevel < 28)
            {
                // Battery level is critically low, shut down immediately
                ESP_LOGW("Critical battery", "Shutting down immediately");
                setState(SYS_SHUT_ME_DOWN);
            }
            else
            {
                // Battery level is above 35%, continue processing
                setState(SYS_READ_AUDIO_HEADER);
            }
        }
        else
        {
            // File not found for the given direction, proceed to download it
            ESP_LOGW("File not found", "%s", location);
            setState(SYS_DOWNLOAD_WIFI_CONN);
        }

        char colorString[16];

        // Uncomment this section if you want to handle theme colors
        sprintf(location, "/sdcard/media/toys/%s/colourTheme.cubbies", this->toy);

        file = fopen(location, "r");
        if (file != NULL)
        {
            // Read the string from the file
            if (fgets(colorString, sizeof(colorString), file) != NULL)
            {
                // Parse the string to extract RGB values as uint8_t
                sscanf(colorString, "%hhu,%hhu,%hhu", &themeColor[0], &themeColor[1], &themeColor[2]);
                ESP_LOGW("theme colors", "%d,%d,%d", themeColor[0], themeColor[1], themeColor[2]);
                led->setLandingPadColor(themeColor[0], themeColor[1], themeColor[2]);
                led->setLeftEarColor(themeColor[0], themeColor[1], themeColor[2]);
                led->setRightEarColor(themeColor[0], themeColor[1], themeColor[2]);
            }
            fclose(file);
        }
        else
        {
            ESP_LOGW("theme colors", "read error");
        }
    }
    break;

    case SYS_READ_AUDIO_HEADER:

    {

        player->readHeader();

        ESP_LOGI("HEADER", "Header Values:");
        ESP_LOGI("HEADER", "T: %d", headerData.T);
        ESP_LOGI("HEADER", "A: %d", headerData.A);
        ESP_LOGI("HEADER", "L: %03d,%03d,%03d", headerData.L[0], headerData.L[1], headerData.L[2]);
        ESP_LOGI("HEADER", "R: %03d,%03d,%03d", headerData.R[0], headerData.R[1], headerData.R[2]);
        ESP_LOGI("HEADER", "Z: %s", headerData.Z);

        // log->logEvent(eLOG_TESTING, "reading audio header");

        // for (int i = 0; i < 16; i++)
        // {
        //     ESP_LOGW("HEADER", "%d -> %d", (i + 1), header[i]);
        // }

        /*
                uint32_t epoch = sysClock->getEpoch();
                uint32_t startEpoch = (header[8] << 24 | header[9] << 16 | header[10] << 8 | header[11]);
                uint32_t endEpoch = (header[12] << 24 | header[13] << 16 | header[14] << 8 | header[15]);
                if (startEpoch != 0 && endEpoch != 0)
                {
                    if (epoch <= startEpoch && epoch >= endEpoch)
                    {
                        ESP_LOGW("TESTING", "SKIPPING AUDIO");

                        setState(SYS_PLAY_NEXT); // this will increment index and next will again redirect to SYS_READ_AUDIO_HEADER state
                        break;                   //
                    }
                }
        */

        if (headerData.T == 2)
        {
            setState(SYS_PLAY_ACTIVITY);
        }
        else if (headerData.T == 1)
        {

            setState(SYS_PLAY_QNA);
        }
        else
        {
            setState(SYS_PLAY_NORMAL_AUDIO);
        }
    }

    break;

    case SYS_PLAY_NEXT:
    {
        player->SkipCurrentAudio();
        // ESP_LOGW("indexx", "%d", indexx);
        indexx++;
        // ESP_LOGW("indexx", "%d", indexx);
        // ESP_LOGW("json size", "%d", cJSON_GetArraySize(json));

        if (indexx >= cJSON_GetArraySize(json))
        {
            setState(SYS_PLAYING_COMPLETE);
        }
        else
        {

            // Handle battery levels and device charging status
            if (batteryLevel >= 28 && batteryLevel < 35)
            {
                if (chargingStatus > 0)
                {
                    // Device is charging, continue processing
                    setState(SYS_READ_AUDIO_HEADER);
                    ESP_LOGW("Battery low", "Charging");
                }
                else
                {
                    // Device is not charging, play the battery low tone and shut down
                    ESP_LOGW("Battery low", "Shutting down");

                    FILE *file = fopen(BAT_LOW_TONE_SDCARD_LOCATION, "r");
                    if (file != NULL)
                    {
                        ESP_LOGW("BATTERYLOW", "Playing tone");
                        fclose(file);

                        player->play(BAT_LOW_TONE_SDCARD_LOCATION);
                        playerGoBackToState = SYS_SHUT_ME_DOWN;
                        setState(SYS_WAIT_FOR_PLAYER); // Update state after playing the tone
                    }
                    else
                    {
                        ESP_LOGW("BATTERYLOW", "Tone not found");
                        setState(SYS_SHUT_ME_DOWN);
                    }
                    // log->logEvent(eLOG_NEXT); // Log the event after handling battery low condition
                }
            }
            else if (batteryLevel < 28)
            {
                // Battery level is critically low, shut down immediately
                ESP_LOGW("Critical battery", "Shutting down immediately");
                setState(SYS_SHUT_ME_DOWN);
                // log->logEvent(eLOG_NEXT); // Log the event for critical battery shutdown
            }
            else
            {
                // Battery level is above 35%, continue processing
                setState(SYS_READ_AUDIO_HEADER);
            }
        }
    }
    break;

    case SYS_PLAY_PREVIOUS:
    {
        player->SkipCurrentAudio();
        if (indexx > 0) // Only decrement indexx if it's greater than 0
        {
            indexx--;
        }
        // log->logEvent(eLOG_PREVIOUS);
        else
        {
            // Stay on the first song if indexx is 0
            ESP_LOGI("PLAYER", "Already at the first song. Cannot go to a previous song.");
        }
        setState(SYS_READ_AUDIO_HEADER);
    }
    break;

    case SYS_RESUME_PLAYING:
    {
        ESP_LOGW("PLAYER", "RESUMING AUDIO FOR :%s", this->toy);
        // led->setPadState(ESP_PLAYING_AUDIO);
        // log->logEvent(eLOG_TESTING, "resume play");
        player->resumePlaying();
    }
    break;

    case SYS_PLAY_NORMAL_AUDIO:
    {

        ESP_LOGW("SYS_PLAY_NORMAL_AUDIO", "PLAYING NORMAL TYPE AUDIO");

        // led->setLeftEarColor(255, 0, 255); //Just for test to see if ears are lighting up
        // led->setRightEarColor(255, 0, 255); //Just for test to see if ears are lighting up

        sprintf(audioIndex, "%d", (indexx + 1));
        sprintf(location, "/sdcard/media/audio/%s.mp3", cJSON_GetObjectItem(json, audioIndex)->valuestring);
        ESP_LOGW("File location", "%s", location);

        memset(playerFilename, 0, sizeof(playerFilename));
        sprintf(playerFilename, "%s", cJSON_GetObjectItem(json, audioIndex)->valuestring);
        // log->logEvent(eLOG_PLAYING, playerFilename);

        player->play(location);

        playerGoBackToState = SYS_PLAY_NEXT;
        setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state

        //     set_current_state(State::NORMAL);
        //     isActivityState = false;
        //     waitingForInput = false;
        //     qnaAttempt = 0;
    }

    break;

    case SYS_PLAY_QNA:
    {
        qnaAttempt = 1;
        ESP_LOGW("SYS_PLAY_QNA", "PLAYING QNA TYPE AUDIO");

        led->setLeftEarColor(headerData.L[0], headerData.L[1], headerData.L[2]);
        led->setRightEarColor(headerData.R[0], headerData.R[1], headerData.R[2]);
        ESP_LOGW("answer", "%d", headerData.A);

        isResponseRequiredFlag = true;
        sys->setLeftEarButton(false);
        sys->setRightEarButton(false);
        responseWaitTick = GetStartTime(); // this is to be updated in the wait for player state

        sprintf(audioIndex, "%d", (indexx + 1));
        sprintf(location, "/sdcard/media/audio/%s.mp3", cJSON_GetObjectItem(json, audioIndex)->valuestring);
        ESP_LOGW("File location", "%s", location);

        memset(playerFilename, 0, sizeof(playerFilename));
        sprintf(playerFilename, "%s", cJSON_GetObjectItem(json, audioIndex)->valuestring);
        // log->logEvent(eLOG_PLAYING, playerFilename);

        player->play(location);

        playerGoBackToState = SYS_WAIT_QNA_RESP;
        setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state

        // set_current_state(State::QNA);
        // currentAValue = a_value;
        // waitingForInput = true;
        // qnaAttempt = 1;
        // responseWaitTick = esp_timer_get_time() / 1000; // Convert to milliseconds
        // setLeftEarButton(false);
        // setRightEarButton(false);
    }

    break;

    case SYS_WAIT_QNA_RESP:
    {
        uint8_t resp = 5; // unrealistic value for no response received

        if (sys->getLeftEarButton())
        {
            resp = 0;
        }
        else if (sys->getRightEarButton())
        {
            resp = 1;
        }

        if (resp != 5) // response received
        {
            if (resp == headerData.A)
            {
                setState(SYS_CORRECT_ANS);
            }
            else
            {
                if (qnaAttempt == 1)
                {
                    setState(SYS_INCORRECT_ANS_TRY1);
                }
                else if (qnaAttempt == 2)
                {
                    setState(SYS_CORRECT_ANS); // tell the correct answer irrespective
                }
            }
        }
        else if (TimeSpent(responseWaitTick, 10000))
        {
            ESP_LOGI("PLAYER", "Response timeout");
            if (qnaAttempt == 1)
            {
                ESP_LOGI("PLAYER", "Timeout (attempt 1), moving to SYS_INCORRECT_ANS_TRY1");
                setState(SYS_INCORRECT_ANS_TRY1);
            }
            else if (qnaAttempt == 2)
            {
                ESP_LOGI("PLAYER", "Timeout (attempt 2), moving to SYS_CORRECT_ANS");
                setState(SYS_CORRECT_ANS); // tell the correct answer irrespective
            }
        }
    }
    break;

    case SYS_INCORRECT_ANS_TRY1:
    {
        ESP_LOGW("QnA", "wrong answer try 1");

        isResponseRequiredFlag = true;
        sys->setLeftEarButton(false);
        sys->setRightEarButton(false);
        responseWaitTick = GetStartTime();

        sprintf(location, "/sdcard/media/audio/%sW.mp3", cJSON_GetObjectItem(json, audioIndex)->valuestring);
        // log->logEvent(eLOG_EAR_RESPONSE, playerFilename, 0);

        player->play(location);

        qnaAttempt = 2;
        playerGoBackToState = SYS_WAIT_QNA_RESP;
        setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
    }
    break;

    case SYS_CORRECT_ANS:
    {

        ESP_LOGW("QnA", "correct answer received");
        sprintf(location, "/sdcard/media/audio/%sC.mp3", cJSON_GetObjectItem(json, audioIndex)->valuestring);
        // log->logEvent(eLOG_EAR_RESPONSE, playerFilename, 1);

        player->play(location);

        qnaAttempt = 0;
        playerGoBackToState = SYS_PLAY_NEXT;
        setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
    }
    break;

    case SYS_PLAY_ACTIVITY:
    {
        ESP_LOGW("SYS_PLAY_ACTIVITY", "PLAYING ACTIVITY TYPE AUDIO");

        led->setLeftEarColor(0, 255, 0);
        led->setRightEarColor(0, 255, 0);

        isResponseRequiredFlag = true;
        sys->setLeftEarButton(false);
        sys->setRightEarButton(false);
        responseWaitTick = GetStartTime();

        sprintf(audioIndex, "%d", (indexx + 1));
        sprintf(location, "/sdcard/media/audio/%s.mp3", cJSON_GetObjectItem(json, audioIndex)->valuestring);
        ESP_LOGW("File location", "%s", location);

        memset(playerFilename, 0, sizeof(playerFilename));
        sprintf(playerFilename, "%s", cJSON_GetObjectItem(json, audioIndex)->valuestring);
        // log->logEvent(eLOG_PLAYING, playerFilename);

        player->play(location);

        playerGoBackToState = SYS_WAIT_ACTIVITY_RESP;
        setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
    }
    break;

    case SYS_WAIT_ACTIVITY_RESP:
    {
        if (TimeSpent(responseWaitTick, 10000) || sys->getLeftEarButton() || sys->getLeftEarButton())
        {
            setState(SYS_PLAY_NEXT);
        }
    }
    break;

    case SYS_PLAYING_COMPLETE:
    {

        // LOOP BACK TO FIRST SONG AFTER ALL THE SONGS ARE DONE BEING PLAYED
        setState(SYS_PLAY_TOY);
    }
    break;

    case SYS_DOWNLOAD_WIFI_CONN:
    {

        if (wifi->init() == 0)
        {
            if (wifi->ConfigSdCardCredentials() == 0) // returns error(1) if creds are missing
            {
                if (wifi->connect() == 0)
                {
                    FILE *file = fopen(DOWNLOAD_START_TONE_SDCARD_LOCATION, "r");
                    if (file != NULL)
                    {
                        ESP_LOGW("DOWNLOADING CONTENT", "PLAY DOWNLOADING,PLEASE GIVE US SOME TIME Tone");
                        fclose(file);

                        player->play(DOWNLOAD_START_TONE_SDCARD_LOCATION);
                        playerGoBackToState = SYS_DOWNLOAD_TOY;
                        setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
                    }
                    else
                    {
                        ESP_LOGW("DOWNLOADING CONTENT", "DOWNLOADING,PLEASE GIVE US SOME TIME Tone NOT FOUND");
                        setState(SYS_DOWNLOAD_TOY);
                    }
                }
                else
                {
                    setState(SYS_WIFI_CONNECT_ERR);
                }
            }
            else
            {
                setState(SYS_WIFI_CRED_MISSING_ERR);
            }
        }
    }

    break;

    case SYS_DOWNLOAD_TOY:

    {
        // led->setPadState(ESP_DOWNLOADING_AUDIO);
        led->setLandingPadColor(254, 13, 0); // ORANGE, Only for check if system is in Download state

        ESP_LOGW("toyDownload", "fresh downloading :%s", this->toy);
        Downloader *downloader = new Downloader();
        sys->ReadBoxUniqueID();
        bool download_success = downloader->begin(this->toy);

        delete downloader;

        if (download_success)
        {
            ESP_LOGI("toyDownload", "All downloads completed successfully");
        }
        else
        {
            ESP_LOGE("toyDownload", "Some downloads failed or max retries reached");
        }

        // led->setPadState(ESP_IDLE_MODE);
        setState(SYS_IDLE);
    }

    break;

    case SYS_WIFI_CONNECT_ERR:
    {

        FILE *file = fopen(INVALID_WIFI_CREDS_TONE_SDCARD_LOCATION, "r");
        if (file != NULL)
        {
            ESP_LOGW("INVALID_WIFI", "PLAY Please CHECK WiFi Credentials Tone");
            fclose(file);

            player->play(INVALID_WIFI_CREDS_TONE_SDCARD_LOCATION);
            playerGoBackToState = SYS_IDLE;
            setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
        }
        else
        {
            ESP_LOGW("INVALID_WIFI", "Please CHECK WiFi Credentials Tone not found");
            setState(SYS_IDLE);
        }
    }

    break;

    case SYS_WIFI_CRED_MISSING_ERR:
    {

        FILE *file = fopen(PLEASE_ENTER_WIFI_CREDS_TONE_SDCARD_LOCATION, "r");
        if (file != NULL)
        {
            ESP_LOGW("ENTER_WIFI", "PLAY Please Enter WiFi Credentials Tone");
            fclose(file);

            player->play(PLEASE_ENTER_WIFI_CREDS_TONE_SDCARD_LOCATION);
            playerGoBackToState = SYS_IDLE;
            setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
        }
        else
        {
            ESP_LOGW("ENTER_WIFI", "Please Enter WiFi Credentials Tone not found");
            setState(SYS_IDLE);
        }
    }
    break;

    case SYS_SHUT_ME_DOWN:
    {
        // Set all LEDs to red to indicate Shutting down
        led->setLandingPadColor(255, 0, 0);
        led->setLeftEarColor(255, 0, 0);
        led->setRightEarColor(255, 0, 0);

        FILE *file = fopen(SHUTDOWN_TONE_SDCARD_LOCATION, "r");
        if (file != NULL)
        {
            ESP_LOGW("SHUTDOWN", "Playing tone");
            fclose(file);

            player->play(SHUTDOWN_TONE_SDCARD_LOCATION);
            playerGoBackToState = SYS_TURN_OFF;
            setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
        }
        else
        {
            ESP_LOGW("SHUTDOWN", "Tone not found");
            setState(SYS_TURN_OFF);
        }
    }

    break;

    case SYS_TURN_OFF:
    {
        sys->setPowerStatus(true);
    }

    break;
    } // switch ends here
}

void System::setState(system_states_et state)
{
    ESP_LOGW("\nsetstate", "%d\n", state);
    // log->logEvent(eLOG_TESTING, "setstate", state);

    /*
        if (this->state == state || this->state == SYS_FOTA)
        {
            return;
        }
    */

    // else if ((state == SYS_IDLE) || (state == SYS_PLAYING_COMPLETE))

    if ((state == SYS_IDLE))
    {
        idleSinceTick = GetStartTime();
    }
    this->state = state;
}

void System::setPowerStatus(uint8_t state)
{
    powerState = state;
}

uint8_t System::getPowerStatus()
{
    return powerState;
}

void System::setChargingStatus(uint8_t state)
{
    chargingState = state;
}

uint8_t System::getChargingStatus()
{
    return chargingState;
}

void System::setBatteryLevel(uint8_t level)
{
    batteryLevel = level;
}

uint8_t System::getBatteryLevel()
{
    return batteryLevel;
}

turn_table_direction_et System::getTurnTable()
{
    return turnTable;
}

void System::setTurnTable(turn_table_direction_et direction)
{
    this->turnTable = direction;
}

bool System::ReadBoxUniqueID()
{
    // read box id *******************************************************************
    FILE *file1 = fopen(BOX_ID_SDCARD_LOCATION, "r");
    if (file1 != NULL)
    {
        memset(uniqueBoxID, 0, sizeof(uniqueBoxID));
        sdcard->read((uint8_t *)&uniqueBoxID, 36, file1);
        char printBuff[40] = {0};
        strncpy(printBuff, (const char *)&uniqueBoxID, 36);
        ESP_LOGW("BOX", "BOXID: %s", printBuff);
        fclose(file1);
        return true;
    }
    else
    {
        ESP_LOGW("BOX", "BOX ID not found");
        memset(uniqueBoxID, 0, sizeof(uniqueBoxID));
        return false;
    }
}

char *System::getMacID()
{
    return this->macID;
}

char *System::getUniqueBoxID()
{
    return this->uniqueBoxID;
}

void System::setToy(char *toy)
{
    memset(this->toy, 0, sizeof(this->toy));
    strcpy(this->toy, toy);
    toyPresenseFlag = true;
    ESP_LOGW("toy", "SetToy:%s", this->toy);
    // player->NewToyPlaced();   //call in system statemachine instead
}

void System::resetToy()
{
    toyPresenseFlag = false;
    // log->logEvent(eLOG_TOY_REMOVED, this->toy);

    player->ToyRemoved();
    // log->logEvent(eLOG_TESTING, "toyRemoved");

    memset(this->toy, 0, sizeof(this->toy));
    ESP_LOGW("tag", "removed");
    // deinit player
    // player->init();
    // player->deinit();
    this->setState(SYS_IDLE);
}

// Add these new functions
void System::setRightEarButton(bool state)
{
    rightEarButtonState = state;
}

void System::setLeftEarButton(bool state)
{
    leftEarButtonState = state;
}

bool System::getRightEarButton()
{
    return rightEarButtonState;
}

bool System::getLeftEarButton()
{
    return leftEarButtonState;
}

char *System::getUserID(int userIndex)
{
    if (userIndex == 1)
    {
        return sys_user1;
    }
    else if (userIndex == 2)
    {
        return sys_user2;
    }
    return NULL;
}

void System::writeUserID(int userIndex, char *user)
{
    if (userIndex == 1)
    {
        memset(sys_user1, 0, 40);
        strncpy(sys_user1, user, 36);

        FILE *file = fopen(USER_ID1_SDCARD_LOCATION, "w+"); // creates file if it doesn't exist
        if (file != NULL)
        {
            sdcard->write((uint8_t *)&sys_user1, 36, file);
            char printBuff[40] = {0};
            strncpy(printBuff, (const char *)&sys_user1, 36);
            ESP_LOGW("WRITE TO SDCARD", "USER1: %s", printBuff);
            fclose(file);
        }
    }
    else if (userIndex == 2)
    {
        memset(sys_user2, 0, 40);
        strncpy(sys_user2, user, 36);

        FILE *file = fopen(USER_ID2_SDCARD_LOCATION, "w+"); // creates file if it doesn't exist
        if (file != NULL)
        {
            sdcard->write((uint8_t *)&sys_user2, 36, file);
            char printBuff[40] = {0};
            strncpy(printBuff, (const char *)&sys_user2, 36);
            ESP_LOGW("WRITE TO SDCARD", "USER2: %s", printBuff);
            fclose(file);
        }
    }
}

bool System::verifyUser(uint8_t *user)
{
    if (strcmp(sys_user1, (char *)user) == 0 || strcmp(sys_user2, (char *)user) == 0)
    {
        return true;
    }
    return false;
}

void System::readUserIDs()
{
    // vTaskDelay(500);

    // read user id 1 *******************************************************************
    FILE *file1 = fopen(USER_ID1_SDCARD_LOCATION, "r");
    if (file1 != NULL)
    {
        sdcard->read((uint8_t *)&sys_user1, 36, file1);
        char printBuff[40] = {0};
        strncpy(printBuff, (const char *)&sys_user1, 36);
        ESP_LOGW("USER", "USER1: %s", printBuff);
        fclose(file1);
    }
    else
    {
        ESP_LOGW("USER", "U1 not found");
        memset(sys_user1, 0, 40);
    }

    // read user id 2 *******************************************************************
    FILE *file2 = fopen(USER_ID2_SDCARD_LOCATION, "r");
    if (file2 != NULL)
    {
        sdcard->read((uint8_t *)&sys_user2, 36, file2);
        char printBuff[40] = {0};
        strncpy(printBuff, (const char *)&sys_user2, 36);
        ESP_LOGW("USER", "USER2: %s", printBuff);
        fclose(file2);
    }
    else
    {
        ESP_LOGW("USER", "U2 not found");
        memset(sys_user2, 0, 40);
    }
}

bool System::isBoxInitialized()
{
    if (!strlen(sys_user1) && !strlen(sys_user2))
    {
        return false;
    }
    return true;
}

bool System::ReadBoxName()
{
    // read box id *******************************************************************
    FILE *file1 = fopen(BOX_NAME_SDCARD_LOCATION, "r");
    if (file1 != NULL)
    {
        sdcard->read((uint8_t *)&boxName, sizeof(boxName), file1);
        ESP_LOGW("BOX", "BoxName: %s", boxName);
        fclose(file1);
        return true;
    }
    else
    {
        ESP_LOGW("BOX", "BoxName not found");
        memset(boxName, 0, sizeof(boxName));
        return false;
    }
}

char *System::GetBoxName()
{
    return this->boxName;
}

void System::WriteBoxName(char *name)
{
    memset(this->boxName, 0, sizeof(this->boxName));

    strcpy(this->boxName, name);

    FILE *file = fopen(BOX_NAME_SDCARD_LOCATION, "w");
    if (file != NULL)
    {
        sdcard->write((uint8_t *)&this->boxName, sizeof(boxName), file);
        ESP_LOGW("BOX", "Box Name write: %s", this->boxName);
        fclose(file);
    }
}

bool System::verifyUserIdWithServer()
{
    if (!validTokenFlag)
    {
        Downloader *downloader = new Downloader();
        downloader->downloadToken();
    }

    if (wifi->connect() == 0)
    {
        esp_http_client_config_t config = {
            .url = HTTPS_VERIFY_USERID_URL,
            .auth_type = HTTP_AUTH_TYPE_BASIC,
            .event_handler = System::InitializerCallback,
            .buffer_size = DOWNLOAD_CHUNK_SIZE,
            .buffer_size_tx = 1500, // newly added line
            .crt_bundle_attach = esp_crt_bundle_attach};

        esp_http_client_handle_t client = esp_http_client_init(&config);

        char postData[500];
        ESP_LOGW("Debug", "init_user1: '%s'", init_user1);
        ESP_LOGW("Debug", "init_user2: '%s'", init_user2);

        sprintf(postData, "{\"macAddress\": \"%s\",\"user1Id\": \"%s\",\"user2Id\": \"%s\"}", sys->getMacID(), init_user1, init_user2);
        ESP_LOGW("sfbs", "\n\nPosting data:\n%s\n\n", postData);

        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_header(client, "x-cubbies-box-token", accessToken);
        esp_http_client_set_post_field(client, postData, strlen(postData));

        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK)
        {
            wifi->disconnect();
            return false;
        }
        esp_http_client_cleanup(client);

        cJSON *json = cJSON_Parse((char *)outputBuff);
        if (json == NULL)
        {
            ESP_LOGW("Testing", "INVALID JSON");
            return false;
        }

        if (strlen(init_user1))
        {
            ESP_LOGW("Test", "USER1");
            cJSON *userJson = cJSON_GetObjectItemCaseSensitive(json, "user1");
            if (cJSON_IsTrue(userJson))
            {
                ESP_LOGW("Test", "USER1 VERIFIED");
                sys->writeUserID(1, init_user1);
                ble->notify("9");
                // ble->notify("VerificationSuccess");
            }
            else
            {
                ESP_LOGW("Test", "USER1 FAILED");
                ble->notify("10");
                // ble->notify("VerificationFailed");
            }
        }
        else if (strlen(init_user2))
        {
            ESP_LOGW("Test", "USER2");
            cJSON *userJson = cJSON_GetObjectItemCaseSensitive(json, "user2");
            if (cJSON_IsTrue(userJson))
            {
                ESP_LOGW("Test", "USER2 VERIFIED");
                sys->writeUserID(2, init_user2);
                ble->notify("9");
                // ble->notify("VerificationSuccess");
            }
            else
            {
                ESP_LOGW("Test", "USER2 FAILED");
                ble->notify("10");
                // ble->notify("VerificationFailed");
            }
        }
    }
    wifi->disconnect();
    return true;
}

esp_err_t System::InitializerCallback(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
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
