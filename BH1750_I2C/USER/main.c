#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stdio.h"

// Chân ADDR n?i GND -> Address = 0x23. Thu vi?n yêu c?u d?a ch? dã d?ch trái.
#define BH1750_ADDR 0x46 
#define BH1750_PWR_ON 0x01
#define BH1750_CONT_HRES_MODE 0x10

char buffer[50];

// --- HÀM DELAY S? D?NG SYSTICK ---
void delay_ms(uint16_t time) {
    SysTick->LOAD = 72000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 5; 
    for(uint16_t i = 0; i < time; i++) {
        while(!(SysTick->CTRL & (1 << 16)));
    }
    SysTick->CTRL = 0;
}

// --- C?U HÌNH UART1 (PA9-TX, PA10-RX) ---
void config_uart(void) {
    GPIO_InitTypeDef gpio_pin;
    USART_InitTypeDef usart_init;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio_pin.GPIO_Pin = GPIO_Pin_9;
    gpio_pin.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_pin.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_pin);

    gpio_pin.GPIO_Pin = GPIO_Pin_10;
    gpio_pin.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio_pin);

    usart_init.USART_BaudRate = 115200;
    usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    
    USART_Init(USART1, &usart_init);
    USART_Cmd(USART1, ENABLE);
}

void UART_SendString(char *s) {
    while (*s) {
        USART_SendData(USART1, *s++);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}

// --- C?U HÌNH I2C1 (PB6-SCL, PB7-SDA) ---
void Config_I2C(void) {
    GPIO_InitTypeDef GPIO_init;
    I2C_InitTypeDef I2C_init;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    // PB6, PB7 ph?i là Alternate Function Open Drain
    GPIO_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_init.GPIO_Mode = GPIO_Mode_AF_OD; 
    GPIO_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_init);

    I2C_init.I2C_ClockSpeed = 100000;
    I2C_init.I2C_Mode = I2C_Mode_I2C;
    I2C_init.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_init.I2C_OwnAddress1 = 0x00;
    I2C_init.I2C_Ack = I2C_Ack_Enable;
    I2C_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_init);
    
    I2C_Cmd(I2C1, ENABLE);
}

// --- HÀM GHI I2C CÓ TIMEOUT CH?NG TREO ---
uint8_t I2C_Write(uint8_t addr, uint8_t cmd) {
    uint32_t timeout = 10000;
    
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) if(--timeout==0) return 1;

    I2C_Send7bitAddress(I2C1, addr, I2C_Direction_Transmitter);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) if(--timeout==0) return 2;

    I2C_SendData(I2C1, cmd);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) if(--timeout==0) return 3;

    I2C_GenerateSTOP(I2C1, ENABLE);
    return 0;
}

void BH1750_Init(void) {
    delay_ms(50); 
    if(I2C_Write(BH1750_ADDR, BH1750_PWR_ON) != 0) {
        UART_SendString("BH1750: Error Power On\n");
    }
    if(I2C_Write(BH1750_ADDR, BH1750_CONT_HRES_MODE) != 0) {
        UART_SendString("BH1750: Error Config Mode\n");
    }
    delay_ms(200); // Ð?i c?m bi?n ?n d?nh
}

uint16_t BH1750_ReadLight(void) {
    uint32_t timeout = 10000;
    uint8_t h, l;

    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) if(--timeout==0) return 0;

    I2C_Send7bitAddress(I2C1, BH1750_ADDR, I2C_Direction_Receiver);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) if(--timeout==0) return 0;

    // Ð?c byte cao
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    h = I2C_ReceiveData(I2C1);

    // T?t ACK và d?ng tru?c khi nh?n byte cu?i
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);

    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    l = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, ENABLE); // B?t l?i cho l?n sau

    return (uint16_t)((h << 8) | l) / 1.2;
}

int main(void) {
    config_uart();
    Config_I2C();
    
    UART_SendString("--- H? th?ng b?t d?u ---\n");
    BH1750_Init();
    UART_SendString("--- Kh?i t?o xong ---\n");

    while(1) {
        uint16_t lux = BH1750_ReadLight();
        sprintf(buffer, "Cuong do sang: %u Lux\n", lux);
        UART_SendString(buffer);
        delay_ms(1000);
    }
}