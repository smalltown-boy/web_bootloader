/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "W5500/w5500.h"
#include "socket.h"
#include "httpServer.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APPLICATION_ADDRESS 0x8040000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
wiz_NetInfo gSetNetInfo = {
	    .mac  = {0x00, 0x08, 0xdc, 0x11, 0x11, 0x11},
	    .ip   = {192, 168, 0, 15},
	    .sn   = {255, 255, 0, 0},
	    .dhcp = NETINFO_STATIC };

uint8_t buffer_size_tx_rx[16] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

uint8_t http_rx_buff[2048];
uint8_t http_tx_buff[2048];
uint8_t socknumlist[] = {0,1,2};

const uint8_t webpage[] = {
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "    <title>Example APP</title>\n"
    "    <style>\n"
    "        body {\n"
    "            font-family: Arial, sans-serif;\n"
    "            text-align: center;\n"
    "            padding: 50px;\n"
    "            background-color: #f4f4f4;\n"
    "        }\n"
    "        h1 {\n"
    "            font-size: 24px;\n"
    "            margin: 0;\n"
    "        }\n"
    "        h2 {\n"
    "            margin: 10px 0 30px;\n"
    "        }\n"
    "        .button-container {\n"
    "            margin: 20px 0;\n"
    "        }\n"
    "        .input-container {\n"
    "            margin: 20px 0;\n"
    "        }\n"
    "        .file-input {\n"
    "            margin: 10px 0;\n"
    "        }\n"
    "        button {\n"
    "            display: block;\n"
    "            margin: 10px auto;\n"
    "        }\n"
    "    </style>\n"
    "</head>\n"
    "<body>\n"
    "    <h1>Example APP</h1>\n"
    "    <h2>Welcome to bootloader</h2>\n"
    "    <div class=\"input-container\">\n"
    "        <input type=\"file\" class=\"file-input\">\n"
    "    </div>\n"
    "    <div class=\"button-container\">\n"
    "        <button type=\"button\">Load firmware</button>\n"
    "        <button id=\"exit-button\" type=\"button\">Exit from bootloader</button>\n"
    "    </div>\n"
    "    <script>\n"
    "        document.getElementById('exit-button').addEventListener('click', function() {\n"
    "            fetch('/exit', {\n"
    "                method: 'POST'\n"
    "            })\n"
    "            .then(response => {\n"
    "                if (response.ok) {\n"
    "                    console.log('Command execute');\n"
    "                } else {\n"
    "                    console.error('Error');\n"
    "                }\n"
    "            });\n"
    "        });\n"
    "    </script>\n"
    "</body>\n"
    "</html>"
};

uint8_t recv_data_buf[64];

typedef void (*pFunction)(void);
uint32_t jump_addr;
pFunction jump_to_app;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
void 	cris_enter(void);
void 	cris_exit(void);
void 	cs_select(void);
void 	cs_deselect(void);
uint8_t spi_readbyte(void);
void 	spi_writebyte(uint8_t wb);
void 	spi_readburst(uint8_t* pBuf, uint16_t len);
void 	spi_writeburst(uint8_t* pBuf, uint16_t len);
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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  reg_wizchip_cs_cbfunc(cs_select, cs_deselect);
  reg_wizchip_spi_cbfunc(spi_readbyte, spi_writebyte);
  reg_wizchip_cris_cbfunc(cris_enter, cris_exit);
  reg_wizchip_spiburst_cbfunc(spi_readburst, spi_writeburst);

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
  HAL_Delay(1600);

  ctlnetwork(CN_SET_NETINFO, (void*)&gSetNetInfo);
  ctlwizchip(CW_INIT_WIZCHIP,(void*)buffer_size_tx_rx);

  socket(0,Sn_MR_TCP,5555,SF_TCP_NODELAY | SF_IO_NONBLOCK);

  httpServer_init(http_tx_buff,http_rx_buff,5,socknumlist);
  reg_httpServer_cbfunc(NULL, NULL);
  reg_httpServer_webContent((uint8_t*)"index.html", (uint8_t*)webpage);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_5) == GPIO_PIN_RESET) //Если перемычка установлена
	  {
	  	//Выполняем основной код загрузчика
	  }
	  else
	  {
	  	//В противном случае переходим к коду основной программы
	  }
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : W5500_RST_Pin */
  GPIO_InitStruct.Pin = W5500_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(W5500_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void 	cs_select(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, RESET);
}

void 	cs_deselect(void)
{
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, SET);
}

void 	cris_enter(void)
{
	__set_PRIMASK(1);
}

void 	cris_exit(void)
{
	__set_PRIMASK(0);
}

uint8_t spi_readbyte(void)
{
	uint8_t data;
	HAL_SPI_Receive(&hspi1,&data,1,100);
	return data;
}

void 	spi_writebyte(uint8_t wb)
{
	HAL_SPI_Transmit(&hspi1,&wb,1,100);
}

void 	spi_readburst(uint8_t* pBuf, uint16_t len)
{
	HAL_SPI_Receive(&hspi1, pBuf, len, HAL_MAX_DELAY);
}

void 	spi_writeburst(uint8_t* pBuf, uint16_t len)
{
	HAL_SPI_Transmit(&hspi1, pBuf, len, HAL_MAX_DELAY);
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
