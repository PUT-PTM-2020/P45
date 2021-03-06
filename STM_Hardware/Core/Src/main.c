/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ff.h"
#include "CANSPI.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SD_CS_HIGH()   HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET)
#define SD_CS_LOW()    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim4;
DMA_HandleTypeDef hdma_tim4_ch1;

/* USER CODE BEGIN PV */
char buffer[256]; //bufor odczytu i zapisu
static FATFS FatFs; //uchwyt do urządzenia FatFs (dysku, karty SD...)
FRESULT fresult; //do przechowywania wyniku operacji na bibliotece

FIL file; //uchwyt do otwartego pliku
WORD bytes_written; //liczba zapisanych byte
WORD bytes_read; //liczba odczytanych byte

char filenameS[128];
char* filename = &filenameS;

uint8_t startMeasure = 0;
uint8_t fileOpen = 0;

uint32_t rpmCounter = 0;

uint8_t getOBD = 0;

uint32_t time = 0;

uCAN_MSG txMessage;
uCAN_MSG rxMessage;

uint32_t canReply[2]; // 0 - RPM, 1 - SPEED

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */
char* createNewFilenameTS();
void createNewFilename(char *filename);

void openFileToWriteRPM(char* filename) {
	SD_CS_LOW();
	fresult = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
	fresult = f_lseek(&file, f_size(&file));
	SD_CS_HIGH();
}

void closeFileToWriteRPM(char* filename) {
	SD_CS_LOW();
	fresult = f_close (&file);
	SD_CS_HIGH();
}

void writeRpmToFile(uint32_t impSens, uint32_t rpmEng, uint32_t vehSpeed) {
	//uint32_t timestamp = time;
	impSens = impSens * 5 * 60 / 20;
	uint32_t len = sprintf( buffer, "%d;%d;%d;%d\n", time, impSens, rpmEng, vehSpeed);
	SD_CS_LOW();
	fresult = f_write(&file, buffer, len, &bytes_written);
	SD_CS_HIGH();
	time+=2;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)==1) {
		HAL_Delay(100);
		if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)==1) {
			if(startMeasure==0) {
				fresult = f_mount(&FatFs, "", 0);
				filename = createNewFilenameTS();
				createNewFilename(filename);
				openFileToWriteRPM(filename);
				HAL_TIM_Base_Start_IT(&htim4);

				startMeasure=1;
				rpmCounter = 0;
				getOBD = 0;
				time = 0;
			} else {
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
				HAL_TIM_Base_Stop_IT(&htim4);
				startMeasure=0;
				closeFileToWriteRPM(filename);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
			}
		}
	}
	if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0)==0) {
		if(startMeasure==1) {
			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
			rpmCounter++;
		}
	}
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM4)
	{
		if(startMeasure==1) {
			uint32_t rpmTemp = rpmCounter;
			rpmCounter = 0;
			writeRpmToFile(rpmTemp, canReply[0], canReply[1]);
			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
			getOBD = 0;
		}
	}
}

/*
 * ; Speed
[transmit1]                  ; Transmit message
destination = 0       	     ; 0 = None, 1 = Logger, 2 = Interface, 3 = Both
period = 200                 ; Period in ms (DEC, 10 ms resolution)
delay = 0                    ; Delay in ms (DEC, 10 ms resolution)
extendedID = false           ; Use extended 29 bit message IDs (2.0B)
msgID = 7DF                  ; Transmit message ID (HEX)
msgData = {02010D5555555555} ; Message data (HEX)


; RPM
[transmit2]            	     ; Transmit message
destination = 0              ; 0 = None, 1 = Logger, 2 = Interface, 3 = Both
period = 200                 ; Period in ms (DEC, 10 ms resolution)
delay = 10                   ; Delay in ms (DEC, 10 ms resolution)
extendedID = false           ; Use extended 29 bit message IDs (2.0B)
msgID = 7DF                  ; Transmit message ID (HEX)
msgData = {02010C5555555555} ; Message data (HEX)

SOURCE https://www.csselectronics.com/screen/page/obd-ii-pid-examples/language/en
 */

void getRPMByOBD_Req() {
	canReply[0] = 0; // Reset RPM
	txMessage.frame.idType = dSTANDARD_CAN_MSG_ID_2_0B; //??
	txMessage.frame.id = 0x7DF; //Request ID
	txMessage.frame.dlc = 8; //??
	txMessage.frame.data0 = 0x02;
	txMessage.frame.data1 = 0x01;
	txMessage.frame.data2 = 0x0C;
	txMessage.frame.data3 = 0x55;
	txMessage.frame.data4 = 0x55;
	txMessage.frame.data5 = 0x55;
	txMessage.frame.data6 = 0x55;
	txMessage.frame.data7 = 0x55;
	CANSPI_Transmit(&txMessage);
}

void getSPEEDByOBD_Req() {
	canReply[1] = 0; // Reset SPEED
	txMessage.frame.idType = dSTANDARD_CAN_MSG_ID_2_0B; //??
	txMessage.frame.id = 0x7DF; //Request ID
	txMessage.frame.dlc = 8; //??
	txMessage.frame.data0 = 0x02;
	txMessage.frame.data1 = 0x01;
	txMessage.frame.data2 = 0x0D;
	txMessage.frame.data3 = 0x55;
	txMessage.frame.data4 = 0x55;
	txMessage.frame.data5 = 0x55;
	txMessage.frame.data6 = 0x55;
	txMessage.frame.data7 = 0x55;
	CANSPI_Transmit(&txMessage);
}

void getReplyByOBD_Cont() {
	bool getRPM = false;
	bool getSPEED = false;
	if(canReply[0]==0) {
		getRPM = true;
	}
	if(canReply[1]==0) {
		getSPEED = true;
	}
	while(1) {
		getReplyByOBD();
		if(getRPM&&canReply[0]!=0) {
			break;
		}
		if(getSPEED&&canReply[1]!=0) {
			break;
		}
	}
}

void getReplyByOBD() { //TODO: return value
	if(CANSPI_Receive(&rxMessage))
	{
	  if(rxMessage.frame.id==0x7E8) {
		  if(rxMessage.frame.data0==0x03 && rxMessage.frame.data1==0x41 && rxMessage.frame.data2==0x0C) { //Reply 03,41,0C - Reply RPM
			  canReply[0] = (rxMessage.frame.data3*256 + rxMessage.frame.data4)/4;
		  }
		  if(rxMessage.frame.data0==0x02 && rxMessage.frame.data1==0x41 && rxMessage.frame.data2==0x0D) { //Reply 02,41,0D - Reply SPEED
			  canReply[1] = rxMessage.frame.data3;
		  }
	  }
	}
}


uint32_t getNextNumber() {
	FRESULT fr;
	FILINFO fno;
	uint32_t nextNum = 0;

	SD_CS_LOW();
	fr = f_stat("measureNumber.txt", &fno);
	if(fr==FR_OK) {
		fresult = f_open(&file, "measureNumber.txt", FA_READ);
		fresult = f_read(&file, buffer, 16, &bytes_read);
		nextNum = atoi(bytes_read);
		fresult = f_close(&file);
	} else if(fr==FR_NO_FILE) {
	    fresult = f_open(&file, "measureNumber.txt", FA_OPEN_ALWAYS | FA_WRITE);
	    uint32_t len = sprintf( buffer, "1");
	    fresult = f_write(&file, buffer, len, &bytes_written);
	    nextNum = 1;
	    fresult = f_close (&file);
	} else {
		//Error handler
	}
	SD_CS_HIGH();
	return nextNum;
}
char* createNewFilenameTS() {
	RTC_TimeTypeDef currentTime;
	RTC_DateTypeDef currentDate;
	//time_t timestamp;
	char *ret = malloc(128*sizeof(char));

	HAL_RTC_GetTime(&hrtc, &currentTime, FORMAT_BIN); //first
	HAL_RTC_GetDate(&hrtc, &currentDate, FORMAT_BIN); //second, even if you dont required

	sprintf(ret, "measure_%d_%d_%d__%d_%d_%d.txt\0",
			currentDate.Year,
			currentDate.Month,
			currentDate.Date,
			currentTime.Hours,
			currentTime.Minutes,
			currentTime.Seconds);

	FRESULT fr;
	FILINFO fno;
	uint8_t addToEnd = 1;

	//fresult = f_mount(&FatFs, "", 0);

	while(1) {
		SD_CS_LOW();
		fr = f_stat(ret, &fno);
		SD_CS_HIGH();
		if(fr==FR_OK) {
			//Change name
			uint32_t len = strlen(ret);
			if(addToEnd>2) {
				ret[len-7] = '\0'; //TODO: verify
			} else {
				ret[len-4] = '\0'; //TODO: verify
			}
			addToEnd++;
			sprintf(ret, "%s__%d.txt", ret, addToEnd);

			/*
			 * 0 1 2 3 4 5 6 7
			 * a b c d e f g 0
			 * 1 2 3 4 5 6 7 8
			 */
		} else if(fr==FR_NO_FILE) {
			//Ok
			break;
		} else {
			//Error handler
		}
	}

	return ret;

	/*
	 * When calling this func, remember to free pointer afterward
	 * free(ptr);
	 */

	/*
	currTime.tm_year = currentDate.Year + 100;  // In fact: 2000 + 18 - 1900
	currTime.tm_mday = currentDate.Date;
	currTime.tm_mon  = currentDate.Month - 1;

	currTime.tm_hour = currentTime.Hours;
	currTime.tm_min  = currentTime.Minutes;
	currTime.tm_sec  = currentTime.Seconds;
	*/


}

void createNewFilename(char *filename) {
	SD_CS_LOW();
	fresult = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
	uint32_t len = sprintf( buffer, "time;rpm_wheel;rpm_engine\n");
	fresult = f_write(&file, buffer, len, &bytes_written);
	fresult = f_close (&file);
	SD_CS_HIGH();
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_RTC_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  CANSPI_Initialize();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  SD_CS_LOW();
  //HAL_GPIO_WritePin(GPIOF, GPIO_PIN_14, GPIO_PIN_RESET); //CS for SD
  fresult = f_mount(&FatFs, "", 0);
  SD_CS_HIGH();
  /*
   * fresult = f_mount(&FatFs, "", 0);
  fresult = f_open(&file, "test.txt", FA_OPEN_ALWAYS | FA_WRITE);
  int len = sprintf( buffer, "Test file\r\n");
  fresult = f_write(&file, buffer, len, &bytes_written);
  fresult = f_close (&file);
   */



  /*
  HAL_Delay(500);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);

  fresult = f_mount(&FatFs, "", 0);
  fresult = f_open(&file, "write.txt", FA_OPEN_ALWAYS | FA_WRITE);
  int len = sprintf( buffer, "Hello PTM!\r\n");
  fresult = f_write(&file, buffer, len, &bytes_written);
  fresult = f_close (&file);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
	*/

  while (1)
  {
	  if(startMeasure==1) {
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
		  if(getOBD==0) {
			  getRPMByOBD_Req();
			  getReplyByOBD_Cont();
			  getSPEEDByOBD_Req();
			  getReplyByOBD_Cont();
			  getOBD = 1;
		  }
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
	  }


	  /*if(HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_0)==GPIO_PIN_RESET) {
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
	  } else {
		  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	  }*/
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only 
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 3199;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 999;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim4, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(CAN_CS_GPIO_Port, CAN_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RPM_Sensor_IN_Pin */
  GPIO_InitStruct.Pin = RPM_Sensor_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(RPM_Sensor_IN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_MDC_Pin */
  GPIO_InitStruct.Pin = RMII_MDC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(RMII_MDC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_REF_CLK_Pin RMII_MDIO_Pin */
  GPIO_InitStruct.Pin = RMII_REF_CLK_Pin|RMII_MDIO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_TXD1_Pin */
  GPIO_InitStruct.Pin = RMII_TXD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(RMII_TXD1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : STLK_RX_Pin STLK_TX_Pin */
  GPIO_InitStruct.Pin = STLK_RX_Pin|STLK_TX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : CAN_CS_Pin */
  GPIO_InitStruct.Pin = CAN_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CAN_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_SOF_Pin USB_ID_Pin USB_DM_Pin USB_DP_Pin */
  GPIO_InitStruct.Pin = USB_SOF_Pin|USB_ID_Pin|USB_DM_Pin|USB_DP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_VBUS_Pin */
  GPIO_InitStruct.Pin = USB_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RMII_TX_EN_Pin RMII_TXD0_Pin */
  GPIO_InitStruct.Pin = RMII_TX_EN_Pin|RMII_TXD0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
