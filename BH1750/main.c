#include "stm32f10x.h"                  // Device header
#include "stm32f10x_gpio.h"             // Keil::Device:StdPeriph Drivers:GPIO
#include "stm32f10x_i2c.h"              // Keil::Device:StdPeriph Drivers:I2C
#include "stm32f10x_rcc.h"              // Keil::Device:StdPeriph Drivers:RCC
#include "stm32f10x_usart.h"            // Keil::Device:StdPeriph Drivers:USART
#include "stdio.h"

/* UART1: APB2
TX: PA9 - AF_PP
RX: PA10 - IN_FLOATING

ADC: PA4 - AIN;
*/

#define BH1750_ADDR 0x46
#define BH1750_PWR_ON 0x01
#define BH1750_RESET  0x07
#define BH1750_CONT_HRES_MODE 0x10

void delay_ms(uint16_t time);
void config_uart();
void config_ADC();

//Ham duoi day de goi printf ra uart
//struct __FILE {
//    int dummy;
//};
//FILE __stdout;
// 
//int fputc(int ch, FILE *f) {
// 
//    uart_SendChar(ch);
//  
//    return ch;
//}

void delay_ms(uint16_t time){ // 1s
		uint16_t i;
			for(i = 0; i<time; i++){
					SysTick -> CTRL = 0x00000005;
					SysTick -> LOAD = 72000-1;
					SysTick -> VAL = 0;
			while(!(SysTick -> CTRL & (1 << 16))){}			
			}
}

void config_uart(){
		GPIO_InitTypeDef gpio_pin;
		USART_InitTypeDef usart_init;
	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
	
	// pin USART
		gpio_pin.GPIO_Pin = GPIO_Pin_9; // TX
		gpio_pin.GPIO_Mode = GPIO_Mode_AF_PP;
		gpio_pin.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &gpio_pin);
	
		gpio_pin.GPIO_Pin = GPIO_Pin_10; // RX
		gpio_pin.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		gpio_pin.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &gpio_pin);

			usart_init.USART_BaudRate = 115200;
			usart_init.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
			usart_init.USART_WordLength = USART_WordLength_8b;
			usart_init.USART_Parity = USART_Parity_No;
			usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
			
			USART_Init(USART1, &usart_init); // configure to register: Baudrate, Word length, Parity, Stop bit, Flow control, Mode… but still not turn on USART.
			USART_Cmd(USART1, ENABLE); // bat bo USART. -> complete
}	
	
	
// send a char
void USART1_SendChar(char c) {
    USART_SendData(USART1, c);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}
 // send string to hercules
void USART1_SendString(char *s) {
    while (*s) {
        USART1_SendChar(*s++);
    }
}

//void Config_I2C(){
////	GPIO_InitTypeDef GPIO_InitStruct;
//	I2C_InitTypeDef I2C_InitStruct;
//	 
//	// Bat clock
//	RCC -> APB2ENR |= 0x0008; // Bat clock cho GPIOB (bit 3)
//	RCC -> APB1ENR |= (1 << 21); //Bat clock cho I2C1 (bit 21)
//	
////	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
////	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

//	// Cau hình PB6, PB7 là Alternate Function output Open-Drain
//	GPIOB -> CRL |= 0xFF000000;
//	
////	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
////	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
////	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
////	GPIO_Init(GPIOB, &GPIO_InitStruct);

//	// Cau hình I2C1
//	I2C_InitStruct.I2C_ClockSpeed = 100000;      // Toc do truyen 100kHz
//	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;			 // Che do I2C
//	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2; // Thigh = Tlow
//	I2C_InitStruct.I2C_OwnAddress1 = 0x00;       // STM32 làm master, day la dia chi chinh khi stm lam slave nen ko quan trong
//	I2C_InitStruct.I2C_Ack = I2C_Ack_Enable;		 // Bat ACK: tin hieu xac nhan sau moi byte truyen
//	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // Che do dia chi 7 bit 
//	I2C_Init(I2C1, &I2C_InitStruct);

//	//Enable I2C1
//	I2C_Cmd(I2C1, ENABLE);
//}


void Config_I2C(){
		GPIO_InitTypeDef GPIO_init;
		I2C_InitTypeDef I2C_init;
		
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	
		GPIO_init.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
		GPIO_init.GPIO_Mode = GPIO_Mode_AF_OD;
		GPIO_init.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOB, &GPIO_init);
	
		I2C_init.I2C_ClockSpeed = 100000; // Toc do xung clock SCL = 100kHz.
		I2C_init.I2C_Mode = I2C_Mode_I2C;
		I2C_init.I2C_DutyCycle = I2C_DutyCycle_2; // Nghia là ti le xung clock = T_high / T_low = 2 (mac dinh cho 100kHz).
		I2C_init.I2C_OwnAddress1 = 0x00; // stm32 is master, enough not care
		I2C_init.I2C_Ack = I2C_Ack_Enable; // Nghia là sau khi nhan 1 byte, STM32 se tra ACK cho slave -> báo hieu "tôi nhan roi, gui tiep di".
		I2C_init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // almost sensor (BH1750) deu dùng 7-bit address, nên chon 7bit.
		I2C_Init(I2C1, &I2C_init);
		
		I2C_Cmd(I2C1, ENABLE);
}


		///////////////
//void BH1750_Init(){ 
//    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
//    I2C_GenerateSTART(I2C1, ENABLE);
//    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
//    I2C_Send7bitAddress(I2C1, BH1750_ADDR, I2C_Direction_Transmitter);
//    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

//    I2C_SendData(I2C1, BH1750_PWR_ON);  // b?t ngu?n
//    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

//    I2C_SendData(I2C1, BH1750_CONT_HRES_MODE); // ch?n mode do
//    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

//    I2C_GenerateSTOP(I2C1, ENABLE);
//}

void I2C_WriteByte(uint8_t address, uint8_t data){
	// Send START
  I2C_GenerateSTART(I2C1, ENABLE);

  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)); // ktra co SB = 1 xac nhan start thanh cong
  I2C_Send7bitAddress(I2C1, address, I2C_Direction_Transmitter); // gui 7 bit dia chi va 1 bit ghi (W = 0)

  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); // Cho gui xong

  I2C_SendData(I2C1,data); // Gui du lieu cho I2C1
  while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // Cho gui xong du lieu, du lieu da duoc nap vao DR
	
	I2C_GenerateSTOP(I2C1, ENABLE);
}

void BH1750_Init(void) {
    delay_ms(10);
    I2C_WriteByte(BH1750_ADDR, BH1750_PWR_ON); 
    delay_ms(10);
    I2C_WriteByte(BH1750_ADDR, BH1750_CONT_HRES_MODE); 
    delay_ms(200); 
}


uint16_t BH1750_ReadLight() {
    uint16_t value = 0;
		uint8_t lsb;
		uint8_t msb;
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)); // ktra co SB = 1 xac nhan start thanh cong

    I2C_Send7bitAddress(I2C1, BH1750_ADDR, I2C_Direction_Receiver); // gui 7 bit dia chi BH1750 va 1 bit 0x01 la Read
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); // Cho gui xong, xac nhan vao che do Read

		//Doc du lieu 2 byte
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    msb = I2C_ReceiveData(I2C1); // bit cao

    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    lsb = I2C_ReceiveData(I2C1);

    I2C_AcknowledgeConfig(I2C1, DISABLE); // Tat ACK de bao rang stm ko nhan them du lieu
    I2C_GenerateSTOP(I2C1, ENABLE); // ket thuc giao tiep
    I2C_AcknowledgeConfig(I2C1, ENABLE); // Bat ACK lai de chuan bi cho lan do sau

    value = ((msb << 8) | lsb) / 1.2; // chuyen ve Lux (Do roi)
    return value;
}


uint16_t rv;
char buffer[40];

int main(){
	
	Config_I2C();
	config_uart();
	USART1_SendString("prepareeee \n");
	BH1750_Init();
	USART1_SendString("finish \n");
	delay_ms(200);

	
	
	while(1){
		rv = BH1750_ReadLight();		
		sprintf(buffer, "value LUX: %u \n", rv);
		USART1_SendString(buffer);
		delay_ms(500);		
}
}