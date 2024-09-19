#include "system.h"
#include "player.h"
#include "log_app.h"
#include "common.h"

extern SdCard *sdcard;
extern Player *player;
extern System *sys;
extern Log *log;
extern LED *led;

bool isResponseRequiredFlag = false;
extern esp_periph_set_handle_t set;

char playerFilename[50];

// uint8_t header[16] = {0};
Header headerData;

uint8_t volume, maxVolume;
audio_pipeline_handle_t pipeline;
audio_element_handle_t reader, writer, decoder;
audio_board_handle_t handle;
audio_event_iface_handle_t evt;
uint32_t responseWaitTick = 0;
char location[100];
char audioIndex[4];

Player::Player()
{
}

Player::~Player()
{
    audio_pipeline_terminate(pipeline);
    audio_pipeline_unregister(pipeline, decoder);
    audio_pipeline_unregister(pipeline, writer);
    audio_pipeline_unregister(pipeline, reader);
    audio_pipeline_remove_listener(pipeline);
    audio_event_iface_destroy(evt);
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(writer);
    audio_element_deinit(decoder);
    audio_element_deinit(reader);
}

void Player::init()
{
    handle = audio_board_init();
    audio_hal_ctrl_codec(handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    audio_hal_set_volume(handle->audio_hal, 50 /*volume*/);

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);

    reader = create_fatfs_stream(AUDIO_STREAM_READER);
    writer = create_i2s_stream(AUDIO_STREAM_WRITER);
    decoder = create_mp3_decoder();

    audio_pipeline_register(pipeline, reader, "file");
    audio_pipeline_register(pipeline, decoder, "mp3");
    audio_pipeline_register(pipeline, writer, "i2s");

    const char *link_tag[3] = {"file", "mp3", "i2s"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);

    audio_pipeline_set_listener(pipeline, evt);
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);
}

// Add these implementations to your player.cpp file

audio_element_handle_t Player::create_fatfs_stream(audio_stream_type_t type)
{
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = type;
    return fatfs_stream_init(&fatfs_cfg);
}

audio_element_handle_t Player::create_i2s_stream(audio_stream_type_t type)
{
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = type;
    return i2s_stream_init(&i2s_cfg);
}

audio_element_handle_t Player::create_mp3_decoder()
{
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    return mp3_decoder_init(&mp3_cfg);
}

void Player::play(char *location)
{
    ESP_LOGW("player", "play");
    this->readVolume();

    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    ESP_LOGW("playerPlay", "terminateDone");
    vTaskDelay(50 / portTICK_PERIOD_MS);

    audio_element_set_uri(reader, location);
    audio_pipeline_reset_ringbuffer(pipeline);
    audio_pipeline_reset_elements(pipeline);

    // Initialize the audio stream
    audio_element_info_t info;
    audio_element_getinfo(reader, &info);
    audio_element_setinfo(writer, &info);
    i2s_stream_set_clk(writer, info.sample_rates, info.bits, info.channels);

    audio_pipeline_run(pipeline);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    fileFinishedPlaying = false;
}

void Player::readHeader()
{
    FILE *file = NULL;
    char location[100];
    char audioIndex[4];
    char line[256];

    // Construct the file path
    sprintf(audioIndex, "%d", (indexx + 1));
    sprintf(location, "/sdcard/media/audio/%s.mp3", cJSON_GetObjectItem(sys->json, audioIndex)->valuestring);

    file = fopen(location, "r");
    if (file == NULL)
    {
        printf("Error opening file\n");
        return;
    }

    // Read file line by line
    while (fgets(line, sizeof(line), file))
    {
        // Trim newline characters
        line[strcspn(line, "\r\n")] = 0;

        if (strncmp(line, "HEADER_START", 12) == 0)
        {
            // Start parsing the header
            continue;
        }
        else if (strncmp(line, "HEADER_END", 10) == 0)
        {
            // End of header
            break;
        }
        else if (line[0] == 'T' && line[1] == ':')
        {
            sscanf(line + 2, "%d", &headerData.T);
        }
        else if (line[0] == 'A' && line[1] == ':')
        {
            sscanf(line + 2, "%d", &headerData.A);
        }
        else if (line[0] == 'L' && line[1] == ':')
        {
            sscanf(line + 2, "%d,%d,%d", &headerData.L[0], &headerData.L[1], &headerData.L[2]);
        }
        else if (line[0] == 'R' && line[1] == ':')
        {
            sscanf(line + 2, "%d,%d,%d", &headerData.R[0], &headerData.R[1], &headerData.R[2]);
        }
        else if (line[0] == 'Z' && line[1] == ':')
        {
            strncpy(headerData.Z, line + 2, sizeof(headerData.Z) - 1);
            headerData.Z[sizeof(headerData.Z) - 1] = '\0'; // Null-terminate the string
        }
    }

    fclose(file);
}

// Function to retrieve specific header values
int Player::getHeaderValue(char field)
{
    switch (field)
    {
    case 'T':
        return headerData.T;
    case 'A':
        return headerData.A;
    default:
        printf("Unsupported field\n");
        return -1; // Error or unsupported field
    }
}

// Function to get RGB value array for L or R
int *Player::getHeaderRGBValue(char field)
{
    if (field == 'L')
    {
        return headerData.L;
    }
    else if (field == 'R')
    {
        return headerData.R;
    }
    else
    {
        printf("Unsupported field\n");
        return nullptr; // Error or unsupported field
    }
}

// Function to get the date range for Z
const char *Player::getHeaderDateRange()
{
    return headerData.Z;
}

void Player::readVolume()
{
    FILE *file = fopen(VOLUME_SDCARD_LOCATION, "r");
    uint8_t data[2] = {0};
    if (file != NULL)
    {
        sdcard->read(data, 2, file);
        volume = data[0];
        maxVolume = data[1];
        fclose(file);
    }
    else
    {
        volume = 100;
        maxVolume = 100;
        // writeVolume();
    }
    ESP_LOGW("Vol", "%d  %d", volume, maxVolume);
}

void Player::writeVolume()
{
    FILE *file = fopen(VOLUME_SDCARD_LOCATION, "w");
    uint8_t data[2] = {0};
    if (file != NULL)
    {
        data[0] = volume;
        data[1] = maxVolume;
        sdcard->write(data, 2, file);
    }
    fclose(file);
}

void Player::writeBrightness(int r, int g, int b, uint32_t duration)
{
    FILE *file = fopen(BRIGHTNESS_SDCARD_LOCATION, "w");

    if (file != NULL)
    {
        brightnessData[0] = r;
        brightnessData[1] = g;
        brightnessData[2] = b;
        brightnessData[3] = (duration) | 0xFF;
        brightnessData[4] = (duration >> 8) | 0xFF;
        brightnessData[5] = (duration >> 16) | 0xFF;
        brightnessData[6] = (duration >> 24) | 0xFF;

        sdcard->write(brightnessData, 7, file);
    }
    fclose(file);
}

void Player::readBrightess()
{
    FILE *file = fopen(BRIGHTNESS_SDCARD_LOCATION, "r");

    if (file != NULL)
    {
        sdcard->read(brightnessData, 7, file);
    }
    fclose(file);
}

void Player::playPauseToggle()
{
    audio_element_state_t state = audio_element_get_state(writer);
    if (state == AEL_STATE_RUNNING)
    {
        // log->logEvent(eLOG_PAUSE, playerFilename);
        audio_pipeline_pause(pipeline);
    }
    else if (state == AEL_STATE_PAUSED)
    {
        // log->logEvent(eLOG_RESUME, playerFilename);
        audio_pipeline_resume(pipeline);
    }
}

void Player::resumePlaying()
{
    audio_element_state_t state = audio_element_get_state(writer);
    if (state == AEL_STATE_PAUSED)
    {
        ESP_LOGI("RESUMEPLAYING", " audio_pipeline_resume(pipeline) IS CALLLEDDDDDDDDDDDDDD");
        // log->logEvent(eLOG_RESUME, playerFilename);
        audio_pipeline_resume(pipeline);

        sys->playerGoBackToState = SYS_PLAY_NEXT;
        sys->setState(SYS_WAIT_FOR_PLAYER); // always update playerGoBackToState before setting to this state
    }
    else
    {
        sys->setState(SYS_PLAY_NEXT);
    }
}

int Player::getVolume()
{
    return volume;
}

int Player::getMaxVolume()
{
    return maxVolume;
}

void Player::incVolume()
{
    volume += 5;
    if (volume > maxVolume)
    {
        volume = maxVolume;
    }
    ESP_LOGW("VOLUME", "%d", volume);
    audio_element_state_t el_state = audio_element_get_state(writer);
    if (el_state == AEL_STATE_RUNNING || el_state == AEL_STATE_PAUSED)
    {
        audio_hal_set_volume(handle->audio_hal, volume);
    }
    writeVolume();
}

void Player::decVolume()
{
    if (volume > 5)
    {
        volume -= 5;
    }
    else
    {
        volume = 0;
    }
    ESP_LOGW("VOLUME", "%d", volume);
    audio_element_state_t el_state = audio_element_get_state(writer);
    if (el_state == AEL_STATE_RUNNING || el_state == AEL_STATE_PAUSED)
    {
        audio_hal_set_volume(handle->audio_hal, volume);
    }
    writeVolume();
}

void Player::setVolume(int volume)
{
    if (volume < 0)
    {
        volume = 0;
    }
    else if (volume > 100)
    {
        volume = 100;
    }
    volume = volume;
    audio_element_state_t el_state = audio_element_get_state(writer);
    if (el_state == AEL_STATE_RUNNING || el_state == AEL_STATE_PAUSED)
    {
        audio_hal_set_volume(handle->audio_hal, volume);
    }
    writeVolume();
}

void Player::setMaxVolume(int volume)
{
    if (volume < 0)
    {
        maxVolume = 0;
    }
    else if (volume > 100)
    {
        maxVolume = 100;
    }
    maxVolume = volume;

    if (volume > maxVolume)
    {
        volume = maxVolume;
        // audio_element_state_t el_state = audio_element_get_state(writer);
        // if(el_state == AEL_STATE_RUNNING || el_state == AEL_STATE_PAUSED) {
        //     audio_hal_set_volume(handle->audio_hal, volume);
        // }
    }
    writeVolume();
}

void Player::deinit()
{
    led->setLeftEarColor(0, 0, 0);
    led->setRightEarColor(0, 0, 0);
    /*
        audio_pipeline_stop(pipeline);
        audio_pipeline_wait_for_stop(pipeline);
        audio_pipeline_terminate(pipeline);

        audio_pipeline_unregister(pipeline, reader);
        audio_pipeline_unregister(pipeline, writer);
        audio_pipeline_unregister(pipeline, decoder);

        audio_pipeline_remove_listener(pipeline);

        esp_periph_set_stop_all(set);
        audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

        audio_event_iface_destroy(evt);

        audio_pipeline_deinit(pipeline);
        audio_element_deinit(reader);
        audio_element_deinit(writer);
        audio_element_deinit(decoder);
        ESP_ERROR_CHECK(esp_periph_set_destroy(set));
    */
}

uint8_t Player::checkDownloadStatus(char *toy)
{
    char sdcardUrl[100];
    sprintf(sdcardUrl, "/sdcard/media/toys/%s/verify.txt", toy);

    ESP_LOGI("Player", "Toy to be verified: %s", toy);
    ESP_LOGI("Player", "Path for verify: %s", sdcardUrl);

    FILE *file = fopen(sdcardUrl, "r");
    if (file != NULL)
    {

        fclose(file);
        return 1;
    }
    return 0; // not verified
}

bool Player::CheckPlayingCompleted()
{

    audio_event_iface_msg_t msg;
    audio_event_iface_listen(evt, &msg, 10 / portTICK_PERIOD_MS);

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
        audio_element_info_t music_info = {0};
        audio_element_getinfo(decoder, &music_info);

        audio_element_setinfo(writer, &music_info);
        i2s_stream_set_clk(writer, music_info.sample_rates, music_info.bits, music_info.channels);
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)writer && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
    {
        if (isResponseRequiredFlag)
        {
            isResponseRequiredFlag = 0;
        }
        if ((int)msg.data == AEL_STATUS_STATE_FINISHED)
        {
            return true;
        }
    }

    return false;
}

void Player::SkipCurrentAudio()
{
    audio_element_state_t el_state = audio_element_get_state(writer);

    if ((el_state == AEL_STATE_RUNNING) || (el_state == AEL_STATE_PAUSED))
    {
        audio_pipeline_stop(pipeline);
        audio_pipeline_wait_for_stop(pipeline);

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void Player::PauseCurrentAudio()
{
    audio_element_state_t el_state = audio_element_get_state(writer);

    if (el_state == AEL_STATE_RUNNING)
    {
        audio_pipeline_pause(pipeline);
    }
}

void Player::skipQuestion()
{
    ESP_LOGI("SKIP QUESTION()", "SKIPPING QUESTION BECAUSE EAR PRESSED");
    if (isResponseRequiredFlag)
    {
        SkipCurrentAudio();
    }
}

void Player::ToyRemoved()
{
    PauseCurrentAudio();
    memset(sys->previousToy, 0, sizeof(sys->previousToy));
    strcpy(sys->previousToy, sys->toy);
    sys->previousTurntable = sys->getTurnTable();
    sys->setState(SYS_IDLE);
}

void Player::DirectionUpdated()
{
    SkipCurrentAudio();
}

void Player::next()
{
    if ((sys->state > SYS_PLAY_TOY) && (sys->state < SYS_PLAYING_COMPLETE))
    {
        sys->setState(SYS_PLAY_NEXT);
    }
}
void Player::previous()
{
    if ((sys->state > SYS_PLAY_TOY) && (sys->state < SYS_PLAYING_COMPLETE))
    {
        // Call setState only if we are not on the first song
        if (indexx > 0)
        {
            sys->setState(SYS_PLAY_PREVIOUS);
        }
        else
        {
            // Stay on the first song
            ESP_LOGI("PLAYER", "Already at the first song. Cannot go to a previous song.");
        }
    }
}

// In player.cpp
void Player::stopPipeline()
{
    audio_pipeline_stop(pipeline);
}

void Player::waitForPipelineStop()
{
    audio_pipeline_wait_for_stop(pipeline);
}