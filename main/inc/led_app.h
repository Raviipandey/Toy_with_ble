#ifndef __LED_APP_H__
#define __LED_APP_H__

#include "main.h"

/*
typedef enum
{
	LP_LED_OFF = 0,
	LP_LED_WIFI_CONNECTION,
	LP_LED_DOWNLOADING_AUDIO,
	LP_LED_PLAYING_AUDIO,
	LP_LED_LOG_TRANSFER,
	LP_LED_LAMP,
	LP_LED_FAULT
}landing_pad_led_states_et;
*/

typedef enum
{
	ESP_IDLE_MODE = 0,
	ESP_PLAYING_AUDIO,
	ESP_DOWNLOADING_AUDIO,
	ESP_DOWNLOADING_ERR,
	ESP_ERR,
	BAT_LOW,
	ESP_LP_LED_DEFINED_BY_RGB,
	SHUT_ME_DOWN_BY_ESP,
	ESP_BUSY_LAMP_MODE,
	POWER_OFF_IF_NO_LAMP,
	IMMEDIATE_POWER_OFF
} landing_pad_led_states_et;

class LED
{
public:
	void handler();
	void setLeftEarColor(int, int, int);
	void setRightEarColor(int, int, int);
	void setLandingPadColor(int, int, int);
	int getLandingPadColor(int);
	void setPadState(landing_pad_led_states_et);
	bool bleWriteLampData(int, uint32_t, int, int, int);
	int getRightEarColor(int);
	int getLeftEarColor(int);
	landing_pad_led_states_et getPadState();
	bool checkRGBValue(int);

private:
	landing_pad_led_states_et padState = ESP_IDLE_MODE;
	uint8_t leftEarColor[3], rightEarColor[3], padColor[3];
	uint32_t startTime, duration;
};

#endif