#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "main.h"
#include "board.h" // Add this line to include the board-specific definitions
#include "esp_timer.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"

struct Header
{
    int T;      // Single integer for T
    int A;      // Single integer for A
    int L[3];   // RGB values for L
    int R[3];   // RGB values for R
    char Z[20]; // Date range in string format
};

extern char audioIndex[4];
extern char location[100];
extern char playerFilename[50];
extern bool isResponseRequiredFlag;

class Player
{
public:
    void begin(char *);
    Player();
    ~Player();

    void init();
    void play(char *location);
    void readVolume();
    void incVolume();
    void decVolume();
    int getVolume();
    int getMaxVolume();
    void setVolume(int);
    void setMaxVolume(int);

    uint8_t checkDownloadStatus(char *toy);
    void writeBrightness(int r, int g, int b, uint32_t duration);
    void readBrightess();
    uint8_t brightnessData[7];

    void deinit();
    void next();
    void previous();

    void NewToyPlaced(char *toy);
    void ToyRemoved();
    void DirectionUpdated();

    void readHeader();
    int getHeaderValue(char field);
    int *getHeaderRGBValue(char field);
    const char *getHeaderDateRange();

    void SkipCurrentAudio();
    void PauseCurrentAudio();

    void skipQuestion();

    bool CheckPlayingCompleted();
    void writeVolume();
    void playPauseToggle();
    void resumePlaying();

    // Add these new methods
    void stopPipeline();
    void waitForPipelineStop();

    // int A_value = 0;
    // int T_value = 0;

private:
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t reader, writer, decoder;
    audio_board_handle_t handle;
    audio_event_iface_handle_t evt;

    uint8_t volume;
    uint8_t maxVolume;
    int current_file_index;
    bool waitingForInput;
    uint32_t responseWaitTick;
    bool rightEarButtonState;
    bool leftEarButtonState;
    bool fileFinishedPlaying;
    bool isActivityState;
    uint64_t activityStartTime;

    // Helper functions
    audio_element_handle_t create_fatfs_stream(audio_stream_type_t type);
    audio_element_handle_t create_i2s_stream(audio_stream_type_t type);
    audio_element_handle_t create_mp3_decoder();
};

#endif // __PLAYER_H__