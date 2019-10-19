/*
  ******************************************************************************
  * @file           : fatfs_platform.h
  * @brief          : fatfs_platform header file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
*/

#include "stm32h7xx_hal.h"           


#define SD_PRESENT               ((uint8_t)0x01)
#define SD_NOT_PRESENT           ((uint8_t)0x00)
#define SD_DETECT_Pin 			GPIO_PIN_2
#define SD_DETECT_GPIO_Port 	GPIOG


uint8_t	SDCardIsDetected(void);
