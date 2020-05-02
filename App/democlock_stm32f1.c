/*******************************************************************************
Clock demo for the charlcd testing

author: Roberto Benjami

this demo is present
- LCD basic functions (LcdInit, LcdProcess, LcdRefreshAll, LcdIntProcess)
- Character set modify
- Blink function
- Cursor use
*******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "charlcd.h"

/* User timer number, interrupt priority */
#define  CLOCK_TIMER          1
#define  CLOCK_TIMX_IRQ_PR    255

//-----------------------------------------------------------------------------// freertos vs HAL
#ifdef  osCMSIS
#define Delay(t)              osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define Delay(t)              HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

unsigned int ev = 2014;
unsigned char honap = 12;
unsigned char nap = 24;
unsigned char ora = 23;
unsigned char perc = 58;
volatile unsigned char masodperc = 0;
unsigned char honaphosszak[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#if LCD_CHARSETCHANGE == 1
LCD_CHARSETARRAY       userchars;
const LCD_CHARSETARRAY usercharsrom = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F,
  0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F,
  0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
  0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
  0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
  0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
#endif

/* User timer for LCD */
#if     LCD_USERTIMER == 1 && ((LCD_MODE == 3) || (LCD_MODE == 5))
#if     CLOCK_TIMER == 1
#define CLOCK_TIMX              TIM1
#define CLOCK_TIMX_CLOCK        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN
#define CLOCK_TIMX_IRQn         TIM1_UP_IRQn
#define CLOCK_INTPROCESS        TIM1_UP_IRQHandler
#elif   CLOCK_TIMER == 2
#define CLOCK_TIMX              TIM2
#define CLOCK_TIMX_CLOCK        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN
#define CLOCK_TIMX_IRQn         TIM2_IRQn
#define CLOCK_INTPROCESS        TIM2_IRQHandler
#elif   CLOCK_TIMER == 3
#define CLOCK_TIMX              TIM3
#define CLOCK_TIMX_CLOCK        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN
#define CLOCK_TIMX_IRQn         TIM3_IRQn
#define CLOCK_INTPROCESS        TIM3_IRQHandler
#elif   CLOCK_TIMER == 4
#define CLOCK_TIMX              TIM4
#define CLOCK_TIMX_CLOCK        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN
#define CLOCK_TIMX_IRQn         TIM4_IRQn
#define CLOCK_INTPROCESS        TIM4_IRQHandler
#endif
#endif

//******************************************************************************
#ifdef osCMSIS
void StartDefaultTask(void const * argument)
#else
void mainApp(void)
#endif
{
  uint32_t lastmsec = 0;
  uint32_t t;

  #if LCD_CHARSETCHANGE == 1
  unsigned char i, j;
  #if LCD_CHARSETINIT == 1
  memcpy(userchars, &LcdDefaultCharset, 64);
  #else
  memcpy(userchars, &usercharsrom, 64);
  #endif
  #endif

  LcdInit();                            /* LCD init */
  //                      "01020304050607080910111213141516"
  memcpy((char *)LcdText, "   eeee.hh.nn       oo:pp:mm    ", 32); /* start screen */

  /* flashing the double dot between the numbers */
  #if LCD_BLINKCHAR == 1
  LcdBlinkChar(22);
  LcdBlinkChar(25);
  #endif

  /* timer init (only interrupt and usertimer mode) */
  #ifdef CLOCK_TIMX
  CLOCK_TIMX_CLOCK;

  uint32_t period;
  period = SystemCoreClock / (LCD_FPS * LCD_LINES * (LCD_WIDTH + 1));
  if(period >= 0x10000)
  { /* prescaler */
    CLOCK_TIMX->PSC = period >> 16;
    period = period / (CLOCK_TIMX->PSC + 1);
  }
  else
    CLOCK_TIMX->PSC = 0;            /* not is prescalse */
  CLOCK_TIMX->ARR = period;         /* autoreload */

  NVIC_SetPriority(CLOCK_TIMX_IRQn, CLOCK_TIMX_IRQ_PR);
  NVIC_EnableIRQ(CLOCK_TIMX_IRQn);

  CLOCK_TIMX->DIER = TIM_DIER_UIE;
  CLOCK_TIMX->CNT = 0;              /* counter = 0 */
  CLOCK_TIMX->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;
  #endif

  // user characters on the corners */
  #if LCD_CHARSETINIT == 1 || LCD_CHARSETCHANGE == 1
  LcdText[0] = 8;
  LcdText[1] = 10;
  LcdText[16] = 9;
  LcdText[17] = 11;
  LcdText[14] = 12;
  LcdText[15] = 14;
  LcdText[30] = 13;
  LcdText[31] = 15;
  #endif

  while(1)
  {
    t = GetTime();

    /* polling mode, continuous mode */
    #if LCD_MODE == 4
    LcdProcess();                       /* one character */
    #endif

    if(lastmsec + 1000 <= t)
    {
      /* if not is the automatic blink -> 1sec blink timer */
      #if (LCD_BLINKCHAR == 1) && (LCD_MODE != 5 || LCD_BLINKSPEED == 0)
      LcdBlinkPhase(2);
      #endif

      /* go clock */
      masodperc++;
      if(masodperc == 60)
      {
        masodperc = 0;
        if(++perc == 60)
        {
          perc = 0;
          if(++ora == 24)
          {
            ora = 0;
            if(++nap > honaphosszak[honap - 1])
            {
              nap = 1;
              if(++honap == 13)
              {
                honap = 1;
                ev++;
      } } } } }
      lastmsec += 1000;

      /* write the framebuffer */
      itoa(ev, (char *)LcdText + 3, 10);
      itoa(honap, (char *)LcdText + 8, 10);
      itoa(nap, (char *)LcdText + 11, 10);
      itoa(ora, (char *)LcdText + 20, 10);
      itoa(perc, (char *)LcdText + 23, 10);
      itoa(masodperc, (char *)LcdText + 26, 10);

      /* single-digit to double-digit */
      if(!LcdText[9]) {LcdText[9]  = LcdText[8];  LcdText[8]  = '0';}
      if(!LcdText[12]){LcdText[12] = LcdText[11]; LcdText[11] = '0';}
      if(!LcdText[21]){LcdText[21] = LcdText[20]; LcdText[20] = '0';}
      if(!LcdText[24]){LcdText[24] = LcdText[23]; LcdText[23] = '0';}
      if(!LcdText[27]){LcdText[27] = LcdText[26]; LcdText[26] = '0';}

      /* replacing punctuation overwritten by itoa */
      LcdText[7] = '.';
      LcdText[10] = '.';
      LcdText[22] = ':';
      LcdText[25] = ':';

      /* cursor step, cursor type change */
      #if LCD_CURSOR == 1 && ((LCD_MODE == 1) || (LCD_MODE == 2) || (LCD_MODE == 3))
      if(LcdGetCursorPos() < 1UL * LCD_WIDTH * LCD_LINES - 1)
        LcdSetCursorPos(LcdGetCursorPos() + 1);
      else
        LcdSetCursorPos(0);
      if     ((LcdGetCursorPos() & 3) == 0) LcdCursorOff();
      else if((LcdGetCursorPos() & 3) == 1) {LcdCursorUnBlink(); LcdCursorOn();}
      else if((LcdGetCursorPos() & 3) == 2) {LcdCursorBlink(); LcdCursorOn();}
      else if((LcdGetCursorPos() & 3) == 3) {LcdCursorUnBlink(); LcdCursorOn();}
      #endif

      /* user chars shifting */
      #if LCD_CHARSETCHANGE == 1
      j = userchars[63];
      i = 63;
      while((i--) > 0)
        userchars[i + 1] = userchars[i];
      userchars[0] = j;
      if(masodperc < 2)
        LcdChangeCharset((char *)&usercharsrom);
      else
        LcdChangeCharset((char *)&userchars);
      #endif /* LCD_CHARSETCHANGE */

      /* LCD frambuffer write to LCD */
      #if (LCD_MODE == 1) || (LCD_MODE == 2) || (LCD_MODE == 3)
      LcdRefreshAll();
      #endif
    }
  }
}

/* user timer interrupt */
#ifdef  CLOCK_TIMX
void CLOCK_INTPROCESS(void)
{
  CLOCK_TIMX->SR = 0;
  LcdIntProcess();
}
#endif
