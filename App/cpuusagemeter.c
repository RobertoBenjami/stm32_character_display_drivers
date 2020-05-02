/******************************************************************************
Cpu usage meter for the charlcd driver testing
  (only LCD_MODE == 5 and LCD_USERTIMER == 0)

author: Roberto Benjami

This program is show the processor usage for LCD
  the 3 number in the display:
  - cycle with LCD refresh on
  - cycle with LCD refresh off
  - what percentage of performance decreased when refresh on

Note: the display is created for 2x16 or 4x40 characters
*******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "charlcd.h"

/* Measurement time base */
#define TIMEBASE              200

//-----------------------------------------------------------------------------// freertos vs HAL
#ifdef  osCMSIS
#define Delay(t)              osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define Delay(t)              HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

//******************************************************************************
#ifdef osCMSIS
void StartDefaultTask(void const * argument)
#else
void mainApp(void)
#endif
{
  static uint32_t cycle;
  uint32_t lastmsec = 0, t, ca = 0, cycle1 = 0, cycle2 = 0, c;

  LcdInit();                            /* LCD init */
  #if (1UL * LCD_WIDTH * LCD_LINES) < 80
  /* 2x16 characters ------------------ */
  #if LCD_BLINKCHAR == 1
  for(c = 24; c < 32; c++)
  LcdBlinkChar(c);
  #endif /* LCD_BLINKCHAR */
  #else  /* (1UL * LCD_WIDTH * LCD_LINES) < 80 */
  /* 4x40 characters ------------------ */
  memcpy((char *)LcdText +   0, "****************************************", 40);
  memcpy((char *)LcdText +  40, "*          LCD CPU used meter          *", 40);
  memcpy((char *)LcdText +  80, "*                                      *", 40);
  memcpy((char *)LcdText + 120, "****************************************", 40);
  #if LCD_BLINKCHAR == 1
  for(c = 41; c < 78; c++)
    LcdBlinkChar(c);
  #endif /* LCD_BLINKCHAR */
  #endif /* else (1UL * LCD_WIDTH * LCD_LINES) < 80 */

  while(1)
  {
    t = GetTime();

    if(lastmsec + TIMEBASE <= t)
    {

      if(!ca)
      {
        LcdRefreshStop();
        cycle1 = cycle;
        cycle = 0;
      }
      else
      {
        LcdRefreshStart();
        cycle2 = cycle;
        cycle = 0;
      }
      ca = 1 - ca;

      if(cycle1 && cycle2)
      {
        /* Show the measurement result */
        #if (1UL * LCD_WIDTH * LCD_LINES) < 80
        /* 2x16 characters ------------------ */
        memcpy((char *)LcdText, "                                ", 32);
        utoa(cycle1,  (char *)LcdText +  0, 10);
        utoa(cycle2,  (char *)LcdText +  8, 10);
        utoa(100 - (201 * cycle1 / (2 * cycle2)), (char *)LcdText + 16, 10);
        #else  /* (1UL * LCD_WIDTH * LCD_LINES) < 80 */
        /* 4x40 characters ------------------ */
        /*                            "0204060810121416182022242628303234363840" */
        memcpy((char *)LcdText +  80, "*With:        Without:              %  *", 40);
        utoa(cycle1,  (char *)LcdText +  86, 10);
        utoa(cycle2,  (char *)LcdText + 102, 10);
        utoa(100 - (201 * cycle1 / (2 * cycle2)), (char *)LcdText + 113, 10);
        #endif /* else (1UL * LCD_WIDTH * LCD_LINES) < 80 */
      }

      lastmsec = GetTime();
    }
    cycle++;
  }
}
