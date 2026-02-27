#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_tim.h"
#include <stdio.h>

/* ================== DHT22 CONFIG ================== */
#define DHT_PORT GPIOA
#define DHT_PIN  GPIO_Pin_1

/* ================== DELAY TIMER 2 ================== */
void TIM2_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM_TimeBaseInitTypeDef tim;
    tim.TIM_Prescaler = 72 - 1;      // 72MHz / 72 = 1MHz (1us)
    tim.TIM_Period = 0xFFFF;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &tim);
    TIM_Cmd(TIM2, ENABLE);
}

void Delay_us(uint16_t us)
{
    TIM_SetCounter(TIM2, 0);
    while (TIM_GetCounter(TIM2) < us);
}

void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}

/* ================== DHT22 FUNCTIONS ================== */
void DHT_Pin_Output(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = DHT_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT_PORT, &gpio);
}

void DHT_Pin_Input(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = DHT_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT_PORT, &gpio);
}

uint8_t DHT_Start(void)
{
    DHT_Pin_Output();
    GPIO_ResetBits(DHT_PORT, DHT_PIN);
    Delay_ms(18);              // Keo xuong LOW 18ms

    GPIO_SetBits(DHT_PORT, DHT_PIN);
    Delay_us(30);              // Keo len HIGH 30us

    DHT_Pin_Input();

    Delay_us(40);
    if (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)) return 1; // Loi mat ket noi

    Delay_us(80);
    if (!GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN)) return 1; // Loi phan hoi

    Delay_us(80);
    return 0; // OK
}

uint8_t DHT_ReadByte(void)
{
    uint8_t i, data = 0;
    uint16_t timeout;

    for (i = 0; i < 8; i++)
    {
        timeout = 10000;
        /* Cho DHT keo HIGH (Co timeout chong treo) */
        while (!GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) && timeout > 0) timeout--;
        
        Delay_us(40);

        /* Neu sau 40us ma van HIGH thi bit do la 1 */
        if (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN))
        {
            data |= (1 << (7 - i)); // Ghi bit 1 vao dung vi tri
            timeout = 10000;
            /* Cho DHT keo LOW tro lai */
            while (GPIO_ReadInputDataBit(DHT_PORT, DHT_PIN) && timeout > 0) timeout--;
        }
    }
    return data;
}

uint8_t DHT_Read(float *temp, float *humi)
{
    uint8_t rh_int, rh_dec, t_int, t_dec, checksum;
    uint16_t raw_humi, raw_temp;

    if (DHT_Start()) return 1; // Loi khong ket noi duoc

    rh_int   = DHT_ReadByte();
    rh_dec   = DHT_ReadByte();
    t_int    = DHT_ReadByte();
    t_dec    = DHT_ReadByte();
    checksum = DHT_ReadByte();

    /* Kiem tra Checksum cho an toan */
    if ((uint8_t)(rh_int + rh_dec + t_int + t_dec) != checksum) return 2;

    /* Tinh toan Do Am (16-bit / 10) */
    raw_humi = (rh_int << 8) | rh_dec;
    *humi = (float)raw_humi / 10.0f;

    /* Tinh toan Nhiet Do (Xu ly bit 15 cho nhiet do am) */
    raw_temp = ((t_int & 0x7F) << 8) | t_dec;
    if (t_int & 0x80) {
        *temp = (float)raw_temp / 10.0f * -1.0f;
    } else {
        *temp = (float)raw_temp / 10.0f;
    }

    return 0;
}

/* ================== UART1 CONFIG ================== */
void USART1_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef gpio;

    /* TX - PA9 */
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* RX - PA10 */
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef us;
    us.USART_BaudRate = 9600;
    us.USART_WordLength = USART_WordLength_8b;
    us.USART_StopBits = USART_StopBits_1;
    us.USART_Parity = USART_Parity_No;
    us.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    us.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &us);
    USART_Cmd(USART1, ENABLE);
}

void UART_SendString(char *str)
{
    while (*str)
    {
        USART_SendData(USART1, *str++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}

/* ================== MAIN ================== */
int main(void)
{
    float temp = 0.0f, humi = 0.0f;
    char buf[50];

    SystemInit();
    TIM2_Init();
    USART1_Init();

    UART_SendString("--- STARTING DHT22 SENSOR ---\r\n");

    while (1)
    {
        uint8_t status = DHT_Read(&temp, &humi);
        
        if (status == 0)
        {
            /* In ra man hinh kem 1 chu so thap phan */
            sprintf(buf, "Temp: %.1f C | Humi: %.1f %%\r\n", temp, humi);
            UART_SendString(buf);
        }
        else if (status == 1)
        {
            UART_SendString("ERROR: Sensor not responding!\r\n");
        }
        else if (status == 2)
        {
            UART_SendString("ERROR: Checksum failed!\r\n");
        }

        /* DHT22 bat buoc phai delay toi thieu 2 giay giua cac lan doc */
        Delay_ms(2000);  
    }
}