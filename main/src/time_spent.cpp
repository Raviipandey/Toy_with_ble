/*
      @file    qtimespent.c
      @author  Gajanan & Gopal
      @version V
      @date
      @brief
*/
#include "time_spent.h"

#define FULL_SCALE_VALUE		0xffffffff

/*
    The millis()is incremented in interrupt every 1 milliseconds.
    It's initialisation is done in the main file in SystemClock_Config().
*/

void millisTimerInit(void)
{
	// return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

uint32_t GetStartTime(void)
{
//  return millis();
	 return ((unsigned long) (esp_timer_get_time() / 1000ULL));
}

uint8_t TimeSpent(uint32_t startTick, uint32_t totalTimeInMs)
{
  if (GetStartTime() >= startTick)
  {
    if ((GetStartTime() - startTick) < totalTimeInMs)
    {
      return 0;
    }
    else
    {
      return 1;
    }
    /*
                  return ((counterTick - startTick) < totalTimeInMs)?0:1;
    */
  }
  else
  {
    if ((FULL_SCALE_VALUE - (startTick - GetStartTime()) < totalTimeInMs))
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }
}
