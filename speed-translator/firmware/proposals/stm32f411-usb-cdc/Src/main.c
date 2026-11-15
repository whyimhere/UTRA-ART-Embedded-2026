
/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : STM32F411 USB CDC Sine PWM Controller
 *
 * Project: UTRA Speed Signal Translator
 *
 * Features:
 * - 96 MHz system clock
 * - 48 MHz USB clock
 * - TIM1 CH1 PWM output on PA8
 * - 20 kHz PWM frequency
 * - 0% -> 100% -> 0% sine-shaped duty cycle
 * - 4-second sine period
 * - USB CDC communication
 * - PA0 frequency reporting over USB
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* USER CODE BEGIN Includes */
#include "pwm_control.h"
#include "sine_pwm.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
#define CAPTURE_TICK_HZ 1000000UL
#define SIGNAL_TIMEOUT_MS 500U
#define REPORT_INTERVAL_MS 500U

static volatile uint32_t previous_capture;
static volatile uint32_t last_edge_ms;
static volatile uint32_t period_ticks;
static volatile uint8_t have_previous_edge;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN PFP */
static void report_input_frequency(void);

/* USER CODE END PFP */

/**
 * @brief Main program
 * @retval int
 */
int main(void)
{
    HAL_Init();

    /* Configure 96 MHz system clock */
    SystemClock_Config();

    /* Initialize peripherals */
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_USB_DEVICE_Init();

    /* USER CODE BEGIN 2 */

    /*
     * Initialize PWM at 20 kHz, 50% duty.
     * TIM1 PWM starts here.
     */
    PWM_Control_Init();

    /*
     * Initialize sine-shaped duty cycle.
     * Set initial duty to 0%.
     */
    Sine_PWM_Init();

    if (HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        /*
         * Process incoming USB CDC commands.
         */
        PWM_Control_Process();

        /*
         * Automatically update PWM duty cycle.
         *
         * 0% -> 100% -> 0%
         * Complete cycle: 4 seconds
         * Update interval: 10 milliseconds
         */
        Sine_PWM_Update();
        report_input_frequency();

        /* USER CODE END 3 */
    }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable power control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Voltage regulator scaling */
    __HAL_PWR_VOLTAGESCALING_CONFIG(
        PWR_REGULATOR_VOLTAGE_SCALE1
    );

    /*
     * External HSE crystal = 25 MHz
     *
     * PLLM = 25
     * PLLN = 192
     * PLLP = 2
     * PLLQ = 4
     *
     * SYSCLK = 96 MHz
     * USB clock = 48 MHz
     */

    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSE;

    RCC_OscInitStruct.HSEState =
        RCC_HSE_ON;

    RCC_OscInitStruct.PLL.PLLState =
        RCC_PLL_ON;

    RCC_OscInitStruct.PLL.PLLSource =
        RCC_PLLSOURCE_HSE;

    RCC_OscInitStruct.PLL.PLLM = 25;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 4;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct)
        != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure bus clocks */

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource =
        RCC_SYSCLKSOURCE_PLLCLK;

    RCC_ClkInitStruct.AHBCLKDivider =
        RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.APB1CLKDivider =
        RCC_HCLK_DIV2;

    RCC_ClkInitStruct.APB2CLKDivider =
        RCC_HCLK_DIV1;

    /*
     * SYSCLK = 96 MHz
     * HCLK   = 96 MHz
     * PCLK1  = 48 MHz
     * PCLK2  = 96 MHz
     *
     * TIM1 clock = 96 MHz
     * TIM2 clock = 96 MHz
     */

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_3) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief TIM1 PWM Initialization
 *
 * TIM1 Channel 1 -> PA8
 *
 * Initial PWM:
 * Frequency = 20 kHz
 * Duty = 50%
 *
 * Sine_PWM_Init() changes duty to 0%.
 */
static void MX_TIM1_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* Enable TIM1 clock */
    __HAL_RCC_TIM1_CLK_ENABLE();

    /*
     * Timer clock = 96 MHz
     *
     * PWM frequency:
     *
     * 96,000,000 / ((0 + 1) * (4799 + 1))
     * = 20,000 Hz
     */

    htim1.Instance = TIM1;

    htim1.Init.Prescaler = 0;

    htim1.Init.CounterMode =
        TIM_COUNTERMODE_UP;

    htim1.Init.Period = 4799;

    htim1.Init.ClockDivision =
        TIM_CLOCKDIVISION_DIV1;

    htim1.Init.RepetitionCounter = 0;

    htim1.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }

    /* TIM1 master configuration */

    sMasterConfig.MasterOutputTrigger =
        TIM_TRGO_RESET;

    sMasterConfig.MasterSlaveMode =
        TIM_MASTERSLAVEMODE_DISABLE;

    if (HAL_TIMEx_MasterConfigSynchronization(
            &htim1,
            &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure PWM Channel 1 */

    sConfigOC.OCMode =
        TIM_OCMODE_PWM1;

    sConfigOC.Pulse = 2400;

    sConfigOC.OCPolarity =
        TIM_OCPOLARITY_HIGH;

    sConfigOC.OCNPolarity =
        TIM_OCNPOLARITY_HIGH;

    sConfigOC.OCFastMode =
        TIM_OCFAST_DISABLE;

    sConfigOC.OCIdleState =
        TIM_OCIDLESTATE_RESET;

    sConfigOC.OCNIdleState =
        TIM_OCNIDLESTATE_RESET;

    if (HAL_TIM_PWM_ConfigChannel(
            &htim1,
            &sConfigOC,
            TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief TIM2 Input Capture Initialization
 *
 * Timer clock = 96 MHz
 * Prescaler = 95
 * Counter frequency = 1 MHz

 */
static void MX_TIM2_Init(void)
{
    TIM_IC_InitTypeDef sConfigIC = {0};

    /* Enable TIM2 clock */
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;

    htim2.Init.Prescaler = 95;

    htim2.Init.CounterMode =
        TIM_COUNTERMODE_UP;

    htim2.Init.Period =
        0xFFFFFFFFU;

    htim2.Init.ClockDivision =
        TIM_CLOCKDIVISION_DIV1;

    htim2.Init.AutoReloadPreload =
        TIM_AUTORELOAD_PRELOAD_DISABLE;

    /* Base init runs the existing MSP setup for PA0 and TIM2 IRQ. */
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK ||
        HAL_TIM_IC_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure TIM2 Channel 1 */

    sConfigIC.ICPolarity =
        TIM_INPUTCHANNELPOLARITY_RISING;

    sConfigIC.ICSelection =
        TIM_ICSELECTION_DIRECTTI;

    sConfigIC.ICPrescaler =
        TIM_ICPSC_DIV1;

    sConfigIC.ICFilter = 0;

    if (HAL_TIM_IC_ConfigChannel(
            &htim2,
            &sConfigIC,
            TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization
 *
 * PA8 = TIM1 Channel 1 PWM output
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIOA clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure PA8 as TIM1_CH1 */

    GPIO_InitStruct.Pin =
        GPIO_PIN_8;

    GPIO_InitStruct.Mode =
        GPIO_MODE_AF_PP;

    GPIO_InitStruct.Pull =
        GPIO_NOPULL;

    GPIO_InitStruct.Speed =
        GPIO_SPEED_FREQ_HIGH;

    GPIO_InitStruct.Alternate =
        GPIO_AF1_TIM1;

    HAL_GPIO_Init(
        GPIOA,
        &GPIO_InitStruct
    );
}

/**
 * @brief Error Handler
 */
void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
        /* Stay here if initialization fails */
    }
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Assert failed
 */
void assert_failed(uint8_t *file,
                   uint32_t line)
{
    (void)file;
    (void)line;
}

#endif



/* USER CODE BEGIN 4 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *timer)
{
    if (timer->Instance != TIM2 || timer->Channel != HAL_TIM_ACTIVE_CHANNEL_1)
    {
        return;
    }

    uint32_t captured = HAL_TIM_ReadCapturedValue(timer, TIM_CHANNEL_1);
    uint32_t now = HAL_GetTick();
    period_ticks = 0U;
    if (have_previous_edge && now - last_edge_ms < SIGNAL_TIMEOUT_MS)
    {
        /* Unsigned subtraction handles wrap of the 32-bit counter. */
        period_ticks = captured - previous_capture;
    }
    previous_capture = captured;
    last_edge_ms = now;
    have_previous_edge = 1U;
}

static void report_input_frequency(void)
{
    static uint32_t last_report_ms;
    uint32_t now = HAL_GetTick();
    if (now - last_report_ms < REPORT_INTERVAL_MS)
    {
        return;
    }
    last_report_ms = now;

    /* Take a consistent snapshot of the values written by the interrupt. */
    uint32_t irq_state = __get_PRIMASK();
    __disable_irq();
    uint32_t ticks = period_ticks;
    uint32_t edge_age_ms = HAL_GetTick() - last_edge_ms;
    if (edge_age_ms >= SIGNAL_TIMEOUT_MS)
    {
        period_ticks = 0U;
        have_previous_edge = 0U;
    }
    __set_PRIMASK(irq_state);

    char message[64];
    int length;
    if (ticks == 0U || edge_age_ms >= SIGNAL_TIMEOUT_MS)
    {
        length = snprintf(message, sizeof message, "PA0: no signal\r\n");
    }
    else
    {
        unsigned long centihz = (CAPTURE_TICK_HZ * 100UL + ticks / 2U) / ticks;
        length = snprintf(message, sizeof message, "PA0: %lu.%02lu Hz\r\n",
                          centihz / 100UL, centihz % 100UL);
    }

    if (length > 0 && (size_t)length < sizeof message)
    {
        /* CDC copies the message into its own transmit buffer. */
        CDC_Transmit_FS((uint8_t *)message, (uint16_t)length);
    }
}
/* USER CODE END 4 */
