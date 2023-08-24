#pragma once
#include "stm32f4xx_conf.h"
#include "ports.h"


#pragma once

void init_midi()
{
    USART_InitTypeDef usart;
    USART_StructInit(&usart);

    // The format is 1 start bit, 8 data, no parity, 1 stop bit. 

    usart.USART_BaudRate = 31250; // 31.25 k
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_Mode = USART_Mode_Rx ;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    // Input
    GpioA::Port<10>::AlternateFunction(7);

    USART_Init(USART1, &usart);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    NVIC_SetPriority(USART1_IRQn, 3);
    NVIC_EnableIRQ(USART1_IRQn);  

    USART_Cmd(USART1, ENABLE);

}

volatile uint8_t rx_buf[128];
volatile uint8_t rx_buf_at = 0;
volatile int flags;

extern "C" void USART1_IRQHandler()
{
    // Read SR, which clears things I hear
    flags = USART1->SR;

    rx_buf[rx_buf_at] = USART_ReceiveData(USART1);
    rx_buf_at++;
}