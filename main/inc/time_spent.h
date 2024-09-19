#ifndef __QTIMESPENT_H
#define __QTIMESPENT_H

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "main.h"


void millisTimerInit(void);
uint32_t GetStartTime(void);
uint8_t TimeSpent(uint32_t startTick, uint32_t totalTimeInMs);



#endif
