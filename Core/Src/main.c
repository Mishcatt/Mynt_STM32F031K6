/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define LEDS 144
#define FRAMEBUFFER 1296
#define LEDPACKET (FRAMEBUFFER + 64)
#define BUTTONHISTORY 19

enum myntStates {
	MYNT_BOOT,
	MYNT_STATIC,
	MYNT_BLINK,
	MYNT_ALERT,
	MYNT_RACE
};

enum buttons {
	BUTTON_NONE = 0b00000000,
	BUTTON_B = 0b00000001,
	BUTTON_A = 0b00000010,
	BUTTON_RIGHT = 0b00000100,
	BUTTON_LEFT = 0b00001000,
	BUTTON_DOWN = 0b00010000,
	BUTTON_UP = 0b00100000
};

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
 ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_adc;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

TIM_HandleTypeDef htim14;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

uint8_t framebuffer[LEDPACKET];
volatile uint8_t spiBusy = 0;
volatile uint8_t msCounter = 0;

const uint8_t colors[8][3] = {
		{0, 0, 0},		// black
		{63, 63, 63},	// white
		{63, 0, 0},		// red
		{0, 63, 0},		// green
		{0, 0, 127},	// blue
		{63, 0, 63},	// purple
		{0, 63, 63},	// cyan
		{63, 63, 0}		// yellow
};

uint8_t buttonState[6];
uint8_t tempButton = 0;
uint8_t buttonHistory[BUTTONHISTORY];
uint8_t heldButton = 0;
uint32_t buttonTime = 0;

uint32_t adcBuffer[1] = {0};
uint32_t randomNumber = 0;
uint8_t randomBits = 0;

uint8_t myntState = MYNT_BOOT;
uint8_t backgroundColor = 0;
uint8_t foregroundColor = 1;
uint8_t previousColor = 1;
uint8_t currentGraphic = 0;

uint8_t carAnim = 0;
uint8_t curbAnim = 0;
int8_t carPlace[6] = {0, 5, 3, 1, 8, 6};
uint8_t carSide[6] = {1, 0, 2, 0, 1, 2};
int8_t curbAnimation = 0;
uint8_t carExplosionFrame = 0;
uint8_t konamiCode = 0;

uint32_t blinkDelay = 0;
uint32_t blinkAnim = 0;
uint8_t blinkState = 0;

const uint8_t carSprites[12][6] = {
		{8, 9, 12, 27, 28, 31},		// left car
		{6, 7, 14, 25, 26, 33},		// middle car
		{4, 5, 16, 23, 24, 35},		// right car
		{8, 9, 27, 28, 28, 28},		// left car explosion 1
		{6, 7, 25, 26, 26, 26},		// middle car explosion 1
		{4, 5, 23, 24, 24, 24},		// right car explosion 1
		{11, 12, 13, 31, 31, 31},	// left car explosion 2
		{13, 14, 15, 33, 33, 33},	// middle car explosion 2
		{15, 16, 17, 35, 35, 35},	// right car explosion 2
		{12, 12, 12, 12, 12, 12},	// left car explosion 3
		{14, 14, 14, 14, 14, 14},	// middle car explosion 3
		{16, 16, 16, 16, 16, 16}	// right car explosion 3
};

const uint8_t digits[10][7] = {
		{78, 94, 95,  0, 113, 114, 116}, 	// 0
		{78,  0,  0, 97,   0,   0, 116}, 	// 1
		{78, 94,  0, 97,   0, 114, 116},	// 2
		{78,  0, 95, 97,   0, 114, 116},	// 3
		{78,  0,  0, 97, 113,   0, 116},	// 4
		{78,  0, 95, 97, 113,   0, 116},	// 5
		{78, 94, 95, 97, 113,   0, 116},	// 6
		{ 0,  0, 95,  0, 113, 114, 116},	// 7
		{78, 94, 95, 97, 113, 114, 116},	// 8
		{78,  0, 95, 97, 113, 114, 116}		// 9
};

const uint8_t graphics[11][18] = {
		{		// Appul
				0b00000000, 0b00000000, 0b00000000, 0b00000000,
				0b01010000, 0b00111100, 0b00011110, 0b00000011,
				0b10000011, 0b11000000, 0b01110000, 0b01111000,
				0b00011110, 0b00000101, 0b00000001, 0b00000000,
				0b00100000, 0b00000000
		},
		{		// OwO
				0b00000000, 0b00000010, 0b10000001, 0b11100000,
				0b10101000, 0b00100100, 0b00000000, 0b00000000,
				0b00000110, 0b00110010, 0b10010100, 0b00000000,
				0b01010010, 0b10011000, 0b11000000, 0b00000000,
				0b00000000, 0b00000000
		},
		{		// dwd
				0b00000000, 0b00000010, 0b10000001, 0b11100000,
				0b10101000, 0b00100100, 0b00000000, 0b00000000,
				0b00000110, 0b00110010, 0b10010100, 0b00000000,
				0b01110011, 0b10000000, 0b00000000, 0b00000000,
				0b00000000, 0b00000000
		},
		{		// .w.
				0b00000000, 0b00000010, 0b10000001, 0b11100000,
				0b10101000, 0b00100100, 0b00000000, 0b00000000,
				0b00000000, 0b00000011, 0b10011100, 0b00000000,
				0b00000000, 0b00000000, 0b00000000, 0b00000000,
				0b00000000, 0b00000000
		},
		{		// -w-
				0b00000000, 0b00000010, 0b10000001, 0b11100000,
				0b10101000, 0b00100100, 0b00000000, 0b00000000,
				0b00000110, 0b00110010, 0b10010100, 0b00000000,
				0b00000000, 0b00000000, 0b00000000, 0b00000000,
				0b00000000, 0b00000000
		},
		{		// uwu
				0b00000000, 0b00000010, 0b10000001, 0b11100000,
				0b10101000, 0b00100100, 0b00000000, 0b00000000,
				0b00000110, 0b00110010, 0b10010100, 0b00000000,
				0b01010010, 0b10000000, 0b00000000, 0b00000000,
				0b00000000, 0b00000000
		},
		{		// track
				0b01110000, 0b00100000, 0b00000010, 0b00000100,
				0b00000000, 0b01000000, 0b10000000, 0b00001000,
				0b00010000, 0b00000001, 0b00000010, 0b00000000,
				0b00100000, 0b01000000, 0b00000100, 0b00001000,
				0b00000011, 0b10000001
		},
		{		// ^w^
				0b00000000, 0b00000010, 0b10000001, 0b11100000,
				0b10101000, 0b00100100, 0b00000000, 0b00000000,
				0b00000000, 0b00000010, 0b10010100, 0b11000110,
				0b00100001, 0b00000000, 0b00000000, 0b00000000,
				0b00000000, 0b00000000
		},
		{		// ok
				0b00000000, 0b00000000, 0b00000000, 0b01000000,
				0b01100000, 0b00011100, 0b00011110, 0b00000110,
				0b11000010, 0b01100001, 0b10000000, 0b00000110,
				0b01100000, 0b00000000, 0b01000000, 0b00000000,
				0b00000000, 0b00000000
		},
		{		// nok
				0b00000000, 0b00000000, 0b00000010, 0b00010001,
				0b10001100, 0b01100110, 0b00011011, 0b00000111,
				0b10000001, 0b11000000, 0b11110000, 0b01101100,
				0b00110011, 0b00011000, 0b11000100, 0b00100000,
				0b00000000, 0b00000000
		},
		{		// serduszko
				0b00000000, 0b00000001, 0b00000000, 0b11000000,
				0b01110000, 0b00111100, 0b00011111, 0b00001111,
				0b11000111, 0b11110011, 0b11111100, 0b11111110,
				0b01111111, 0b10011101, 0b11000110, 0b01100000,
				0b00000000, 0b00000000
		}

};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM14_Init(void);
static void MX_ADC_Init(void);
/* USER CODE BEGIN PFP */

void setPixelColor(uint16_t p, uint8_t r, uint8_t g, uint8_t b);
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void SPI_SetOpenDrain();

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
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_TIM14_Init();
  MX_ADC_Init();
  /* USER CODE BEGIN 2 */

  for (uint16_t i=0; i<FRAMEBUFFER; i+=9) {
	  framebuffer[i+0] = 0b00100100; // G
	  framebuffer[i+1] = 0b10010010;
	  framebuffer[i+2] = 0b01001001;
	  framebuffer[i+3] = 0b00100100; // R
	  framebuffer[i+4] = 0b10010010;
	  framebuffer[i+5] = 0b01001001;
	  framebuffer[i+6] = 0b00100100; // B
	  framebuffer[i+7] = 0b10010010;
	  framebuffer[i+8] = 0b01001001;
  }

  HAL_TIM_Base_Start_IT(&htim14);
  HAL_ADC_Start_DMA(&hadc, adcBuffer, 1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  if (HAL_GPIO_ReadPin(PadUp_GPIO_Port, PadUp_Pin) == GPIO_PIN_RESET) {
		  if (buttonState[0] < 255) buttonState[0]++;
	  }	else {
		  if (buttonState[0] > 0) buttonState[0]--;
	  }
	  if (HAL_GPIO_ReadPin(PadDown_GPIO_Port, PadDown_Pin) == GPIO_PIN_RESET) {
		  if (buttonState[1] < 255) buttonState[1]++;
	  }	else {
		  if (buttonState[1] > 0) buttonState[1]--;
	  }
	  if (HAL_GPIO_ReadPin(PadLeft_GPIO_Port, PadLeft_Pin) == GPIO_PIN_RESET) {
		  if (buttonState[2] < 255) buttonState[2]++;
	  }	else {
		  if (buttonState[2] > 0) buttonState[2]--;
	  }
	  if (HAL_GPIO_ReadPin(PadRight_GPIO_Port, PadRight_Pin) == GPIO_PIN_RESET) {
		  if (buttonState[3] < 255) buttonState[3]++;
	  }	else {
		  if (buttonState[3] > 0) buttonState[3]--;
	  }
	  if (HAL_GPIO_ReadPin(PadA_GPIO_Port, PadA_Pin) == GPIO_PIN_RESET) {
		  if (buttonState[4] < 255) buttonState[4]++;
	  }	else {
		  if (buttonState[4] > 0) buttonState[4]--;
	  }
	  if (HAL_GPIO_ReadPin(PadB_GPIO_Port, PadB_Pin) == GPIO_PIN_RESET) {
		  if (buttonState[5] < 255) buttonState[5]++;
	  }	else {
		  if (buttonState[5] > 0) buttonState[5]--;
	  }

	  tempButton = BUTTON_NONE;
	  if (buttonState[0] > 127) { // Up
		  tempButton |= BUTTON_UP;
	  }
	  if (buttonState[1] > 127) { // Down
		  tempButton |= BUTTON_DOWN;
	  }
	  if (buttonState[2] > 127) { // Left
		  tempButton |= BUTTON_LEFT;
	  }
	  if (buttonState[3] > 127) { // Right
		  tempButton |= BUTTON_RIGHT;
	  }
	  if (buttonState[4] > 127) { // A
		  tempButton |= BUTTON_A;
	  }
	  if (buttonState[5] > 127) { // B
		  tempButton |= BUTTON_B;
	  }
	  if (buttonHistory[0] != tempButton) {
		  for (uint8_t i=(BUTTONHISTORY-1); i>0; i--) {
			  buttonHistory[i] = buttonHistory[i-1];
		  }
		  buttonHistory[0] = tempButton;
	  }
	  if (buttonHistory[18] == BUTTON_UP && buttonHistory[17] == BUTTON_NONE &&
		  buttonHistory[16] == BUTTON_UP && buttonHistory[15] == BUTTON_NONE &&
		  buttonHistory[14] == BUTTON_DOWN && buttonHistory[13] == BUTTON_NONE &&
		  buttonHistory[12] == BUTTON_DOWN && buttonHistory[11] == BUTTON_NONE &&
		  buttonHistory[10] == BUTTON_LEFT && buttonHistory[9] == BUTTON_NONE &&
		  buttonHistory[8] == BUTTON_RIGHT && buttonHistory[7] == BUTTON_NONE &&
		  buttonHistory[6] == BUTTON_LEFT && buttonHistory[5] == BUTTON_NONE &&
		  buttonHistory[4] == BUTTON_RIGHT && buttonHistory[3] == BUTTON_NONE &&
		  buttonHistory[2] == BUTTON_B && buttonHistory[1] == BUTTON_NONE &&
		  buttonHistory[0] == BUTTON_A) {
		  konamiCode = 1;
		  //currentGraphic = 6;
	  }

	  if (msCounter) {
		  msCounter--;
		  if (currentGraphic == 6) {
			  carAnim++;
			  curbAnim++;
		  } else {
			  blinkAnim++;
		  }
		  if (heldButton == tempButton) {
			  buttonTime++;
		  } else {
			  buttonTime = 0;
			  heldButton = tempButton;
			  if (currentGraphic == 6) {
				  if (tempButton == BUTTON_LEFT) {
					  if (carSide[0] > 0) carSide[0]--;
				  }
				  if (tempButton == BUTTON_RIGHT) {
					  if (carSide[0] < 2) carSide[0]++;
				  }
			  } else {
				  if (tempButton == (BUTTON_A | BUTTON_UP)) {
					  currentGraphic = 8;
					  if (myntState != MYNT_ALERT) previousColor = foregroundColor;
					  foregroundColor = 3;
					  myntState = MYNT_ALERT;
				  }
				  if (tempButton == (BUTTON_A | BUTTON_DOWN)) {
					  currentGraphic = 9;
					  if (myntState != MYNT_ALERT) previousColor = foregroundColor;
					  foregroundColor = 2;
					  myntState = MYNT_ALERT;
				  }
				  if (tempButton == (BUTTON_A | BUTTON_LEFT)) {
					  currentGraphic = 10;
					  if (myntState != MYNT_ALERT) previousColor = foregroundColor;
					  foregroundColor = 5;
					  myntState = MYNT_ALERT;
				  }
				  if (tempButton == (BUTTON_A | BUTTON_RIGHT)) {
					  foregroundColor++;
					  if (foregroundColor > 7) foregroundColor = 1;
				  }
				  if (tempButton == BUTTON_LEFT) {
					  currentGraphic = 1;
					  blinkDelay = 0;
					  blinkAnim = 0;
					  if (myntState == MYNT_ALERT) foregroundColor = previousColor;
					  myntState = MYNT_BLINK;
				  }
				  if (tempButton == BUTTON_RIGHT) {
					  currentGraphic = 2;
					  if (myntState == MYNT_ALERT) foregroundColor = previousColor;
					  myntState = MYNT_STATIC;
				  }
				  if (tempButton == BUTTON_UP) {
					  currentGraphic = 7;
					  if (myntState == MYNT_ALERT) foregroundColor = previousColor;
					  myntState = MYNT_STATIC;
				  }
				  if (tempButton == BUTTON_DOWN) {
					  currentGraphic = 5;
					  if (myntState == MYNT_ALERT) foregroundColor = previousColor;
					  myntState = MYNT_STATIC;
				  }
			  }


		  }
		  if (buttonTime > 100) {
			  if (tempButton == 0b00111101) {
				  currentGraphic = 0;
				  myntState = MYNT_STATIC;
			  }
			  if (tempButton == 0b00110001) {
				  currentGraphic = 1;
				  myntState = MYNT_BLINK;
			  }
			  if (tempButton == 0b00001101) {
				  currentGraphic = 6;
				  myntState = MYNT_RACE;
			  }
		  }
	  }

	  if (spiBusy == 0) {

		  for (uint8_t i=0; i<LEDS; i++) {
			  if (graphics[currentGraphic][i/8] & (0x80 >> i%8)) {
				  setPixelColor(i, colors[foregroundColor][0], colors[foregroundColor][1], colors[foregroundColor][2]);
			  } else {
				  setPixelColor(i, colors[backgroundColor][0], colors[backgroundColor][1], colors[backgroundColor][2]);
				  //setPixelColor(i, colors[blinkAnim%8][0], colors[blinkAnim%8][1], colors[blinkAnim%8][2]);
			  }
		  }

		  if (currentGraphic == 6) {
			  if (curbAnim > 9) {
				  curbAnim -= 10;
				  curbAnimation--;
				  if (curbAnimation < 0) curbAnimation = 3;
			  }

			  if (carAnim > 59) {
				  carAnim -= 60;

				  for (uint8_t car=1; car<6; car++) {
					  carPlace[car]--;
					  if (carPlace[car] < -2) carPlace[car] = 7;
				  }
			  }
			  for (uint8_t car=1; car<6; car++) {
				  for (uint8_t i=0; i<6; i++) {
					  setPixelColor((carSprites[carSide[car]][i]+(19*carPlace[car])), colors[3][0], colors[3][1], colors[3][2]);
				  }
			  }
			  for (uint8_t i=0; i<6; i++) {
				  setPixelColor((carSprites[carSide[0]+carExplosionFrame][i]+(19*carPlace[0])), colors[3][0], colors[3][1], colors[3][2]);
			  }
			  for (uint8_t p=0; p<2; p++) { // TODO
				  setPixelColor(3+(19*curbAnimation)+(19*p*4), colors[backgroundColor][0], colors[backgroundColor][1], colors[backgroundColor][2]);
				  setPixelColor(10+(19*curbAnimation)+(19*p*4), colors[backgroundColor][0], colors[backgroundColor][1], colors[backgroundColor][2]);
				  setPixelColor(3+(19*(curbAnimation+1))+(19*p*4), colors[backgroundColor][0], colors[backgroundColor][1], colors[backgroundColor][2]);
				  setPixelColor(10+(19*(curbAnimation+1))+(19*p*4), colors[backgroundColor][0], colors[backgroundColor][1], colors[backgroundColor][2]);
			  }
		  } else {
			  if (myntState == MYNT_BLINK) {
				  switch (blinkState) {
				  case 0:
					  if (blinkAnim > blinkDelay) {
						  currentGraphic = 2;
						  blinkState = 1;
						  blinkAnim -= blinkDelay;
						  blinkDelay = 8;
					  }
					  break;
				  case 1:
					  if (blinkAnim > blinkDelay) {
						  currentGraphic = 3;
						  blinkState = 2;
						  blinkAnim -= blinkDelay;
						  blinkDelay = 16;
					  }
					  break;
				  case 2:
					  if (blinkAnim > blinkDelay) {
						  currentGraphic = 4;
						  blinkState = 3;
						  blinkAnim -= blinkDelay;
						  blinkDelay = 8;
					  }
				  case 3:
					  if (blinkAnim > blinkDelay) {
						  currentGraphic = 5;
						  blinkState = 4;
						  blinkAnim -= blinkDelay;
						  blinkDelay = 8;
					  }
				  case 4:
					  if (blinkAnim > blinkDelay) {
						  currentGraphic = 1;
						  blinkState = 0;
						  blinkAnim -= blinkDelay;
						  blinkDelay = (randomNumber & 0x3FF);
					  }
					  break;
				  default:
					  break;
				  }
			  }
		  }

		  /*if (tempButton & BUTTON_UP) setPixelColor(1, 63, 0, 0);
		  if (tempButton & BUTTON_DOWN) setPixelColor(20, 63, 0, 0);
		  if (tempButton & BUTTON_LEFT) setPixelColor(39, 63, 0, 0);
		  if (tempButton & BUTTON_RIGHT) setPixelColor(58, 63, 0, 0);
		  if (tempButton & BUTTON_A) setPixelColor(77, 63, 0, 0);
		  if (tempButton & BUTTON_B) setPixelColor(96, 63, 0, 0);*/

		  spiBusy = 1;
		  HAL_SPI_Transmit_DMA(&hspi1, framebuffer, LEDPACKET);
	  }

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL5;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = ENABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = ENABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM14 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM14_Init(void)
{

  /* USER CODE BEGIN TIM14_Init 0 */

  /* USER CODE END TIM14_Init 0 */

  /* USER CODE BEGIN TIM14_Init 1 */

  /* USER CODE END TIM14_Init 1 */
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 200;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 1000;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM14_Init 2 */

  /* USER CODE END TIM14_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 38400;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

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
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PadA_Pin PadB_Pin */
  GPIO_InitStruct.Pin = PadA_Pin|PadB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : PadUp_Pin PadRight_Pin PadLeft_Pin */
  GPIO_InitStruct.Pin = PadUp_Pin|PadRight_Pin|PadLeft_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PadDown_Pin */
  GPIO_InitStruct.Pin = PadDown_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(PadDown_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

inline void setPixelColor(uint16_t p, uint8_t r, uint8_t g, uint8_t b) {
	if (p > LEDS) return;

	uint32_t er = 0;
	er |= ((r&0b10000000)<<15);
	er |= ((r&0b01000000)<<13);
	er |= ((r&0b00100000)<<11);
	er |= ((r&0b00010000)<<9);
	er |= ((r&0b00001000)<<7);
	er |= ((r&0b00000100)<<5);
	er |= ((r&0b00000010)<<3);
	er |= ((r&0b00000001)<<1);

	uint32_t eg = 0;
	eg |= ((g&0b10000000)<<15);
	eg |= ((g&0b01000000)<<13);
	eg |= ((g&0b00100000)<<11);
	eg |= ((g&0b00010000)<<9);
	eg |= ((g&0b00001000)<<7);
	eg |= ((g&0b00000100)<<5);
	eg |= ((g&0b00000010)<<3);
	eg |= ((g&0b00000001)<<1);

	uint32_t eb = 0;
	eb |= ((b&0b10000000)<<15);
	eb |= ((b&0b01000000)<<13);
	eb |= ((b&0b00100000)<<11);
	eb |= ((b&0b00010000)<<9);
	eb |= ((b&0b00001000)<<7);
	eb |= ((b&0b00000100)<<5);
	eb |= ((b&0b00000010)<<3);
	eb |= ((b&0b00000001)<<1);

	uint16_t i = p*9;
	framebuffer[i+0] = 0b00100100 | ((eg>>16)&0xFF);
	framebuffer[i+1] = 0b10010010 | ((eg>>8)&0xFF);
	framebuffer[i+2] = 0b01001001 | ((eg)&0xFF);
	framebuffer[i+3] = 0b00100100 | ((er>>16)&0xFF);
	framebuffer[i+4] = 0b10010010 | ((er>>8)&0xFF);
	framebuffer[i+5] = 0b01001001 | ((er)&0xFF);
	framebuffer[i+6] = 0b00100100 | ((eb>>16)&0xFF);
	framebuffer[i+7] = 0b10010010 | ((eb>>8)&0xFF);
	framebuffer[i+8] = 0b01001001 | ((eb)&0xFF);

}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
	spiBusy = 0;
	//HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	msCounter++;
	//if (periodCounter > 100) periodCounter = 0;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	static uint32_t randomNumberBuffer = 0;
	static uint8_t randomShift = 0;
	static uint8_t randomBitsBuffer = 0;

	if (adcBuffer[0] & 1) {
		randomNumberBuffer |= 1 << randomShift;
		randomBitsBuffer++;
	}
	randomShift++;

	if (randomShift > 31) {
		randomNumber = randomNumberBuffer;
		randomBits = randomBitsBuffer;
		randomNumberBuffer = 0;
		randomShift = 0;
		randomBitsBuffer = 0;
	}
}

void SPI_SetOpenDrain() { // Added to stm32f0xx_hal_msp.c
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin = SPI1_MOSI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
	HAL_GPIO_Init(SPI1_MOSI_GPIO_Port, &GPIO_InitStruct);
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
