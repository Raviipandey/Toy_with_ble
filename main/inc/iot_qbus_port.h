#ifndef __IOT_QBUS_PORT_H
#define __IOT_QBUS_PORT_H

#include "main.h"
#include "driver/uart.h"
#include "soc/uart_reg.h"
#include "esp32/rom/uart.h"
#include "driver/gpio.h"


#define 	UART_PORT_NUM 		UART_NUM_0
#define 	TXD_PIN 			(GPIO_NUM_1)
#define 	RXD_PIN 			(GPIO_NUM_3)

#define UART_EMPTY_THRESH_DEFAULT  	(10)
#define UART_FULL_THRESH_DEFAULT  	(1000)
#define UART_TOUT_THRESH_DEFAULT  	(10)
#define UART_QBUS_RX_SIZE			1024
#define UART_QBUS_TX_SIZE			1024

#define DISP_QBUS_TX_TIMEOUT    250

void IotQbusUartPortInit(void);
void byteReceived(uint8_t recvChar);
void IotQbusInterruptEnable(uint8_t xTxEnable, uint8_t xRxEnable);
void IotQbusSendByte(uint8_t sendDataQbus);


#endif
