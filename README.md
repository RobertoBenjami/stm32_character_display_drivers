# STM32Fxxx character display drivers

I converted my old character LCD drive to stm32

Layer chart, examples circuits and settings:
- Char_Lcd_drv.pdf ( https://github.com/RobertoBenjami/stm32_character_display_drivers/blob/master/Char_Lcd_drv.pdf )

App:
- helloworld: Minimal LCD program
- democlock: Clock demo
- lcd_speedmeter: measures the speed at which the LCD can be operated
- cpuusagemeter: measures how much cpu is used for continuous interrupt update

How to use starting from zero?

1. Create project for Cubemx
- setting the RCC (Crystal/ceramic resonator)
- setting the debug (SYS / serial wire or trace assyn sw)
- setting the timebase source (i like the basic timer for this)
- setting the clock configuration
- project settings: project name, toolchain = truestudio, stack size = 0x800
- generate source code
2. Truestudio
- open projects from file system
- open main.c
- add USER CODE BEGIN PFP: void mainApp(void);
- add USER CODE BEGIN WHILE: mainApp();
- open main.h
- add USER CODE BEGIN Includes (#include "stm32f1xx_hal.h" or ...)
- add 2 new folder for Src folder (App, Lcd)
- copy file(s) from App/... to App
- copy 2 files from Drivers to Lcd (charlcd.h, charlcd.c)
- if printf to SWO : copy syscalls.c to Src folder
- setting the configuration the driver header file (pin settings, speed settings etc...)
- add include path : Src/Lcd
- setting the compile options (Enable paralell build, optimalization)
- compile, run ...

How to adding the SWO support to cheap stlink ?
https://lujji.github.io/blog/stlink-clone-trace/

