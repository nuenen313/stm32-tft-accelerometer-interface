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
#include "usb_host.h"
#include "LSM6DS3.h"
#include <math.h>
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fonts.h"
#include "z_displ_ILI9XXX.h"
#include "z_displ_ILI9XXX_test.h"
#include "z_touch_XPT2046.h"
#include "z_touch_XPT2046_test.h"
#include "z_touch_XPT2046_menu.h"
extern int16_t _width;
extern int16_t _height;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUTTON_W 120
#define BUTTON_H 64
#define BUTTON_X ((_width - BUTTON_W)/2)
#define BUTTON_Y ((_height - BUTTON_H)/2)
#define PI 3.14159f
#define PLOT_X 10
#define PLOT_Y 30
#define PLOT_WIDTH (_width - 20)
#define PLOT_HEIGHT 150
#define MAX_PLOT_POINTS 100
#define PLOT_SCALE 2.0f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s2;
I2S_HandleTypeDef hi2s3;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi4;
DMA_HandleTypeDef hdma_spi4_tx;

TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S2_Init(void);
static void MX_I2S3_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI4_Init(void);
static void MX_TIM3_Init(void);
static void MX_CRC_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static bool grid_needs_redraw = true;
uint16_t x_touch;
uint16_t y_touch;
uint16_t z_touch;
char text[30];
uint32_t touchTime=0, touchDelay=0;
uint16_t px=0,py,npx,npy;
uint8_t isTouch;
uint32_t lastUpdateTime = 0;
uint32_t updateInterval = 2000;
int screen_state = 0;
static float x_data[MAX_PLOT_POINTS];
static int plot_index = 0;
static bool plot_buffer_full = false;

void initPlot() {
    // Initialize plot data
    for (int i = 0; i < MAX_PLOT_POINTS; i++) {
        x_data[i] = 0.0f;
    }
    plot_index = 0;
    plot_buffer_full = false;
    grid_needs_redraw = true;  // Force grid redraw on init
}

void drawPlot() {
    // Only redraw grid if needed
    if (grid_needs_redraw) {
        drawPlotGrid();
        grid_needs_redraw = false;
    } else {
        // Just clear the plot data area (not the grid)
        Displ_FillArea(PLOT_X + 1, PLOT_Y + 1, PLOT_WIDTH - 2, PLOT_HEIGHT - 2, BLACK);
    }

    // Calculate how many points to draw
    int points_to_draw = plot_buffer_full ? MAX_PLOT_POINTS : plot_index;
    if (points_to_draw < 2) return;

    // Calculate pixel spacing
    float x_step = (float)PLOT_WIDTH / (MAX_PLOT_POINTS - 1);

    // Draw the plot line
    for (int i = 0; i < points_to_draw - 1; i++) {
        // Get data points
        int current_idx = plot_buffer_full ? (plot_index + i) % MAX_PLOT_POINTS : i;
        int next_idx = plot_buffer_full ? (plot_index + i + 1) % MAX_PLOT_POINTS : i + 1;

        float current_val = x_data[current_idx];
        float next_val = x_data[next_idx];

        // Convert to screen coordinates
        int x1 = PLOT_X + (int)(i * x_step);
        int x2 = PLOT_X + (int)((i + 1) * x_step);

        // Convert acceleration to Y coordinate (invert Y axis for display)
        int y1 = PLOT_Y + PLOT_HEIGHT/2 - (int)((current_val / PLOT_SCALE) * (PLOT_HEIGHT/2));
        int y2 = PLOT_Y + PLOT_HEIGHT/2 - (int)((next_val / PLOT_SCALE) * (PLOT_HEIGHT/2));

        // Clamp Y coordinates to plot area
        if (y1 < PLOT_Y) y1 = PLOT_Y;
        if (y1 > PLOT_Y + PLOT_HEIGHT) y1 = PLOT_Y + PLOT_HEIGHT;
        if (y2 < PLOT_Y) y2 = PLOT_Y;
        if (y2 > PLOT_Y + PLOT_HEIGHT) y2 = PLOT_Y + PLOT_HEIGHT;

        // Draw line segment
        Displ_Line(x1, y1, x2, y2, ORANGE);

        // Draw current point
        Displ_fillCircle(x1, y1, 1, YELLOW);
    }

    // Draw the latest point in red
    if (points_to_draw > 0) {
        int latest_idx = plot_buffer_full ? (plot_index - 1 + MAX_PLOT_POINTS) % MAX_PLOT_POINTS : plot_index - 1;
        if (latest_idx >= 0) {
            float latest_val = x_data[latest_idx];
            int latest_x = PLOT_X + (int)((points_to_draw - 1) * x_step);
            int latest_y = PLOT_Y + PLOT_HEIGHT/2 - (int)((latest_val / PLOT_SCALE) * (PLOT_HEIGHT/2));

            // Clamp Y coordinate
            if (latest_y < PLOT_Y) latest_y = PLOT_Y;
            if (latest_y > PLOT_Y + PLOT_HEIGHT) latest_y = PLOT_Y + PLOT_HEIGHT;

            Displ_fillCircle(latest_x, latest_y, 2, DD_RED);
        }
    }
}
void addPlotPoint(float x_accel) {
    // Add new data point
    x_data[plot_index] = x_accel;
    plot_index++;

    if (plot_index >= MAX_PLOT_POINTS) {
        plot_index = 0;
        plot_buffer_full = true;
    }
}
void drawPlotGrid() {
    // Draw plot background - use FillArea instead of fillRect
    Displ_FillArea(PLOT_X, PLOT_Y, PLOT_WIDTH, PLOT_HEIGHT, BLACK);

    // Draw border using Border function
    Displ_Border(PLOT_X, PLOT_Y, PLOT_WIDTH, PLOT_HEIGHT, 1, WHITE);

    // Draw center line (0g) using horizontal line
    int center_y = PLOT_Y + PLOT_HEIGHT / 2;
    for (int x = PLOT_X; x < PLOT_X + PLOT_WIDTH; x++) {
        Displ_Pixel(x, center_y, WHITE);
    }

    // Draw +1g line
    int plus_1g_y = PLOT_Y + PLOT_HEIGHT / 4;
    for (int x = PLOT_X; x < PLOT_X + PLOT_WIDTH; x += 4) {  // Dotted line
        Displ_Pixel(x, plus_1g_y, WHITE);
    }

    // Draw -1g line
    int minus_1g_y = PLOT_Y + (3 * PLOT_HEIGHT) / 4;
    for (int x = PLOT_X; x < PLOT_X + PLOT_WIDTH; x += 4) {  // Dotted line
        Displ_Pixel(x, minus_1g_y, WHITE);
    }

    // Draw scale labels using WString
    Displ_WString(PLOT_X - 25, plus_1g_y - 8, "+1g", Font12, 1, WHITE, DDDD_WHITE);
    Displ_WString(PLOT_X - 20, center_y - 8, "0g", Font12, 1, WHITE, DDDD_WHITE);
    Displ_WString(PLOT_X - 25, minus_1g_y - 8, "-1g", Font12, 1, WHITE, DDDD_WHITE);
}
void drawLiveDataWithPlot() {
        static bool first_run = true;
        if (first_run) {
            Displ_CLS(DDDD_WHITE);
            first_run = false;
        } else {
            // Just clear the title area
            Displ_FillArea(0, 0, _width, PLOT_Y - 5, DDDD_WHITE);
        }

        // Title
        Displ_WString(10, 5, "LSM6DS3 X-Axis Plot", Font16, 1, BLACK, DDDD_WHITE);

        // Read sensor data
        LSM6DS3_AccelData accel_data = LSM6DS3_read_acceleration(hi2c1);

        if (accel_data.status == 1) {
            // Add data point to plot
            addPlotPoint(accel_data.x);

            // Draw the plot
            drawPlot();

            // Clear and redraw text area only
            int text_y = PLOT_Y + PLOT_HEIGHT + 15;
            Displ_FillArea(0, text_y, _width, _height - text_y, DDDD_WHITE);

            char msg[64];
            sprintf(msg, "X: %.3f g", accel_data.x);
            Displ_WString(10, text_y, msg, Font16, 1, BLACK, DDDD_WHITE);

            sprintf(msg, "Y: %.3f g", accel_data.y);
            Displ_WString(10, text_y + 20, msg, Font12, 1, BLACK, DDDD_WHITE);

            sprintf(msg, "Z: %.3f g", accel_data.z);
            Displ_WString(10, text_y + 35, msg, Font12, 1, BLACK, DDDD_WHITE);

            sprintf(msg, "Time: %lu ms", HAL_GetTick());
            Displ_WString(10, text_y + 50, msg, Font12, 1, WHITE, DDDD_WHITE);

        } else {
            Displ_WString(50, PLOT_Y + PLOT_HEIGHT/2, "Sensor Failed!", Font16, 1, DD_RED, DDDD_WHITE);
        }
}

void drawStartButton(){
	Displ_CLS(DDDD_WHITE);
	Displ_fillRoundRect(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_H/6, DD_GREEN);
	Displ_CString(BUTTON_X + 6, BUTTON_Y + 6,
	                  BUTTON_X + BUTTON_W - 6, BUTTON_Y + BUTTON_H - 4,
	                  "START", Font24, 1, WHITE, DD_GREEN);
}
void drawLiveData() {
    if (HAL_GetTick() - lastUpdateTime >= updateInterval) {
        lastUpdateTime = HAL_GetTick();

        Displ_CLS(DDDD_WHITE);
        Displ_CString(0, 0, _width-1, 20, "LSM6DS3 Live Data:", Font16, 1, WHITE, DDDD_WHITE);

        LSM6DS3_AccelData accel_data = LSM6DS3_read_acceleration(hi2c1);
        if (accel_data.status == 1) {
            char msg[64];

            sprintf(msg, "Accel X: %.2f g", accel_data.x);
            Displ_CString(0, 30, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

            sprintf(msg, "Accel Y: %.2f g", accel_data.y);
            Displ_CString(0, 50, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

            sprintf(msg, "Accel Z: %.2f g", accel_data.z);
            Displ_CString(0, 70, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

            float total = sqrt(accel_data.x*accel_data.x + accel_data.y*accel_data.y + accel_data.z*accel_data.z);
            sprintf(msg, "Total: %.2f g", total);
            Displ_CString(0, 90, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

        } else {
            Displ_CString(0, 30, _width-1, 20, "Sensor Read Failed!", Font16, 1, RED, DDDD_WHITE);
        }
    }
}

void I2C_bus_scan(){
	char msg[32];
	uint8_t i2c_address;
	HAL_StatusTypeDef result;
	uint8_t who_am_i = 0;

	for (i2c_address = 1; i2c_address < 127; i2c_address++) {
	        result = HAL_I2C_IsDeviceReady(&hi2c1, (i2c_address << 1), 3, 10);
	        if (result == HAL_OK) {
	        	Displ_CLS(DDDD_WHITE);
	        	sprintf(msg, "Device found at 0x%02X", i2c_address);
	            Displ_CString(0, 24, _width - 1, 40, msg, Font16, 1, WHITE, DDDD_WHITE);
	            who_am_i = LSM6DS3_check_who_am_i(hi2c1);
	            if ( who_am_i == LSM6DS3_WHO_AM_I){
	                char msg2[64];
	                sprintf(msg2, "LSM6DS3 WHO_AM_I OK: 0x%02X", who_am_i);
	                Displ_CString(0, 48, _width-1, 40, msg2, Font16, 1, WHITE, DDDD_WHITE);
	            } else {
	            	Displ_CString(0, 48, _width-1, 40, "WHO_AM_I read failed", Font16, 1, WHITE, DDDD_WHITE);
	            }
	        }
	    }
	if (LSM6DS3_XL_CONFIG(hi2c1) == 0) {
	        Displ_CString(0, 72, _width-1, 20, "XL Config: OK", Font16, 1, WHITE, DDDD_WHITE);
	    } else {
	        Displ_CString(0, 72, _width-1, 20, "XL Config: FAIL", Font16, 1, WHITE, DDDD_WHITE);
	        HAL_Delay(2000);
	        return;
	    }
	    HAL_Delay(500);

	    if (LSM6DS3_G_CONFIG(hi2c1) == 0) {
	        Displ_CString(0, 96, _width-1, 20, "G Config: OK", Font16, 1, WHITE, DDDD_WHITE);
	    } else {
	        Displ_CString(0, 96, _width-1, 20, "G Config: FAIL", Font16, 1, WHITE, DDDD_WHITE);
	        HAL_Delay(2000);
	        return;
	    }
	    HAL_Delay(500);

	    if (LSM6DS3_XL_ODR_CONFIG(hi2c1) == 0) {
	        Displ_CString(0, 120, _width-1, 20, "ODR Config: OK", Font16, 1, WHITE, DDDD_WHITE);
	    } else {
	        Displ_CString(0, 120, _width-1, 20, "ODR Config: FAIL", Font16, 1, WHITE, DDDD_WHITE);
	        HAL_Delay(2000);
	        return;
	    }
	    HAL_Delay(1000);

	    Displ_CLS(DDDD_WHITE);
	    Displ_CString(0, 0, _width-1, 20, "LSM6DS3 Ready!", Font16, 1, WHITE, DDDD_WHITE);
	    float accel_odr = LSM6DS3_read_XL_ODR_debug_display(hi2c1);
}

float LSM6DS3_read_XL_ODR_debug_display(I2C_HandleTypeDef hi2c_def) {
    char msg[64];
    uint8_t ctrl1_xl_value;

    // Read CTRL1_XL register
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c_def, LSM6DS3_ADDRESS << 1, CTRL1_XL,
                                                I2C_MEMADD_SIZE_8BIT, &ctrl1_xl_value, 1, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        sprintf(msg, "ODR Read FAIL: %d", status);
        Displ_CString(0, 24, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);
        return -1.0f;
    }

    sprintf(msg, "CTRL1_XL: 0x%02X", ctrl1_xl_value);
    Displ_CString(0, 24, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

    // Extract ODR bits (bits 7:4)
    uint8_t odr_bits = (ctrl1_xl_value >> 4) & 0x0F;
    sprintf(msg, "ODR bits: 0x%X", odr_bits);
    Displ_CString(0, 48,_width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);

    // Convert ODR bits to actual frequency
    float odr_value;
    switch (odr_bits) {
        case 0x0: odr_value = 0.0f; strcpy(msg, "ODR: Power down"); break;
        case 0x1: odr_value = 12.5f; strcpy(msg, "ODR: 12.5 Hz"); break;
        case 0x2: odr_value = 26.0f; strcpy(msg, "ODR: 26 Hz"); break;
        case 0x3: odr_value = 52.0f; strcpy(msg, "ODR: 52 Hz"); break;
        case 0x4: odr_value = 104.0f; strcpy(msg, "ODR: 104 Hz"); break;
        case 0x5: odr_value = 208.0f; strcpy(msg, "ODR: 208 Hz"); break;
        case 0x6: odr_value = 416.0f; strcpy(msg, "ODR: 416 Hz"); break;
        case 0x7: odr_value = 833.0f; strcpy(msg, "ODR: 833 Hz"); break;
        case 0x8: odr_value = 1666.0f; strcpy(msg, "ODR: 1666 Hz"); break;
        case 0x9: odr_value = 3333.0f; strcpy(msg, "ODR: 3333 Hz"); break;
        case 0xA: odr_value = 6666.0f; strcpy(msg, "ODR: 6666 Hz"); break;
        default: odr_value = -1.0f; sprintf(msg, "ODR: Invalid 0x%X", odr_bits); break;
    }

    Displ_CString(0, 72, _width-1, 20, msg, Font16, 1, WHITE, DDDD_WHITE);
    return odr_value;
}


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

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S2_Init();
  MX_I2S3_Init();
  MX_SPI1_Init();
  MX_USB_HOST_Init();
  MX_SPI4_Init();
  MX_TIM3_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
  Displ_Init(Displ_Orientat_90);		// initialize the display and set the initial display orientation (here is orientaton: 0°) - THIS FUNCTION MUST PRECEED ANY OTHER DISPLAY FUNCTION CALL.
  Displ_CLS(BLACK);			// after initialization (above) and before turning on backlight (below), you can draw the initial display appearance. (here I'm just clearing display with a black background)
  Displ_BackLight('I');
  //Displ_FillArea(0,0,_width,_height,WHITE);
  drawStartButton();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* USER CODE END WHILE */
      MX_USB_HOST_Process();

      /* USER CODE BEGIN 3 */
      if (Touch_GotATouch(1)) {
          if (Touch_In_XY_area(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H)) {
              Touch_WaitForUntouch(300);
              I2C_bus_scan();
              while (1) {
                  MX_USB_HOST_Process();
                  //drawLiveData();
                  drawLiveDataWithPlot();
                  //HAL_Delay(updateInterval);
              }
          }
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 200;
  PeriphClkInitStruct.PLLI2S.PLLI2SM = 5;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_96K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  /* USER CODE END I2S2_Init 2 */

}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_96K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

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
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI4_Init(void)
{

  /* USER CODE BEGIN SPI4_Init 0 */

  /* USER CODE END SPI4_Init 0 */

  /* USER CODE BEGIN SPI4_Init 1 */

  /* USER CODE END SPI4_Init 1 */
  /* SPI4 parameter configuration*/
  hspi4.Instance = SPI4;
  hspi4.Init.Mode = SPI_MODE_MASTER;
  hspi4.Init.Direction = SPI_DIRECTION_2LINES;
  hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi4.Init.NSS = SPI_NSS_SOFT;
  hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi4.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI4_Init 2 */

  /* USER CODE END SPI4_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 10000;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 1000;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, CS_I2C_SPI_Pin|DISPL_RST_Pin|DISPL_LED_Pin|DISPL_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, DISPL_CS_Pin|TOUCH_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : DATA_Ready_Pin */
  GPIO_InitStruct.Pin = DATA_Ready_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DATA_Ready_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : CS_I2C_SPI_Pin */
  GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : INT1_Pin INT2_Pin MEMS_INT2_Pin */
  GPIO_InitStruct.Pin = INT1_Pin|INT2_Pin|MEMS_INT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : DISPL_CS_Pin DISPL_RST_Pin DISPL_DC_Pin TOUCH_CS_Pin */
  GPIO_InitStruct.Pin = DISPL_CS_Pin|DISPL_RST_Pin|DISPL_DC_Pin|TOUCH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : DISPL_LED_Pin */
  GPIO_InitStruct.Pin = DISPL_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DISPL_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TOUCH_INT_Pin */
  GPIO_InitStruct.Pin = TOUCH_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TOUCH_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin */
  GPIO_InitStruct.Pin = LD4_Pin|LD3_Pin|LD5_Pin|LD6_Pin
                          |Audio_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
