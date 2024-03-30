#include "main.h"
#include "Time_Delays.h"
#include "Clk_Config.h"

#include "LCD_Display.h"

#include <stdio.h>
#include <string.h>

#include "Small_7.h"
#include "Arial_9.h"
#include "Arial_12.h"
#include "Arial_24.h"

#include "stm32f4xx_ll_crc.h"

/*
This program flashes an LED to test the nucleo board, while including the header files and '#defines' that
you will need for the later parts of the lab. Use this project as a template for the remaining tasks of the lab.
*/

//Temperature Sensor I2C Address
#define TEMPADR 0x90

//EEPROM I2C Address
#define EEPROMADR 0xA0
		
int main(void){
	//Init
  SystemClock_Config();/* Configure the system clock to 84.0 MHz */
	SysTick_Config_MCE2(us);	
	
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
	
	//Configure LED: GPIO Port B, pin 4
	LL_GPIO_SetPinMode (GPIOB, LL_GPIO_PIN_4, LL_GPIO_MODE_OUTPUT);
	LL_GPIO_SetPinOutputType (GPIOB, LL_GPIO_PIN_4, LL_GPIO_OUTPUT_PUSHPULL);
	LL_GPIO_SetPinPull (GPIOB, LL_GPIO_PIN_4, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinSpeed (GPIOB, LL_GPIO_PIN_4, LL_GPIO_SPEED_FREQ_HIGH);
	
	while(1){
		LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_4);
		LL_mDelay(500000);
	}
}

