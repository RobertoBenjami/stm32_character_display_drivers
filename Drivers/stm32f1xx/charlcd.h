/* author:  Roberto Benjami
   version: 2020.05 */

#ifndef __CHARLCD_H
#define __CHARLCD_H

/*==============================================================================
Configuration section, please change the settings !
================================================================================

LCD MODE (1..5):
- 1: Full framebuffer copy mode with BUSY flag check
     Use:
     - 1. put the text to LcdText[] array
     - 2. use the LcdRefreshAll function for copy to LCD
     note:
     - the LcdRefreshAll function is return when transaction is completed
- 2: Full framebuffer copy mode with delay
     Use:
     - 1. put the text to LcdText[] array
     - 2. use the LcdRefreshAll function for copy to LCD
     note:
     - the LcdRefreshAll function is return when transaction is completed
- 3: Full framebuffer copy mode with timer interrupt
     Use:
     - 1. put the text to LcdText[] array
     - 2. use the LcdRefreshAll function for copy to LCD
     note:
     - the LcdRefreshAll function is return immediately,
       we can check the end of the transaction with the LcdRefreshed function.
- 4: Continuous operation in the program loop with BUSY flag check
     Use:
     - 1. put the text to LcdText[] array
       2. LcdProcess function copies one character from the framebuffer to the LCD each cycle
- 5: Continuous operation in the timer interrupt
     Use:
     - 1. put the text to LcdText[] array
note: in each mode, the LcdInit function must be operated first */
#define LCD_MODE       2

/*==============================================================================
LCD I/O pins:
 - E2: (only double controlled LCD (ex. 4 x 40 characters) or double LCD)
 - R/W: (not necessary in LCD_MODE == 2 or 3 or 5)
 - D0..D3: (not necessary in 4bit mode) */
#define LCD_E        X, 0
#define LCD_E2       X, 0   /* If not used leave it that way */
#define LCD_RW       X, 0   /* If not used leave it that way */
#define LCD_RS       X, 0
#define LCD_D0       X, 0   /* If not used leave it that way */
#define LCD_D1       X, 0   /* If not used leave it that way */
#define LCD_D2       X, 0   /* If not used leave it that way */
#define LCD_D3       X, 0   /* If not used leave it that way */
#define LCD_D4       X, 0
#define LCD_D5       X, 0
#define LCD_D6       X, 0
#define LCD_D7       X, 0

/*==============================================================================
LCD lines (1, 2 or 4) */
#define LCD_LINES        2
/*-----------------------
LCD width (16, 20, 24, 40) */
#define LCD_WIDTH       16
/*------------------------------------------------------------------------------
Select the timer interrupt (only LCD_MODE == 3 or 5) */
#define LCD_TIMER        4
/*-----------------------
Timer interrupt priority (0..255 -> high priority..low priority) */
#define LCD_TIMER_PR     255
/*-----------------------
User-created timer  (only LCD_MODE == 3 or 5)
- 0: user timer disabled
- 1: user timer enabled (user-created scheduled display update) */
#define LCD_USERTIMER    0
/*-----------------------
Frame per secundum (only LCD MODE == 3 or 5, default value: 20) */
#define LCD_FPS          20
/*------------------------------------------------------------------------------
Delay cycle for LCD write and read E pin pulse (default value: 2) */
#define LCD_PULSEDELAY   2
/*-----------------------
Delay cycle for LCD execution time (only LCD MODE == 2, value for minimum 42usec delay)
- The value depends on the processor clock and display. First add a large value,
  then start decreasing until the display works correctly
  Default value: 360 (50usec on stm32f103 with 72MHz) */
#define LCD_EXEDELAY    360
/*------------------------------------------------------------------------------
Blinkable characters
- 0: cannot blinkable
- 1: can blinkable */
#define LCD_BLINKCHAR   0
/*-----------------------
Blink speed (only LCD_MODE == 5, how many frames should I switch?) */
#define LCD_BLINKSPEED  5
/*------------------------------------------------------------------------------
Cursor option
- 0: cursor disable
- 1: cursor enable (only LCD_MODE == 1 or 2 or 3) */
#define LCD_CURSOR      0
/*------------------------------------------------------------------------------
Change the #0 character to SPACE character when copy to LCD ?
- 0: no change
- 1: change (I recommend codes #8..#15 for user definied characters */
#define LCD_ZEROCHANGE    1
/*-----------------------
Change the #0 character to SPACE character in frame buffer ?
- 0: no change
- 1: change (I recommend codes #8..#15 for user definied characters */
#define LCD_ZEROCHANGEBACK 0
/*==============================================================================
User characterset upload in init
- 0: disabled the user character set
- 1: enabled the user character set (it also uploads at init) */
#define LCD_CHARSETINIT    0
/*-----------------------
Can change the character set anytime ?
- 0: disabled
- 1: enabled (see LCD_CHARSETARRAY) */
#define LCD_CHARSETCHANGE  0
/*-----------------------
User characterset definition (8 * 5x8 pixels character, if bit == 0 -> bright pixel, bit == 1 -> dark pixel)
note: if botton line is dark -> not visible the small cursor */
#define LCD_USR0_CHR0   0x1F /* 0b11111 */
#define LCD_USR0_CHR1   0x1B /* 0b11011 */
#define LCD_USR0_CHR2   0x15 /* 0b10101 */
#define LCD_USR0_CHR3   0x15 /* 0b10101 */
#define LCD_USR0_CHR4   0x15 /* 0b10101 */
#define LCD_USR0_CHR5   0x15 /* 0b10101 */
#define LCD_USR0_CHR6   0x1B /* 0b11011 */
#define LCD_USR0_CHR7   0x1F /* 0b11111 */

#define LCD_USR1_CHR0   0x1F /* 0b11111 */
#define LCD_USR1_CHR1   0x1D /* 0b11101 */
#define LCD_USR1_CHR2   0x19 /* 0b11001 */
#define LCD_USR1_CHR3   0x15 /* 0b10101 */
#define LCD_USR1_CHR4   0x1D /* 0b11101 */
#define LCD_USR1_CHR5   0x1D /* 0b11101 */
#define LCD_USR1_CHR6   0x1D /* 0b11101 */
#define LCD_USR1_CHR7   0x1F /* 0b11111 */

#define LCD_USR2_CHR0   0x1F /* 0b11111 */
#define LCD_USR2_CHR1   0x13 /* 0b10011 */
#define LCD_USR2_CHR2   0x15 /* 0b10101 */
#define LCD_USR2_CHR3   0x1D /* 0b11101 */
#define LCD_USR2_CHR4   0x1B /* 0b11011 */
#define LCD_USR2_CHR5   0x17 /* 0b10111 */
#define LCD_USR2_CHR6   0x11 /* 0b10001 */
#define LCD_USR2_CHR7   0x1F /* 0b11111 */

#define LCD_USR3_CHR0   0x1F /* 0b11111 */
#define LCD_USR3_CHR1   0x13 /* 0b10011 */
#define LCD_USR3_CHR2   0x1D /* 0b11101 */
#define LCD_USR3_CHR3   0x13 /* 0b10011 */
#define LCD_USR3_CHR4   0x1D /* 0b11101 */
#define LCD_USR3_CHR5   0x1D /* 0b11101 */
#define LCD_USR3_CHR6   0x13 /* 0b10011 */
#define LCD_USR3_CHR7   0x1F /* 0b11111 */

#define LCD_USR4_CHR0   0x1F /* 0b11111 */
#define LCD_USR4_CHR1   0x1D /* 0b11101 */
#define LCD_USR4_CHR2   0x19 /* 0b11001 */
#define LCD_USR4_CHR3   0x15 /* 0b10101 */
#define LCD_USR4_CHR4   0x11 /* 0b10001 */
#define LCD_USR4_CHR5   0x1D /* 0b11101 */
#define LCD_USR4_CHR6   0x1D /* 0b11101 */
#define LCD_USR4_CHR7   0x1F /* 0b11111 */

#define LCD_USR5_CHR0   0x1F /* 0b11111 */
#define LCD_USR5_CHR1   0x11 /* 0b10001 */
#define LCD_USR5_CHR2   0x17 /* 0b10111 */
#define LCD_USR5_CHR3   0x13 /* 0b10011 */
#define LCD_USR5_CHR4   0x1D /* 0b11101 */
#define LCD_USR5_CHR5   0x1D /* 0b11101 */
#define LCD_USR5_CHR6   0x13 /* 0b10011 */
#define LCD_USR5_CHR7   0x1F /* 0b11111 */

#define LCD_USR6_CHR0   0x1F /* 0b11111 */
#define LCD_USR6_CHR1   0x19 /* 0b11001 */
#define LCD_USR6_CHR2   0x17 /* 0b10111 */
#define LCD_USR6_CHR3   0x13 /* 0b10011 */
#define LCD_USR6_CHR4   0x15 /* 0b10101 */
#define LCD_USR6_CHR5   0x15 /* 0b10101 */
#define LCD_USR6_CHR6   0x1B /* 0b11011 */
#define LCD_USR6_CHR7   0x1F /* 0b11111 */

#define LCD_USR7_CHR0   0x1F /* 0b11111 */
#define LCD_USR7_CHR1   0x11 /* 0b10001 */
#define LCD_USR7_CHR2   0x1D /* 0b11101 */
#define LCD_USR7_CHR3   0x1D /* 0b11101 */
#define LCD_USR7_CHR4   0x1B /* 0b11011 */
#define LCD_USR7_CHR5   0x17 /* 0b10111 */
#define LCD_USR7_CHR6   0x17 /* 0b10111 */
#define LCD_USR7_CHR7   0x1F /* 0b11111 */

/*==============================================================================
I/O group optimization so that GPIO operations are not performed bit by bit:
Note: If the pins are in order, they will automatically optimize.
The example belongs to the following pins:
      LCD_D0<-D14, LCD_D1<-D15, LCD_D2<-D0, LCD_D3<-D1
      LCD_D4<-E7,  LCD_D5<-E8,  LCD_D6<-E9, LCD_D7<-E10 */
#if 0
/* datapins setting to output (data direction: STM32 -> LCD) */
#define LCD_DIRWRITE { /* D0..D1, D14..D15, E7..E10 <- 0x3 */ \
GPIOD->CRH = (GPIOD->CRH & ~0xFF000000) | 0x33000000; \
GPIOD->CRL = (GPIOD->CRL & ~0x000000FF) | 0x00000033; \
GPIOE->CRL = (GPIOE->CRL & ~0xF0000000) | 0x30000000; \
GPIOE->CRH = (GPIOE->CRH & ~0x00000FFF) | 0x00000333; }
/* datapins setting to input (data direction: STM32 <- LCD) */
#define LCD_DIRREAD { /* D0..D1, D14..D15, E7..E10 <- 0x4 */ \
GPIOD->CRH = (GPIOD->CRH & ~0xFF000000) | 0x44000000; \
GPIOD->CRL = (GPIOD->CRL & ~0x000000FF) | 0x00000044; \
GPIOE->CRL = (GPIOE->CRL & ~0xF0000000) | 0x40000000; \
GPIOE->CRH = (GPIOE->CRH & ~0x00000FFF) | 0x00000444; }
/* datapins write, STM32 -> LCD (write I/O pins from dt data) */
#define LCD_WRITE(dt) { /* D14..15 <- dt0..1, D0..1 <- dt2..3, E7..10 <- dt4..7 */ \
GPIOD->ODR = (GPIOD->ODR & ~0b1100000000000011) | (((dt & 0b00000011) << 14) | ((dt & 0b00001100) >> 2)); \
GPIOE->ODR = (GPIOE->ODR & ~0b0000011110000000) | ((dt & 0b11110000) << 3); }
/* Note: the keil compiler cannot use binary numbers, convert it to hexadecimal */
#endif

/*==============================================================================
Interface section (please don't modify) !
==============================================================================*/

/* LCD text (put in this the text) */
extern volatile char LcdText[];

/* LCD initialization */
void LcdInit(void);

/* One byte write to LCD (only LCD_MODE == 4) */
void LcdProcess(void);

/* All LcdText buffer write to LCD (only LCD_MODE == 1 or 2 or 3) */
void LcdRefreshAll(void);

/* One byte write to LCD (only LCD_MODE == 3 or 5 and LCD_USERTIMER == 1) */
void LcdIntProcess(void);

/* Timer start for LCD refresh (only LCD_MODE == 5) */
void LcdRefreshStart(void);

/* Timer stop for LCD refresh (only LCD_MODE == 5, pause mode) */
void LcdRefreshStop(void);

/* How has the LcdRefreshAll function been completed? (only LCD_MODE == 3)
   - return 0: completed
   - return 1: not completed */
uint32_t LcdRefreshed(void);

/* Cursor (only LCD_MODE == 1 or 2 or 3) */
void LcdSetCursorPos(uint32_t cp);
uint32_t LcdGetCursorPos(void);
void LcdCursorOn(void);
void LcdCursorOff(void);
void LcdCursorBlink(void);
void LcdCursorUnBlink(void);

/* Blink phase, blink on, blink off */
void LcdBlinkPhase(uint32_t n);      /* n=0: blinked characters is visible, n=1: not visible, n>=2: toggle */
void LcdBlinkChar(uint32_t n);       /* n-th character blink on */
void LcdUnBlinkChar(uint32_t n);     /* n-th character blink off */

/* User definied characters */
typedef char  LCD_CHARSETARRAY[64];  /* User character set type */
extern  const LCD_CHARSETARRAY LcdDefaultCharset; /* Default user character set */
void    LcdChangeCharset(char* pch); /* Change the user character set */

//------------------------------------------------------------------------------

#endif // __CHARLCD_H
