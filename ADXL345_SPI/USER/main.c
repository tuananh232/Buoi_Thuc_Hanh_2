/*
#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_usart.h"
#include <stdio.h>

// ================== KHAI BÁO PROTOTYPES (S?a l?i Warning) ================== 
void delay_ms(uint16_t t);
void UART_Config(void);
void UART_SendChar(char c);
void UART_SendString(char *s);
void SPI_Config(void);
uint8_t SPI_Transfer(uint8_t data);
void ADXL345_Write(uint8_t reg, uint8_t data);
void ADXL345_Init(void);
void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);

// ================== ADXL345 DEFINES ================== 
#define ADXL345_POWER_CTL     0x2D
#define ADXL345_DATA_FORMAT   0x31
#define ADXL345_DATAX0        0x32

#define CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)

// ================== DELAY ================== 
void delay_ms(uint16_t t)
{
    // Luu ý: Vòng l?p này ph? thu?c vào t?c d? CPU, dùng t?m cho m?c dích test
    for(volatile uint32_t i=0; i < (uint32_t)t*8000; i++);
}

// ================== UART CONFIG ================== 
void UART_Config(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // TX - PA9
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = 115200;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

void UART_SendChar(char c)
{
    USART_SendData(USART1, (uint16_t)c);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendString(char *s)
{
    while(*s) UART_SendChar(*s++);
}

// ================== SPI1 CONFIG ================== 
void SPI_Config(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    // SCK (PA5) + MOSI (PA7)
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    // MISO (PA6)
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    // CS (PA4)
    gpio.GPIO_Pin = GPIO_Pin_4;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    CS_HIGH();

    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    
    // S?A L?I ERROR T?I ÐÂY:
    spi.SPI_CPHA = SPI_CPHA_2Edge; 
    
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}

// ================== SPI TRANSFER ================== 
uint8_t SPI_Transfer(uint8_t data)
{
    // Ch? cho d?n khi thanh ghi truy?n tr?ng
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);
    
    // Ch? cho d?n khi nh?n du?c d? li?u
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t)SPI_I2S_ReceiveData(SPI1); // Ép ki?u d? h?t warning
}

// ================== ADXL345 FUNCTIONS ================== 
void ADXL345_Write(uint8_t reg, uint8_t data)
{
    CS_LOW();
    SPI_Transfer(reg);
    SPI_Transfer(data);
    CS_HIGH();
}

void ADXL345_Init(void)
{
    ADXL345_Write(ADXL345_DATA_FORMAT, 0x0B);  // Full resolution +/- 16g
    ADXL345_Write(ADXL345_POWER_CTL, 0x08);    // Measure mode
}

void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    CS_LOW();

    // 0xC0 = Read (bit 7 = 1) + Multibyte (bit 6 = 1)
    SPI_Transfer(0xC0 | ADXL345_DATAX0); 

    *x = (int16_t)SPI_Transfer(0x00);
    *x |= (int16_t)(SPI_Transfer(0x00) << 8);

    *y = (int16_t)SPI_Transfer(0x00);
    *y |= (int16_t)(SPI_Transfer(0x00) << 8);

    *z = (int16_t)SPI_Transfer(0x00);
    *z |= (int16_t)(SPI_Transfer(0x00) << 8);

    CS_HIGH();
}

// ================== MAIN ================== 
int main(void)
{
    int16_t x, y, z;
    char buffer[100];

    SystemInit();
    UART_Config();
    SPI_Config();
    ADXL345_Init();

    UART_SendString("ADXL345 Ready\r\n");

    while(1)
    {
        ADXL345_ReadXYZ(&x, &y, &z);

        sprintf(buffer, "X:%d  Y:%d  Z:%d\r\n", x, y, z);
        UART_SendString(buffer);

        delay_ms(500);
    }
}

*/


#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_usart.h"
#include <stdio.h>

/* ================== PROTOTYPES ================== */
void delay_ms(uint16_t t);
void UART_Config(void);
void UART_SendChar(char c);
void UART_SendString(char *s);
void SPI_Config(void);
uint8_t SPI_Transfer(uint8_t data);
void ADXL345_Write(uint8_t reg, uint8_t data);
uint8_t ADXL345_ReadReg(uint8_t reg);
void ADXL345_Init(void);
void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z);

/* ================== ADXL345 DEFINES ================== */
#define ADXL345_DEVID        0x00
#define ADXL345_POWER_CTL    0x2D
#define ADXL345_DATA_FORMAT  0x31
#define ADXL345_DATAX0       0x32

#define CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)

/* ================== DELAY ================== */
void delay_ms(uint16_t t)
{
    for(volatile uint32_t i=0; i < (uint32_t)t*8000; i++);
}

/* ================== UART ================== */
void UART_Config(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = 115200;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_Mode = USART_Mode_Tx;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

void UART_SendChar(char c)
{
    USART_SendData(USART1, (uint16_t)c);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void UART_SendString(char *s)
{
    while(*s) UART_SendChar(*s++);
}

/* ================== SPI1 CONFIG (MODE 3) ================== */
void SPI_Config(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    // SCK + MOSI
    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    // MISO
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    // CS
    gpio.GPIO_Pin = GPIO_Pin_4;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    CS_HIGH();

    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;

    // ===== QUAN TR?NG: MODE 3 =====
    spi.SPI_CPOL = SPI_CPOL_High;
    spi.SPI_CPHA = SPI_CPHA_2Edge;

    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;

    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}

/* ================== SPI TRANSFER ================== */
uint8_t SPI_Transfer(uint8_t data)
{
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);

    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

/* ================== ADXL345 ================== */
void ADXL345_Write(uint8_t reg, uint8_t data)
{
    CS_LOW();
    SPI_Transfer(reg);
    SPI_Transfer(data);
    CS_HIGH();
}

uint8_t ADXL345_ReadReg(uint8_t reg)
{
    uint8_t value;
    CS_LOW();
    SPI_Transfer(0x80 | reg);
    value = SPI_Transfer(0x00);
    CS_HIGH();
    return value;
}

void ADXL345_Init(void)
{
    ADXL345_Write(ADXL345_DATA_FORMAT, 0x0B);
    ADXL345_Write(ADXL345_POWER_CTL, 0x08);
}

void ADXL345_ReadXYZ(int16_t *x, int16_t *y, int16_t *z)
{
    CS_LOW();

    SPI_Transfer(0xC0 | ADXL345_DATAX0);

    *x = SPI_Transfer(0x00);
    *x |= (SPI_Transfer(0x00) << 8);

    *y = SPI_Transfer(0x00);
    *y |= (SPI_Transfer(0x00) << 8);

    *z = SPI_Transfer(0x00);
    *z |= (SPI_Transfer(0x00) << 8);

    CS_HIGH();
}

/* ================== MAIN ================== */
int main(void)
{
    int16_t x, y, z;
    uint8_t id;
    char buffer[100];

    SystemInit();
    UART_Config();
    SPI_Config();

    id = ADXL345_ReadReg(ADXL345_DEVID);

    sprintf(buffer, "Device ID: %d\r\n", id);
    UART_SendString(buffer);

    ADXL345_Init();

    UART_SendString("ADXL345 Ready\r\n");

    while(1)
    {
        ADXL345_ReadXYZ(&x, &y, &z);

        sprintf(buffer, "X:%d  Y:%d  Z:%d\r\n", x, y, z);
        UART_SendString(buffer);

        delay_ms(500);
    }
}