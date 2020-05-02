/*------------------------------------------------------------------------------
   author:  Roberto Benjami
   version: 2020.04
------------------------------------------------------------------------------*/

#include "main.h"
#include "charlcd.h"

//=============================================================================

#define BITBAND_ACCESS(a, b)  *(volatile uint32_t*)(((uint32_t)&a & 0xF0000000) + 0x2000000 + (((uint32_t)&a & 0x000FFFFF) << 5) + (b << 2))

//-----------------------------------------------------------------------------
/* GPIO mode */
#define MODE_ANALOG_INPUT     0x0
#define MODE_PP_OUT_10MHZ     0x1
#define MODE_PP_OUT_2MHZ      0x2
#define MODE_PP_OUT_50MHZ     0x3
#define MODE_FF_DIGITAL_INPUT 0x4
#define MODE_OD_OUT_10MHZ     0x5
#define MODE_OD_OUT_2MHZ      0x6
#define MODE_OD_OUT_50MHZ     0x7
#define MODE_PU_DIGITAL_INPUT 0x8
#define MODE_PP_ALTER_10MHZ   0x9
#define MODE_PP_ALTER_2MHZ    0xA
#define MODE_PP_ALTER_50MHZ   0xB
#define MODE_RESERVED         0xC
#define MODE_OD_ALTER_10MHZ   0xD
#define MODE_OD_ALTER_2MHZ    0xE
#define MODE_OD_ALTER_50MHZ   0xF

#define GPIOX_PORT_(a, b)     GPIO ## a
#define GPIOX_PORT(a)         GPIOX_PORT_(a)

#define GPIOX_PIN_(a, b)      b
#define GPIOX_PIN(a)          GPIOX_PIN_(a)

#define GPIOX_MODE_(a,b,c)    ((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL = (((GPIO_TypeDef*)(((c & 8) >> 1) + GPIO ## b ## _BASE))->CRL & ~(0xF << ((c & 7) << 2))) | (a << ((c & 7) << 2))
#define GPIOX_MODE(a, b)      GPIOX_MODE_(a, b)

#define GPIOX_ODR_(a, b)      BITBAND_ACCESS(GPIO ## a ->ODR, b)
#define GPIOX_ODR(a)          GPIOX_ODR_(a)

#define GPIOX_SET_(a, b)      GPIO ## a ->BSRR = 1 << b
#define GPIOX_SET(a)          GPIOX_SET_(a)

#define GPIOX_CLR_(a, b)      GPIO ## a ->BSRR = 1 << (b + 16)
#define GPIOX_CLR(a)          GPIOX_CLR_(a)

#define GPIOX_IDR_(a, b)      BITBAND_ACCESS(GPIO ## a ->IDR, b)
#define GPIOX_IDR(a)          GPIOX_IDR_(a)

#define GPIOX_LINE_(a, b)     EXTI_Line ## b
#define GPIOX_LINE(a)         GPIOX_LINE_(a)

#define GPIOX_PORTSRC_(a, b)  GPIO_PortSourceGPIO ## a
#define GPIOX_PORTSRC(a)      GPIOX_PORTSRC_(a)

#define GPIOX_PINSRC_(a, b)   GPIO_PinSource ## b
#define GPIOX_PINSRC(a)       GPIOX_PINSRC_(a)

#define GPIOX_CLOCK_(a, b)    RCC_APB2ENR_IOP ## a ## EN
#define GPIOX_CLOCK(a)        GPIOX_CLOCK_(a)

#define GPIOX_PORTNUM_A       1
#define GPIOX_PORTNUM_B       2
#define GPIOX_PORTNUM_C       3
#define GPIOX_PORTNUM_D       4
#define GPIOX_PORTNUM_E       5
#define GPIOX_PORTNUM_F       6
#define GPIOX_PORTNUM_G       7
#define GPIOX_PORTNUM_H       8
#define GPIOX_PORTNUM_I       9
#define GPIOX_PORTNUM_J       10
#define GPIOX_PORTNUM_K       11
#define GPIOX_PORTNUM_(a, b)  GPIOX_PORTNUM_ ## a
#define GPIOX_PORTNUM(a)      GPIOX_PORTNUM_(a)

#define GPIOX_PORTNAME_(a, b) a
#define GPIOX_PORTNAME(a)     GPIOX_PORTNAME_(a)

//-----------------------------------------------------------------------------
/* freertos vs HAL */
#ifdef  osCMSIS
#define DelayMs(t)            osDelay(t)
#define GetTime()             osKernelSysTick()
#else
#define DelayMs(t)            HAL_Delay(t)
#define GetTime()             HAL_GetTick()
#endif

//==============================================================================
/* if LCD_D0..LCD_D3 is valid -> 8 bits mode */
#if GPIOX_PORTNUM(LCD_D0) >= GPIOX_PORTNUM_A && GPIOX_PORTNUM(LCD_D1) >= GPIOX_PORTNUM_A && GPIOX_PORTNUM(LCD_D2) >= GPIOX_PORTNUM_A && GPIOX_PORTNUM(LCD_D3) >= GPIOX_PORTNUM_A
#define LCD_DATABITS          8
#else
#define LCD_DATABITS          4
#endif

//------------------------------------------------------------------------------
/* LCD MODE ONCE BUSY */
#if     LCD_MODE == 1
#if     !GPIOX_PORTNUM(LCD_RW) >= GPIOX_PORTNUM_A
#error  LCD RW is undefinied
#endif
#undef  LCD_USERTIMER
#define LCD_USERTIMER   0
#endif

/* LCD MODE ONCE DELAY */
#if     LCD_MODE == 2
#ifndef LCD_EXEDELAY
#error  LCD EXEDELAY is undefinied
#endif
#undef  LCD_USERTIMER
#define LCD_USERTIMER   0
#endif

/* LCD MODE ONCE IRQ */
#if     LCD_MODE == 3
#endif

/* LCD MODE CONT BUSY */
#if     LCD_MODE == 4
#if     !GPIOX_PORTNUM(LCD_RW) >= GPIOX_PORTNUM_A
#error  LCD RW is undefinied
#endif
#undef  LCD_CURSOR
#define LCD_CURSOR      0
#undef  LCD_USERTIMER
#define LCD_USERTIMER   0
#endif

/* LCD MODE CONT IRQ */
#if     LCD_MODE == 5
#undef  LCD_CURSOR
#define LCD_CURSOR      0
#endif

#if (LCD_LINES != 1) && (LCD_LINES != 2) && (LCD_LINES != 4)
#error  LCD LINES value only 1, 2 or 4 !
#endif

#if (LCD_WIDTH > 40)
#error  Max LCD WIDTH = 40 !
#endif

//------------------------------------------------------------------------------
#if     LCD_USERTIMER == 0 && ((LCD_MODE == 3) || (LCD_MODE == 5))
#undef  LCD_USERTIMER
#if     LCD_TIMER == 1
#define LCD_TIMX              TIM1
#define LCD_TIMX_CLOCK        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN
#define LCD_TIMX_IRQn         TIM1_UP_IRQn
#define LCD_INTPROCESS        TIM1_UP_IRQHandler
#elif   LCD_TIMER == 2
#define LCD_TIMX              TIM2
#define LCD_TIMX_CLOCK        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN
#define LCD_TIMX_IRQn         TIM2_IRQn
#define LCD_INTPROCESS        TIM2_IRQHandler
#elif   LCD_TIMER == 3
#define LCD_TIMX              TIM3
#define LCD_TIMX_CLOCK        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN
#define LCD_TIMX_IRQn         TIM3_IRQn
#define LCD_INTPROCESS        TIM3_IRQHandler
#elif   LCD_TIMER == 4
#define LCD_TIMX              TIM4
#define LCD_TIMX_CLOCK        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN
#define LCD_TIMX_IRQn         TIM4_IRQn
#define LCD_INTPROCESS        TIM4_IRQHandler
#endif
#endif

//------------------------------------------------------------------------------
#if (1UL * LCD_LINES * LCD_WIDTH > 160)
#error  The caracter number is too many !
#endif

#if (1UL * LCD_LINES * LCD_WIDTH > 80)

#if     !GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
#error  LCD E2 pin is undefinied !
#endif

#if     LCD_LINES != 4
#error  if number of char > 80 -> required LCD LINES : 4 !
#endif

/* if > 80 chars -> 2 piece of 2 lines */
#undef  LCD_LINES
#define LCD_LINES        2
#endif  /* #if (1UL * LCD_LINES * LCD_WIDTH > 80) */

#define LCDCHARPERMODUL (1UL * LCD_LINES * LCD_WIDTH)
#define LCDCHARPERSEC   (1UL * LCD_FPS * (LCD_LINES * (LCD_WIDTH + 1)))

#if      GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
#define LCDTEXTSIZE     (2UL * LCD_WIDTH * LCD_LINES)
#else
#define LCDTEXTSIZE     (1UL * LCD_WIDTH * LCD_LINES)
#endif

/* LCD DDRAM address (begins for 1..4 lines) */
#define SETDDRAMADDR1  0x80
#define SETDDRAMADDR2  0xC0
#define SETDDRAMADDR3  (0x80 + LCD_WIDTH)
#define SETDDRAMADDR4  (0xC0 + LCD_WIDTH)

#define SETCGRAMADDR   0x40

//==============================================================================
/* Framebuffer array */
volatile char LcdText[LCDTEXTSIZE + 1];

volatile unsigned char LcdPos;          /* Actual DDRAM position */

/* LcdStatus value:
   - DDRADDR0: DDRAM address = 0 setting state (write text position = HOME)
   - DDRADDR:  DDRAM address setting state (write text position)
   - DDR:      DDRAM upload state (write text to LCD)
   - CURTYPE:  Cursor type change state
   - CURPOS:   Cursor position change state
   - CGRADDR:  CGRAM address setting state
   - CGR:      CGRAM upload state (write chargen to LCD)
   - REFREND:  Refresh end */
enum LS {DDRADDR0, DDRADDR, DDR, CURTYPE, CURPOS, CGRADDR, CGR, REFREND} LcdStatus = DDR;

//==============================================================================
#if LCD_DATABITS == 8

/* if the 8 data pins are in order -> automatic optimalization */
#if ((GPIOX_PORTNUM(LCD_D0) == GPIOX_PORTNUM(LCD_D1))\
  && (GPIOX_PORTNUM(LCD_D1) == GPIOX_PORTNUM(LCD_D2))\
  && (GPIOX_PORTNUM(LCD_D2) == GPIOX_PORTNUM(LCD_D3))\
  && (GPIOX_PORTNUM(LCD_D3) == GPIOX_PORTNUM(LCD_D4))\
  && (GPIOX_PORTNUM(LCD_D4) == GPIOX_PORTNUM(LCD_D5))\
  && (GPIOX_PORTNUM(LCD_D5) == GPIOX_PORTNUM(LCD_D6))\
  && (GPIOX_PORTNUM(LCD_D6) == GPIOX_PORTNUM(LCD_D7)))
#if ((GPIOX_PIN(LCD_D0) + 1 == GPIOX_PIN(LCD_D1))\
  && (GPIOX_PIN(LCD_D1) + 1 == GPIOX_PIN(LCD_D2))\
  && (GPIOX_PIN(LCD_D2) + 1 == GPIOX_PIN(LCD_D3))\
  && (GPIOX_PIN(LCD_D3) + 1 == GPIOX_PIN(LCD_D4))\
  && (GPIOX_PIN(LCD_D4) + 1 == GPIOX_PIN(LCD_D5))\
  && (GPIOX_PIN(LCD_D5) + 1 == GPIOX_PIN(LCD_D6))\
  && (GPIOX_PIN(LCD_D6) + 1 == GPIOX_PIN(LCD_D7)))
#if GPIOX_PIN(LCD_D0) == 0
/* LCD data pins on 0..7 pin (ex. B0,B1,B2,B3,B4,B5,B6,B7) */
#define LCD_AUTOOPT  1
#elif GPIOX_PIN(LCD_D0) == 8
/* LCD data pins on 8..15 pin (ex. B8,B9,B10,B11,B12,B13,B14,B15) */
#define LCD_AUTOOPT  2
#else
/* LCD data pins on n..n+7 pin (ex. B6,B7,B8,B9,B10,B11,B12,B13) */
#define LCD_AUTOOPT  3
#define LCD_DATA_DIRSET_(a,b,c)   *(uint64_t *)GPIO ## b ## _BASE = (*(uint64_t *)GPIO ## b ## _BASE & ~(0xFFFFFFFFLL << (c << 2))) | ((uint64_t)a << (c << 2))
#define LCD_DATA_DIRSET(a, b)     LCD_DATA_DIRSET_(a, b)
#endif
#endif /* D0..D7 pin order */
#endif /* D0..D7 port same */

#ifndef LCD_DIRREAD
#if     LCD_AUTOOPT == 1
#define LCD_DIRREAD  GPIOX_PORT(LCD_D0)->CRL = 0x44444444
#elif   LCD_AUTOOPT == 2
#define LCD_DIRREAD  GPIOX_PORT(LCD_D0)->CRH = 0x44444444
#elif   (LCD_AUTOOPT == 3)
#define LCD_DIRREAD  LCD_DATA_DIRSET(0x44444444, LCD_D0)
#else
#define LCD_DIRREAD { \
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D0); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D1);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D2); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D3);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D4); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D5);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D6); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D7);}
#endif
#endif

/* data pins set to output direction */
#ifndef LCD_DIRWRITE
#if     (LCD_AUTOOPT == 1)
#define LCD_DIRWRITE  GPIOX_PORT(LCD_D0)->CRL = 0x33333333
#elif   (LCD_AUTOOPT == 2)
#define LCD_DIRWRITE  GPIOX_PORT(LCD_D0)->CRH = 0x33333333
#elif   (LCD_AUTOOPT == 3)
#define LCD_DIRWRITE  LCD_DATA_DIRSET(0x33333333, LCD_D0)
#else
#define LCD_DIRWRITE { \
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D0); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D1);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D2); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D3);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D4); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D5);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D6); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D7);}
#endif
#endif

/* 8 bit data write to the data pins */
#ifndef LCD_WRITE
#ifdef  LCD_AUTOOPT
#define LCD_WRITE(dt) { \
  GPIOX_PORT(LCD_D0)->BSRR = (dt << GPIOX_PIN(LCD_D0)) | (0xFF << (GPIOX_PIN(LCD_D0) + 16));}
#else
#define LCD_WRITE(dt) {                      \
  GPIOX_ODR(LCD_D0) = BITBAND_ACCESS(dt, 0); \
  GPIOX_ODR(LCD_D1) = BITBAND_ACCESS(dt, 1); \
  GPIOX_ODR(LCD_D2) = BITBAND_ACCESS(dt, 2); \
  GPIOX_ODR(LCD_D3) = BITBAND_ACCESS(dt, 3); \
  GPIOX_ODR(LCD_D4) = BITBAND_ACCESS(dt, 4); \
  GPIOX_ODR(LCD_D5) = BITBAND_ACCESS(dt, 5); \
  GPIOX_ODR(LCD_D6) = BITBAND_ACCESS(dt, 6); \
  GPIOX_ODR(LCD_D7) = BITBAND_ACCESS(dt, 7); }
#endif
#endif

/* 8 bit data read from the data pins */
#ifndef LCD_READ
#ifdef  LCD_AUTOOPT
#define LCD_READ(dt) {                               \
  dt = GPIOX_PORT(LCD_D0)->IDR >> GPIOX_PIN(LCD_D0); }
#else
#define LCD_READ(dt) {                       \
  BITBAND_ACCESS(dt, 0) = GPIOX_IDR(LCD_D0); \
  BITBAND_ACCESS(dt, 1) = GPIOX_IDR(LCD_D1); \
  BITBAND_ACCESS(dt, 2) = GPIOX_IDR(LCD_D2); \
  BITBAND_ACCESS(dt, 3) = GPIOX_IDR(LCD_D3); \
  BITBAND_ACCESS(dt, 4) = GPIOX_IDR(LCD_D4); \
  BITBAND_ACCESS(dt, 5) = GPIOX_IDR(LCD_D5); \
  BITBAND_ACCESS(dt, 6) = GPIOX_IDR(LCD_D6); \
  BITBAND_ACCESS(dt, 7) = GPIOX_IDR(LCD_D7); }
#endif
#endif

//-----------------------------------------------------------------------------
#elif LCD_DATABITS == 4

/* if the 8 data pins are in order -> automatic optimalization */
#if ((GPIOX_PORTNUM(LCD_D4) == GPIOX_PORTNUM(LCD_D5))\
  && (GPIOX_PORTNUM(LCD_D5) == GPIOX_PORTNUM(LCD_D6))\
  && (GPIOX_PORTNUM(LCD_D6) == GPIOX_PORTNUM(LCD_D7)))
#if ((GPIOX_PIN(LCD_D4) + 1 == GPIOX_PIN(LCD_D5))\
  && (GPIOX_PIN(LCD_D5) + 1 == GPIOX_PIN(LCD_D6))\
  && (GPIOX_PIN(LCD_D6) + 1 == GPIOX_PIN(LCD_D7)))
#if GPIOX_PIN(LCD_D7) <= 7
/* LCD data pins on 0..7 pin (ex. B0..B3 .. B4..B7) */
#define LCD_AUTOOPT  1
#elif GPIOX_PIN(LCD_D4) >= 8
/* LCD data pins on 8..15 pin (ex. B8..B11 .. B12..B15) */
#define LCD_AUTOOPT  2
#else
/* LCD data pins on n..n+7 pin (ex. B6,B7,B8,B9) */
#define LCD_AUTOOPT  3
#define LCD_DATA_DIRSET_(a,b,c)   *(uint64_t *)GPIO ## b ## _BASE = (*(uint64_t *)GPIO ## b ## _BASE & ~(0xFFFFLL << (c << 2))) | ((uint64_t)a << (c << 2))
#define LCD_DATA_DIRSET(a, b)     LCD_DATA_DIRSET_(a, b)
#endif
#endif /* D0..D7 pin order */
#endif /* D0..D7 port same */

/* data pins set to input direction */
#ifndef LCD_DIRREAD
#if     LCD_AUTOOPT == 1
#define LCD_DIRREAD  GPIOX_PORT(LCD_D4)->CRL = (GPIOX_PORT(LCD_D4)->CRL & ~(0xFFFF << (4 * (GPIOX_PIN(LCD_D4) - 0)))) | (0x4444 << (4 * (GPIOX_PIN(LCD_D4) - 0)))
#elif   LCD_AUTOOPT == 2
#define LCD_DIRREAD  GPIOX_PORT(LCD_D4)->CRH = (GPIOX_PORT(LCD_D4)->CRH & ~(0xFFFF << (4 * (GPIOX_PIN(LCD_D4) - 8)))) | (0x4444 << (4 * (GPIOX_PIN(LCD_D4) - 8)))
#elif   (LCD_AUTOOPT == 3)
#define LCD_DIRREAD  LCD_DATA_DIRSET(0x4444, LCD_D4)
#else
#define LCD_DIRREAD { \
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D4); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D5);\
  GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D6); GPIOX_MODE(MODE_FF_DIGITAL_INPUT, LCD_D7);}
#endif
#endif

/* data pins set to output direction */
#ifndef LCD_DIRWRITE
#if     (LCD_AUTOOPT == 1)
#define LCD_DIRWRITE  GPIOX_PORT(LCD_D4)->CRL = (GPIOX_PORT(LCD_D4)->CRL & ~(0xFFFF << (4 * (GPIOX_PIN(LCD_D4) - 0)))) | (0x3333 << (4 * (GPIOX_PIN(LCD_D4) - 0)))
#elif   (LCD_AUTOOPT == 2)
#define LCD_DIRWRITE  GPIOX_PORT(LCD_D4)->CRH = (GPIOX_PORT(LCD_D4)->CRH & ~(0xFFFF << (4 * (GPIOX_PIN(LCD_D4) - 8)))) | (0x3333 << (4 * (GPIOX_PIN(LCD_D4) - 8)))
#elif   (LCD_AUTOOPT == 3)
#define LCD_DIRWRITE  LCD_DATA_DIRSET(0x3333, LCD_D4)
#else
#define LCD_DIRWRITE { \
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D4); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D5);\
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D6); GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_D7);}
#endif
#endif

/* 4 bit data write to the data pins (4..7 bit) */
#ifndef LCD_WRITE_HI
#ifdef  LCD_AUTOOPT
#if GPIOX_PIN(LCD_D4) == 4
#define LCD_WRITE_HI(dt) {GPIOX_PORT(LCD_D4)->BSRR = (dt & 0xF0) (0xF << (GPIOX_PIN(LCD_D4) + 16));}
#elif GPIOX_PIN(LCD_D4) > 4
#define LCD_WRITE_HI(dt) {GPIOX_PORT(LCD_D4)->BSRR = ((dt & 0xF0) << (GPIOX_PIN(LCD_D4) - 4)) | (0xF << (GPIOX_PIN(LCD_D4) + 16));}
#elif GPIOX_PIN(LCD_D4) < 4
#define LCD_WRITE_HI(dt) {GPIOX_PORT(LCD_D4)->BSRR = ((dt & 0xF0) >> (4 - GPIOX_PIN(LCD_D4))) | (0xF << (GPIOX_PIN(LCD_D4) + 16));}
#endif
#else  
#define LCD_WRITE_HI(dt) {;                    \
    GPIOX_ODR(LCD_D4) = BITBAND_ACCESS(dt, 4); \
    GPIOX_ODR(LCD_D5) = BITBAND_ACCESS(dt, 5); \
    GPIOX_ODR(LCD_D6) = BITBAND_ACCESS(dt, 6); \
    GPIOX_ODR(LCD_D7) = BITBAND_ACCESS(dt, 7); }
#endif  /* else LCDDTAUTOOPT */
#endif

/* 4 bit data write to the data pins (0..3 bit) */
#ifndef LCD_WRITE_LO
#ifdef  LCD_AUTOOPT
#define LCD_WRITE_LO(dt) { \
  GPIOX_PORT(LCD_D4)->BSRR = ((dt & 0x0F) << GPIOX_PIN(LCD_D4)) | (0xF << (GPIOX_PIN(LCD_D4) + 16));}
#else  /* LCDDTAUTOOPT */
#define LCD_WRITE_LO(dt) {;                    \
    GPIOX_ODR(LCD_D4) = BITBAND_ACCESS(dt, 0); \
    GPIOX_ODR(LCD_D5) = BITBAND_ACCESS(dt, 1); \
    GPIOX_ODR(LCD_D6) = BITBAND_ACCESS(dt, 2); \
    GPIOX_ODR(LCD_D7) = BITBAND_ACCESS(dt, 3); }
#endif /* else LCDDTAUTOOPT */
#endif /* ifndef LCD_WRITE_LO */

#endif

/* E pin impulse delay (0: none, 1: NOP, 2.. LCD_IO_Delay) */
#if     LCD_PULSEDELAY == 0
#define LCD_E_DELAY
#elif   LCD_PULSEDELAY == 1
#define LCD_E_DELAY          __NOP()
#else
#define LCD_E_DELAY          LCD_IO_Delay(LCD_PULSEDELAY - 2)
#endif

//==============================================================================

#ifdef  __GNUC__
#pragma GCC push_options
#pragma GCC optimize("O0")
#elif   defined(__CC_ARM)
#pragma push
#pragma O0
#endif
void LCD_IO_Delay(uint32_t c)
{
  while(c--);
}
#ifdef  __GNUC__
#pragma GCC pop_options
#elif   defined(__CC_ARM)
#pragma pop
#endif

/*==============================================================================
LcdBusy
 - LCD busy check
   (does not wait until it is released)
 return:
   1: busy
   0: not busy (operable)
==============================================================================*/
#if (LCD_MODE == 1) || (LCD_MODE == 4)
char LcdBusy(void)
{
  LCD_DIRREAD;                          /* data pins are input */
  GPIOX_SET(LCD_RW);                    /* data direction: LCD -> microcontroller (read) */
  GPIOX_CLR(LCD_RS);                    /* RS = 0 */
  LCD_E_DELAY;
  GPIOX_SET(LCD_E);
  LCD_E_DELAY;

  if(GPIOX_IDR(LCD_D7))                 /* busy flag ? */
  {
    GPIOX_CLR(LCD_E);
    #if LCD_DATABITS == 4
    LCD_E_DELAY;
    GPIOX_SET(LCD_E); LCD_E_DELAY; GPIOX_CLR(LCD_E);
    LCD_E_DELAY;
    #endif /* #if LCD_DATABITS == 4 */
    GPIOX_SET(LCD_RS);                  /* RS = 1 */
    return 1;                           /* busy */
  }
  GPIOX_CLR(LCD_E);
  #if LCD_DATABITS == 4
  LCD_E_DELAY;
  GPIOX_SET(LCD_E); LCD_E_DELAY; GPIOX_CLR(LCD_E);
  #endif /* #if LCD_DATABITS == 4 */

  #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
  LCD_E_DELAY;
  GPIOX_SET(LCD_E2);
  LCD_E_DELAY;
  if(GPIOX_IDR(LCD_D7))                 /* busy flag ? */
  {
    GPIOX_CLR(LCD_E2);
    #if LCD_DATABITS == 4
    LCD_E_DELAY;
    GPIOX_SET(LCD_E2); LCD_E_DELAY; GPIOX_CLR(LCD_E2);
    #endif /* #if LCD_DATABITS == 4 */
    GPIOX_SET(LCD_RS);                  /* RS = 1 */
    return 1;                           /* busy */
  }
  GPIOX_CLR(LCD_E2);
  #if LCD_DATABITS == 4
  LCD_E_DELAY;
  GPIOX_SET(LCD_E2); LCD_E_DELAY; GPIOX_CLR(LCD_E2);
  LCD_E_DELAY;
  #endif /* #if LCD_DATABITS == 4 */
  #endif /* LCD_E2 */

  GPIOX_SET(LCD_RS);                  /* RS = 1 */
  return 0;                             // szabad
}
#endif  /* #if (LCD_MODE == 1) || (LCD_MODE == 4) */

/*==============================================================================
LcdWrite (one byte write to LCD)
 - if LCD RS pin == 0 -> command write
 - if LCD RS pin == 1 -> data write
 - ch : data
==============================================================================*/
void LcdWrite(uint8_t ch)
{
  #if (LCD_MODE == 1) || (LCD_MODE == 4)
  GPIOX_CLR(LCD_RW);                    /* data direction: microcontroller -> LCD (write) */
  LCD_DIRWRITE;
  #endif

  #if LCD_DATABITS == 4
  LCD_WRITE_HI(ch);
  GPIOX_SET(LCD_E); LCD_E_DELAY; GPIOX_CLR(LCD_E); LCD_E_DELAY;
  LCD_WRITE_LO(ch);
  #elif LCD_DATABITS == 8
  LCD_WRITE(ch);
  #endif
  GPIOX_SET(LCD_E); LCD_E_DELAY; GPIOX_CLR(LCD_E); LCD_E_DELAY;
}

//------------------------------------------------------------------------------
#if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
void LcdWrite2(uint8_t ch)
{
  #if LCD_DATABITS == 4
  LCD_WRITE_HI(ch);
  GPIOX_SET(LCD_E2); LCD_E_DELAY; GPIOX_CLR(LCD_E2); LCD_E_DELAY;
  LCD_WRITE_LO(ch);
  #elif LCD_DATABITS == 8
  LCD_WRITE(ch);
  #endif
  GPIOX_SET(LCD_E2); LCD_E_DELAY; GPIOX_CLR(LCD_E2); LCD_E_DELAY;
}

void LcdWrite12(uint8_t ch)
{
  #if LCD_DATABITS == 4
  LCD_WRITE_HI(ch);
  GPIOX_SET(LCD_E); GPIOX_SET(LCD_E2); LCD_E_DELAY; GPIOX_CLR(LCD_E); GPIOX_CLR(LCD_E2); LCD_E_DELAY;
  LCD_WRITE_LO(ch);
  #elif LCD_DATABITS == 8
  LCD_WRITE(ch);
  #endif
  GPIOX_SET(LCD_E); GPIOX_SET(LCD_E2); LCD_E_DELAY; GPIOX_CLR(LCD_E); GPIOX_CLR(LCD_E2); LCD_E_DELAY;
}

#else
#define LcdWrite12(c)   LcdWrite(c)
#endif /* #else LCD_E2 */

#if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
#define  E_PULSE  {GPIOX_SET(LCD_E); GPIOX_SET(LCD_E2); LCD_E_DELAY; GPIOX_CLR(LCD_E); GPIOX_CLR(LCD_E2); LCD_E_DELAY; }
#else
#define  E_PULSE  {GPIOX_SET(LCD_E); LCD_E_DELAY; GPIOX_CLR(LCD_E); LCD_E_DELAY; }
#endif

//==============================================================================
#if LCD_CHARSETINIT == 1

const LCD_CHARSETARRAY LcdDefaultCharset =
{
  LCD_USR0_CHR0, LCD_USR0_CHR1, LCD_USR0_CHR2, LCD_USR0_CHR3, LCD_USR0_CHR4, LCD_USR0_CHR5, LCD_USR0_CHR6, LCD_USR0_CHR7,
  LCD_USR1_CHR0, LCD_USR1_CHR1, LCD_USR1_CHR2, LCD_USR1_CHR3, LCD_USR1_CHR4, LCD_USR1_CHR5, LCD_USR1_CHR6, LCD_USR1_CHR7,
  LCD_USR2_CHR0, LCD_USR2_CHR1, LCD_USR2_CHR2, LCD_USR2_CHR3, LCD_USR2_CHR4, LCD_USR2_CHR5, LCD_USR2_CHR6, LCD_USR2_CHR7,
  LCD_USR3_CHR0, LCD_USR3_CHR1, LCD_USR3_CHR2, LCD_USR3_CHR3, LCD_USR3_CHR4, LCD_USR3_CHR5, LCD_USR3_CHR6, LCD_USR3_CHR7,
  LCD_USR4_CHR0, LCD_USR4_CHR1, LCD_USR4_CHR2, LCD_USR4_CHR3, LCD_USR4_CHR4, LCD_USR4_CHR5, LCD_USR4_CHR6, LCD_USR4_CHR7,
  LCD_USR5_CHR0, LCD_USR5_CHR1, LCD_USR5_CHR2, LCD_USR5_CHR3, LCD_USR5_CHR4, LCD_USR5_CHR5, LCD_USR5_CHR6, LCD_USR5_CHR7,
  LCD_USR6_CHR0, LCD_USR6_CHR1, LCD_USR6_CHR2, LCD_USR6_CHR3, LCD_USR6_CHR4, LCD_USR6_CHR5, LCD_USR6_CHR6, LCD_USR6_CHR7,
  LCD_USR7_CHR0, LCD_USR7_CHR1, LCD_USR7_CHR2, LCD_USR7_CHR3, LCD_USR7_CHR4, LCD_USR7_CHR5, LCD_USR7_CHR6, LCD_USR7_CHR7
};
#endif // LCD_CHARSETINIT

/* User character pointer */
#if LCD_CHARSETCHANGE == 1
char*  uchp;
#endif

//==============================================================================
#if     LCD_BLINKCHAR == 1
volatile uint32_t BlinkPhase; /* 0 = blinked characters is visible, else not visible */
volatile char LcdBlink[(LCDTEXTSIZE + 7) / 8];
void LcdBlinkChar(uint32_t n) { if(n < LCDTEXTSIZE) LcdBlink[n >> 3] |= (1 << (n & 7)); }
void LcdUnBlinkChar(uint32_t n)  { if(n < LCDTEXTSIZE) LcdBlink[n >> 3] &= ~(1 << (n & 7)); }
#endif // LCD_BLINKCHAR == 1

//==============================================================================
// Cursor pos and cursor type
#if  LCD_CURSOR == 1
volatile unsigned char         LcdCursorPos = 0;
volatile unsigned char         LcdCursorType;
void LcdCursorOn(void)         { LcdCursorType |= 2; }
void LcdCursorOff(void)        { LcdCursorType &= ~2; }
void LcdCursorBlink(void)      { LcdCursorType |= 1; }
void LcdCursorUnBlink(void)    { LcdCursorType &= ~1; }
uint32_t LcdGetCursorPos(void) { return LcdCursorPos; }

#endif  // LCD_CURSOR == 1

//==============================================================================
#if (LCD_MODE == 3) || (LCD_MODE == 5) /* Interrupt mode */
#if LCD_USERTIMER == 1
uint32_t LcdIrqStatus;
void LcdIntProcess(void)    { if(!LcdIrqStatus)LcdProcess(); }
void LcdRefreshStart(void)  { LcdIrqStatus = 0; }
void LcdRefreshStop(void)   { LcdIrqStatus = 1; }
#if     LCD_MODE == 3
uint32_t LcdRefreshed(void) { return LcdIrqStatus; }
#else  /* #if LCD_MODE == 3 (LCD_MODE == 5) */
uint32_t LcdRefreshed(void) { return 0; }
#endif /* #else LCD_MODE == 3 */
#else
void LcdRefreshStart(void)  { LCD_TIMX->CR1 |= TIM_CR1_CEN; }
void LcdRefreshStop(void)   { LCD_TIMX->CR1 &= ~TIM_CR1_CEN; }
uint32_t LcdRefreshed(void) { if(LCD_TIMX->CR1 & TIM_CR1_CEN) return 0; else return 1; }
#endif /* #if LCD_USERTIMER == 1 */
#endif /* #if (LCD_MODE == 3) || (LCD_MODE == 5) */

/*==============================================================================
  LcdInit
  - I/O pins setting
  - LCD init
  - Memory array init
  - Timer init (if interrupt mode)
===============================================================================*/
void LcdInit(void)
{
  unsigned int i;
  uint8_t ch;

  #if 0
  RCC->APB2ENR |= GPIOX_CLOCK_(B, 8) | GPIOX_CLOCK_(B, 91) | GPIOX_CLOCK_(B, 10) | GPIOX_CLOCK_(B, 11);
  GPIOX_CLR_(B, 8); GPIOX_CLR_(B, 9); GPIOX_CLR_(B, 10); GPIOX_CLR_(B, 11);
  GPIOX_MODE_(MODE_PP_OUT_2MHZ, B, 8); GPIOX_MODE_(MODE_PP_OUT_2MHZ, B, 9); GPIOX_MODE_(MODE_PP_OUT_2MHZ, B, 10); GPIOX_MODE_(MODE_PP_OUT_2MHZ, B, 11);
  #endif

  /* GPIO Clock */
  #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_E2  GPIOX_CLOCK(LCD_E2)
  #else
  #define GPIOX_CLOCK_LCD_E2  0
  #endif

  #if GPIOX_PORTNUM(LCD_RW) >= GPIOX_PORTNUM_A
  #define GPIOX_CLOCK_LCD_RW  GPIOX_CLOCK(LCD_RW)
  #else
  #define GPIOX_CLOCK_LCD_RW  0
  #endif

  #if LCD_DATABITS == 8
  #define GPIOX_CLOCK_LCD_D0  GPIOX_CLOCK(LCD_D0)
  #define GPIOX_CLOCK_LCD_D1  GPIOX_CLOCK(LCD_D1)
  #define GPIOX_CLOCK_LCD_D2  GPIOX_CLOCK(LCD_D2)
  #define GPIOX_CLOCK_LCD_D3  GPIOX_CLOCK(LCD_D3)
  #elif LCD_DATABITS == 4
  #define GPIOX_CLOCK_LCD_D0  0
  #define GPIOX_CLOCK_LCD_D1  0
  #define GPIOX_CLOCK_LCD_D2  0
  #define GPIOX_CLOCK_LCD_D3  0
  #endif

  RCC->APB2ENR |= GPIOX_CLOCK(LCD_E)  | GPIOX_CLOCK(LCD_RS) |
                  GPIOX_CLOCK(LCD_D4) | GPIOX_CLOCK(LCD_D5) | GPIOX_CLOCK(LCD_D6) | GPIOX_CLOCK(LCD_D7) |
                  GPIOX_CLOCK_LCD_E2  | GPIOX_CLOCK_LCD_RW  |
                  GPIOX_CLOCK_LCD_D0  | GPIOX_CLOCK_LCD_D1  | GPIOX_CLOCK_LCD_D2  | GPIOX_CLOCK_LCD_D3;

  /* I/O pins setting */
  GPIOX_CLR(LCD_E);                     /* E = 0 */
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_E); /* E = out */

  #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
  GPIOX_CLR(LCD_E2);                    /* E2 = 0 */
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_E2);/* E2 = out */
  #endif

  GPIOX_CLR(LCD_RS);                    /* RS = 0 */
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RS);/* RS = out */

  #if GPIOX_PORTNUM(LCD_RW) >= GPIOX_PORTNUM_A
  GPIOX_CLR(LCD_RW);                    /* RW = 0 */
  GPIOX_MODE(MODE_PP_OUT_50MHZ, LCD_RW);/* RW = out */
  #endif

  /* LCD init sequence */
  #if LCD_DATABITS == 4
  ch = 0x00;                            /* 0b00000000 */
  LCD_WRITE_HI(ch);
  LCD_DIRWRITE;
  DelayMs(100);
  ch = 0x30;                            /* 0b00110000 */
  LCD_WRITE_HI(ch);
  E_PULSE;                              /* 0b0011 */
  DelayMs(7);
  E_PULSE;                              /* 0b0011 */
  DelayMs(7);
  E_PULSE;                              /* 0b0011 */
  DelayMs(7);
  ch = 0x20;                            /* 0b00100000 */
  LCD_WRITE_HI(ch);
  E_PULSE;                              /* 0b0010 */
  DelayMs(7);
  #if  LCD_LINES > 1
  LcdWrite12(0x28);                     /* 0b00101000: 4bit, 2line, 5x7dot */
  #else
  LcdWrite12(0x20);                     /* 0b00100000: 4bit, 1line, 5x7dot */
  #endif

  #elif LCD_DATABITS == 8
  ch = 0x00;                            /* 0b00000000 */
  LCD_WRITE(ch);
  LCD_DIRWRITE;
  DelayMs(100);
  ch = 0x30;                            /* 0b00110000 */
  LCD_WRITE(ch);
  E_PULSE;                              /* 0b00110000 */
  DelayMs(5);
  E_PULSE;                              /* 0b00110000 */
  DelayMs(5);
  E_PULSE;                              /* 0b00110000 */
  DelayMs(5);
  #if  LCD_LINES > 1
  LcdWrite12(0x38);                     /* 0b00111000: 8bit, 2line, 5x7dot */
  #else
  LcdWrite12(0x30);                     /* 0b00110000: 8bit, 1line, 5x7dot */
  #endif
  #endif // #elif LCD_DATABITS == 8

  DelayMs(7);
  LcdWrite12(0x06);                     /* 0b00000110: entry mode set: cursor move dir -> inc, display not shifted */

  DelayMs(5);
  LcdWrite12(0x0C);                     /* 0b00001100: display onoff: display on, cursor off, blinking off */

  DelayMs(2);
  LcdWrite12(0x01);                     /* 0b00000001: display clear */
  DelayMs(2);

  #if LCD_CHARSETINIT == 1
  DelayMs(1);
  LcdWrite(SETCGRAMADDR);               /* CGRAM = 0 */
  LcdWrite2(SETCGRAMADDR);

  GPIOX_SET(LCD_RS);
  for(i = 0; i < 64; i++)
  {
    DelayMs(2);
    LcdWrite12(LcdDefaultCharset[i]);
  }

  DelayMs(2);
  GPIOX_CLR(LCD_RS);
  LcdWrite12(SETDDRAMADDR1);            /* DDRAM = 1.line 1.pos */
  #endif /* LCD_CHARSETINIT */
  GPIOX_SET(LCD_RS);

  /* clear text */
  #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
  for(i = 0; i < (2UL * LCD_WIDTH * LCD_LINES); i++) LcdText[i] = ' ';
  #else
  for(i = 0; i < (1UL * LCD_WIDTH * LCD_LINES); i++) LcdText[i] = ' ';
  #endif

  /* blink array clear */
  #if LCD_BLINKCHAR == 1
  #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
  for(i = 0; i < ((2UL * LCD_WIDTH * LCD_LINES + 7) / 8); i++) LcdBlink[i] = 0;
  #else  /* LCD_E2 */
  for(i = 0; i < ((1UL * LCD_WIDTH * LCD_LINES + 7) / 8); i++) LcdBlink[i] = 0;
  #endif /* LCD_E2 */
  #endif /* LCD_BLINKCHAR */

  LcdPos = 0;

  /* Timer interrupt setting */
  #if (defined LCD_TIMX) && ((LCD_MODE == 3) || (LCD_MODE == 5))
  LCD_TIMX_CLOCK;

  uint32_t period;
  period = SystemCoreClock / (LCD_FPS * LCD_LINES * (LCD_WIDTH + 1));
  if(period >= 0x10000)
  { /* Prescaler */
    LCD_TIMX->PSC = period >> 16;
    period = period / (LCD_TIMX->PSC + 1);
  }
  else
    LCD_TIMX->PSC = 0;            /* no prescaler */
  LCD_TIMX->ARR = period;         /* autoreload */

  NVIC_SetPriority(LCD_TIMX_IRQn, LCD_TIMER_PR);
  NVIC_EnableIRQ(LCD_TIMX_IRQn);

  LCD_TIMX->DIER = TIM_DIER_UIE;
  LCD_TIMX->CNT = 0;              /* counter = 0 */
  LCD_TIMX->CR1 = TIM_CR1_ARPE;

  #if    LCD_MODE == 5
  LcdRefreshStart();
  #endif /* LCD_MODE == 5 */
  #endif /* (!defined LCD_USERTIMER) && ((defined LCD_MODE == 3) || (defined LCD_MODE == 5)) */
}

/*==============================================================================
LcdProcess (one byte write to LCD)
  Input:
  - LcdText[],
  - if enabled the blink mode: LcdBlink[],
  - *uchp
//============================================================================*/
#ifdef LCD_INTPROCESS
void LcdProcess(void) { return; }

void LCD_INTPROCESS(void)
#else
void LcdProcess(void)
#endif
{
  uint8_t ch;

  /* interrupt flag clear */
  #ifdef LCD_INTPROCESS
  LCD_TIMX->SR = 0;
  #endif

  /* LCD busy check */
  #if (LCD_MODE == 1) || (LCD_MODE == 4)
  if(LcdBusy())
    return;                             /* LCD is busy -> return */
  #endif

  /* Blink scheduling */
  #if (LCD_MODE == 5) && (LCD_BLINKCHAR == 1) && (LCD_BLINKSPEED > 0)
  static uint32_t BlinkTimer = LCD_BLINKSPEED * (LCD_LINES * (LCD_WIDTH + 1));
  if(!BlinkTimer--)
  {
    BlinkPhase = !BlinkPhase;
    BlinkTimer = LCD_BLINKSPEED * (LCD_LINES * (LCD_WIDTH + 1));
  }
  #endif

  //----------------------------------------------------------------------------
  /* DDRAM data write */
  if(LcdStatus == DDR)
  {
    ch = LcdText[LcdPos];               /* ch = actual character */
    
    #if LCD_ZEROCHANGE == 1
    if (ch == 0)
    {
      ch = ' ';                         /* #0 code character -> change to ' ' */
      #if LCD_ZEROCHANGEBACK == 1
      LcdText[LcdPos] = ' ';            /* in LcdText #0 -> ' ' change to */
      #endif
    }
    #endif /* LCD_ZEROCHANGE */

    #if LCD_BLINKCHAR == 1
    if((BlinkPhase) && (LcdBlink[LcdPos >> 3] & (1 << (LcdPos & 7))))
      ch = ' ';                         /* if blink phase == 1, and the character is blinked -> change to ' ' */
    #endif /* LCD_BLINKCHAR */

    LcdWrite(ch);

    /* ----------------------------------- double or two piece LCD -> another LCD */
    #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
    ch = LcdText[LcdPos + LCD_LINES * LCD_WIDTH];
    #if LCD_ZEROCHANGE == 1
    if (ch == 0)
    {
      ch = ' ';                         /* #0 code character -> change to ' ' */
      #if LCD_ZEROCHANGEBACK == 1
      LcdText[LcdPos + LCD_LINES * LCD_WIDTH] = ' '; /* in LcdText #0 -> ' ' change to */
      #endif
    }
    #endif /* LCD_ZEROCHANGE */
    #if LCD_BLINKCHAR == 1
    if((BlinkPhase) && (LcdBlink[(LcdPos + LCD_LINES * LCD_WIDTH) >> 3] & (1 << ((LcdPos + LCD_LINES * LCD_WIDTH) & 7))))
      ch = ' ';
    #endif /* LCD_BLINKCHAR */

    LcdWrite2(ch);
    #endif /* LCD_E2 */
    //------------------------------------

    LcdPos++;

    //------------------------------------ 1 line type
    #if LCD_LINES == 1
    if(LcdPos == LCD_WIDTH)             /* 1.line end ? */
    {
      #if LCD_CURSOR == 1
      LcdStatus = CURPOS;               /* cursor pos setting */
      #else /* LCD_CURSOR */
      #if    LCD_MODE == 3
      LcdRefreshStop();                 /* LCD refresh is completed -> stop */
      #else  /* LCD_MODE == 3 */
      LcdStatus = REFREND;              /* LCD refresh is completed */
      #endif /* else LCD_MODE == 3 */
      #endif /* else LCD_CURSOR */

      #if (LCD_MODE == 4) || (LCD_MODE == 5)
      LcdStatus = DDRADDR;              /* next step : DDRAM address setting */
      #endif
    }
    //------------------------------------ 2 line type
    #elif LCD_LINES == 2
    if(LcdPos == LCD_WIDTH)             /* 1.line end ? */
    {
      LcdStatus = DDRADDR;              /* next step : DDRAM address setting */
    }
    else if(LcdPos == LCD_WIDTH * 2)    /* 2.line end ? */
    {
      #if LCD_CURSOR == 1
      LcdStatus = CURPOS;               /* cursor pos setting */
      #else /* LCD_CURSOR */
      #if    LCD_MODE == 3
      LcdRefreshStop();                 /* LCD refresh is completed -> stop */
      #else  /* LCD_MODE == 3 */
      LcdStatus = REFREND;              /* LCD refresh is completed -> stop */
      #endif /* else LCD_MODE == 3 */
      #endif /* else LCD_CURSOR */

      #if (LCD_MODE == 4) || (LCD_MODE == 5)
      LcdStatus = DDRADDR;              /* next step : DDRAM address setting */
      #endif
    }
    //------------------------------------ 4 line type
    #elif LCD_LINES == 4
 
    if(LcdPos == LCD_WIDTH)             /* 1.line end ? */
    {
      LcdStatus = DDRADDR;              /* next step : DDRAM address setting */
    }
    else if(LcdPos == LCD_WIDTH * 2)    /* 2.line end ? */
    {
      LcdStatus = DDRADDR;              /* next step : DDRAM address setting */
    }
    else if(LcdPos == LCD_WIDTH * 3)    /* 3.line end ? */
    {
      LcdStatus = DDRADDR;              /* next step : DDRAM address setting */
    }
    else if(LcdPos == LCD_WIDTH * 4)    /* 4.line end ? */
    {
      #if LCD_CURSOR == 1
      LcdStatus = CURPOS;               /* cursor pos setting */
      #else // LCD_CURSOR
      #if      LCD_MODE == 3
      LcdRefreshStop();                 /* LCD refresh is completed -> stop */
      #else  /* LCD_MODE == 3 */
      LcdStatus = REFREND;              /* LCD refresh is completed -> stop */
      #endif // else LCD_MODE == 3
      #endif // LCD_CURSOR

      #if ((defined LCD_MODE == 4) || (defined LCD_MODE == 5))
      LcdStatus = DDRADDR;              /* next step : DDRAM address setting */
      #endif
    }
    #endif // LCD_LINES

  } // LcdStatus

  //----------------------------------------------------------------------------
  /* DDRAM address = HOME setting */
  else if(LcdStatus == DDRADDR0)
  {
    GPIOX_CLR(LCD_RS);                  /* Command (RS = 0) */
    LcdWrite12(SETDDRAMADDR1);          /* DDRAMADDR = 1.th line start */
    LcdPos = 0;
    LcdStatus = DDR;                    /* Next step: DDRAM data write */
    GPIOX_SET(LCD_RS);                  /* Data write (RS = 1) */
  }

  //----------------------------------------------------------------------------
  /* DDRAM address setting */
  else if(LcdStatus == DDRADDR)
  {
    GPIOX_CLR(LCD_RS);                  /* Command (RS = 0) */

    // ----------------------------------- 1 line type
    #if LCD_LINES == 1
    ch = SETDDRAMADDR1;
    LcdWrite(SETDDRAMADDR1);            /* DDRAMADDR = 1.th line start */
    LcdWrite2(SETDDRAMADDR1);
    LcdPos = 0;

    // ----------------------------------- 2 line type
    #elif LCD_LINES == 2
    if(LcdPos == LCD_WIDTH)
    {                                   /* end of 1.th line */
      LcdWrite12(SETDDRAMADDR2);        /* DDRAMADDR = 2.th line start */
    }
    else
    {                                   /* end of 2.th line */
      LcdWrite12(SETDDRAMADDR1);        /* DDRAMADDR = 1.th line start */
      LcdPos = 0;
    }

    // ----------------------------------- 4 line type
    #elif LCD_LINES == 4

    /* 4 lines (max 80 characters) */
    if(LcdPos == LCD_WIDTH)
    {                                   /* end of 1.th line */
      LcdWrite12(SETDDRAMADDR2);        /* DDRAMADDR = 2.th line start */
    }
    else if(LcdPos == (LCD_WIDTH * 2))
    {                                   /* end of 2.th line */
      LcdWrite12(SETDDRAMADDR3);        /* DDRAMADDR = 3.th line start */
    }
    else if(LcdPos == (LCD_WIDTH * 3))
    {                                   /* end of 3.th line */
      LcdWrite12(SETDDRAMADDR4);        /* DDRAMADDR = 4.th line start */
    }
    else if(LcdPos == (LCD_WIDTH * 4))
    {                                   /* end of 4.th line */
      LcdWrite12(SETDDRAMADDR1);        /* DDRAMADDR = 1.th line start */
      LcdPos = 0;
      BLINKER();
    }
    #endif // LCD_LINES
    LcdStatus = DDR;                    /* Next step: DDRAM data write */
    GPIOX_SET(LCD_RS);                  /* Data write (RS = 1) */
  }

  //----------------------------------------------------------------------------
  /* Charset address */
  #if LCD_CHARSETCHANGE == 1
  else if(LcdStatus == CGRADDR)
  {                                     /* CGRAM address setting */
    GPIOX_CLR(LCD_RS);                  /* Command (RS = 0) */
    LcdWrite12(SETCGRAMADDR);           /* CGRAM address = 0 */
    LcdPos = 0;                         /* Chargen counter = 0 */
    LcdStatus = CGR;                    /* Next step: Chargen write */
    GPIOX_SET(LCD_RS);                  /* Data write (RS = 1) */
  }

  //----------------------------------------------------------------------------
  /* Charset write */
  else if(LcdStatus == CGR)
  {
    ch = *uchp++;                       /* Character generator data */
    LcdWrite(ch);
    LcdWrite2(ch);
    LcdPos++;
    if(LcdPos >= 64)
    {                                   /* End of character generator data */
      #if (LCD_MODE == 1) || (LCD_MODE == 2)
      LcdStatus = REFREND;              /* Next step: Refresh end */

      #elif  LCD_MODE == 3
      LcdStatus = DDRADDR0;             /* Next step: DDRAM address setting to HOME */

      #elif (LCD_MODE == 4) || (LCD_MODE == 5)
      LcdStatus = DDRADDR0;             /* Next step: DDRAM address setting to HOME */
      #endif /* (defined LCD_MODE == 4) || (defined LCD_MODE == 5) */
    }
  }
  #endif /* LCD_CHARSETCHANGE */

  //----------------------------------------------------------------------------
  /* Cursor position setting */
  #if LCD_CURSOR == 1
  else if(LcdStatus == CURPOS)
  {
    GPIOX_CLR(LCD_RS);                  /* Command (RS = 0) */

    // ----------------------------------- 1 line type
    #if LCD_LINES == 1
    LcdWrite(SETDDRAMADDR1 + LcdCursorPos);/* In 1.th line + cursor position */

    #elif LCD_LINES == 2
    // ----------------------------------- 2 piece of 2 line type
    #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
    if(LcdCursorPos < LCDCHARPERMODUL)
    {                                   /* The cursor position in a 1.th module */
      if (LcdCursorPos < LCD_WIDTH)
        ch = SETDDRAMADDR1 + LcdCursorPos;/* In 1.th line + cursor position */
      else
        ch = SETDDRAMADDR2 - LCD_WIDTH + LcdCursorPos;/* In 2.th line + cursor position */
    }
    else
    {                                   /* The cursor position in a 2.th module */
      if (LcdCursorPos < LCDCHARPERMODUL + LCD_WIDTH)
        ch = SETDDRAMADDR1 - LCDCHARPERMODUL + LcdCursorPos;/* In 1.th line + cursor position */
      else
        ch = SETDDRAMADDR2 - LCDCHARPERMODUL - LCD_WIDTH + LcdCursorPos;/* In 2.th line + cursor position */
    }
    LcdWrite12(ch);
    #else  // LCD_E2

    // ----------------------------------- 2 line type
    if (LcdCursorPos < LCD_WIDTH)
      ch = SETDDRAMADDR1 + LcdCursorPos;/* In 1.th line + cursor position */
    else
      ch = SETDDRAMADDR2 - LCD_WIDTH + LcdCursorPos;/* In 2.th line + cursor position */
    LcdWrite12(ch);
    #endif // else LCD_E2

    // ----------------------------------- 2 piece of 4 line type
    #elif LCD_LINES == 4
    #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
    if(LcdCursorPos < LCDCHARPERMODUL)
    {                                   /* The cursor position in a 1.th module */
      if(LcdCursorPos < LCD_WIDTH)
      {
        ch = SETDDRAMADDR1 + LcdCursorPos;/* In 1.th line + cursor position */
      }
      else if(LcdCursorPos < 2 * LCD_WIDTH)
      {
        ch = SETDDRAMADDR2 - LCD_WIDTH + LcdCursorPos;/* In 2.th line + cursor position */
      }
      else if(LcdCursorPos < 3 * LCD_WIDTH)
      {
        ch = SETDDRAMADDR3 - 2*LCD_WIDTH + LcdCursorPos;/* In 3.th line + cursor position */
      }
      else
      {
        ch = SETDDRAMADDR4 - 3*LCD_WIDTH + LcdCursorPos;/* In 4.th line + cursor position */
      }
    }
    else
    {                                   /* The cursor position in a 2.th module */
      if(LcdCursorPos < LCD_WIDTH + LCDCHARPERMODUL)
      {
        ch = SETDDRAMADDR1 - LCDCHARPERMODUL + LcdCursorPos;/* In 1.th line + cursor position */
      }
      else if(LcdCursorPos < 2 * LCD_WIDTH + LCDCHARPERMODUL)
      {
        ch = SETDDRAMADDR2 - LCDCHARPERMODUL - LCD_WIDTH + LcdCursorPos;/* In 2.th line + cursor position */
      }
      else if(LcdCursorPos < 3 * LCD_WIDTH + LCDCHARPERMODUL)
      {
        ch = SETDDRAMADDR3 - LCDCHARPERMODUL - 2*LCD_WIDTH + LcdCursorPos;/* In 3.th line + cursor position */
      }
      else
      {
        ch = SETDDRAMADDR4 - LCDCHARPERMODUL - 3*LCD_WIDTH + LcdCursorPos;/* In 4.th line + cursor position */
      }
    }
    LcdWrite12(ch);
  
    // ----------------------------------- 4 line type
    #else  /* LCD_E2 */
    if(LcdCursorPos < LCD_WIDTH)
    {
      ch = SETDDRAMADDR1 + LcdCursorPos;/* In 1.th line + cursor position */
    }
    else if(LcdCursorPos < 2 * LCD_WIDTH)
    {
      ch = SETDDRAMADDR2 - LCD_WIDTH + LcdCursorPos;/* In 2.th line + cursor position */
    }
    else if(LcdCursorPos < 3 * LCD_WIDTH)
    {
      ch = SETDDRAMADDR3 - 2*LCD_WIDTH + LcdCursorPos;/* In 3.th line + cursor position */
    }
    else
    {
      ch = SETDDRAMADDR4 - 3*LCD_WIDTH + LcdCursorPos;/* In 4.th line + cursor position */
    }
    LcdWrite12(ch);

    #endif // else LCD_E2
    #endif // LCD_LINES

    LcdStatus = CURTYPE;                /* netx step: Cursor type change */
  }

  //----------------------------------------------------------------------------
  /* Cursor type change */
  else if(LcdStatus == CURTYPE)
  {
    GPIOX_CLR(LCD_RS);                  /* Command (RS = 0) */

    #if GPIOX_PORTNUM(LCD_E2) >= GPIOX_PORTNUM_A
    if(LcdCursorPos < LCDCHARPERMODUL)
    {                                   /* cursor in 1.modul */
      LcdWrite(LcdCursorType | 0xC0);   /* 0b00001100: + cursor setting */
      LcdWrite2(0xC0);                  /* 0b00001100: cursor off */
    }
    else
    {                                   /* cursor in 2.modul */
      LcdWrite(0xC0);                   /* 0b00001100: cursor off */
      LcdWrite2(LcdCursorType | 0xC0);  /* 0b00001100: + cursor setting */
    }

    #else  /* LCD_E2 */
    LcdWrite(LcdCursorType | 0xC0);     /* 0b00001100 */
    #endif /* LCD_E2 */

    #if    LCD_MODE == 3
    LcdRefreshStop();                   /* Refresh is completed -> STOP */
    #else  // LCD_MODE == 3
    LcdStatus = REFREND;                /* Refresh is completed -> LcdStatus = REFREND */
    #endif // else LCD_MODE == 3
  }
  #endif // LCD_CURSOR
}

#if (LCD_MODE == 1) || (LCD_MODE == 2) || (LCD_MODE == 3)
/*==============================================================================
LcdRefreshAll: All LcdText buffer write to LCD
  Precondition:
  - LcdInit
  Input:
  - LcdText[]
  - LcdBlink[] (if LCD_BLINKCHAR == 1)
==============================================================================*/
void LcdRefreshAll(void)
{
  #if    LCD_MODE == 1
  LcdStatus = DDRADDR0;
  while(LcdStatus != REFREND)
    LcdProcess();
  #elif  LCD_MODE == 2
  LcdStatus = DDRADDR0;
  while(LcdStatus != REFREND)
  {
    LCD_IO_Delay(LCD_EXEDELAY);         /* Lcd write sheduler */
    LcdProcess();
  }
  #elif  LCD_MODE == 3
  #if    LCD_CHARSETCHANGE == 1
  if(!LcdRefreshed())
  { /* The previous one has not been completed yet */
    if((LcdStatus == CGRADDR) || (LcdStatus == CGR))
    {
    }
    else
    {
      LcdStatus = DDRADDR0;
      LcdRefreshStart();
    }
  }
  else
  { /* The previous one has been completed yet */
    LcdStatus = DDRADDR0;
    LcdRefreshStart();
  }
  #else  // LCD_CHARSETCHANGE
  LcdStatus = DDRADDR0;
  LcdRefreshStart();
  #endif // LCD_CHARSETCHANGE
  #endif // LCD_MODE == 3
}

//==============================================================================
#if  LCD_CURSOR == 1
void LcdSetCursorPos(uint32_t cp)
{
  if(cp < LCDTEXTSIZE)
    LcdCursorPos = cp;
  else
    return;

  #if    LCD_MODE == 1
  LcdStatus = CURPOS;
  while(LcdStatus != REFREND)
    LcdProcess();
  #elif  LCD_MODE == 2
  LcdStatus = CURPOS;
  while(LcdStatus != REFREND)
  {
    LCD_IO_Delay(LCD_EXEDELAY);         /* Lcd write sheduler */
    LcdProcess();
  }
  #elif  LCD_MODE == 3
  #if    LCD_CHARSETCHANGE == 1
  if(!LcdRefreshed())
  { /* The previous one has not been completed yet */
    if((LcdStatus == CURPOS) || (LcdStatus == CURTYPE))
    {
    }
    else
    {
      LcdStatus = CURPOS;
      LcdRefreshStart();
    }
  }
  else
  { /* The previous one has been completed yet */
    LcdStatus = DDRADDR0;
    LcdRefreshStart();
  }
  #else  // LCD_CHARSETCHANGE
  LcdStatus = DDRADDR0;
  LcdRefreshStart();
  #endif // LCD_CHARSETCHANGE
  #endif // LCD_MODE == 3
}
#endif


#endif //  ((defined LCD_MODE == 1) || (defined LCD_MODE == 2) || (defined LCD_MODE == 3))

/*==============================================================================
LcdChangeCharset: Change the user character set
  Precondition:
  - LcdInit(), LCD_CHARSETCHANGE == 1
  Input
  - pch*: pointer of 64 byte size character generator data
==============================================================================*/
#if LCD_CHARSETCHANGE == 1
void LcdChangeCharset(char* pch)
{
  uchp = pch;                           /* Character generator pointer */
  LcdStatus = CGRADDR;                  /* Character generator address */

  #if    LCD_MODE == 1
  while(LcdStatus != REFREND)
    LcdProcess();
  #elif  LCD_MODE == 2
  while(LcdStatus != REFREND)
  {
    LCD_IO_Delay(LCD_EXEDELAY);         /* Lcd write sheduler */
    LcdProcess();
  }
  #elif    LCD_MODE == 3
  LcdRefreshStart();
  #endif
}
#endif // LCD_CHARSETCHANGE
