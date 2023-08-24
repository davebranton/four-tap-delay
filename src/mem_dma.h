#pragma once

#include "stm32f4xx_conf.h"
#include "ports.h"

using SPI1_NSS = GpioA::Port<15>;

DMA_InitTypeDef dma;

void mem_init()
{
   
    // select pins
    GpioB::Port<3>::AlternateFunction(5); // SPI1_SCK
    //GpioA::Port<15>::AlternateFunction(5); // SPI1_NSS
    SPI1_NSS::Output(); // SPI1_NSS drive by hand
    SPI1_NSS::Set();
    GpioB::Port<4>::AlternateFunction(5); // SPI1_MISO
    GpioB::Port<5>::AlternateFunction(5); // SPI1_MOSI

    GpioE::Port<4>::Output();
    GpioE::Port<2>::Output();

    GpioE::Port<2>::Reset();
    GpioE::Port<4>::Reset();

    // set up timer
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    
    TIM7->PSC = 1;    // Set PSC+1 = 160000
    TIM7->CNT = 0;
    TIM7->ARR = 1;        // Set timer to reset after CNT = 10   

    // This gives us an inter-SPI-frame delay of about 800ns to 1us 

    TIM7->CR1 |= TIM_CR1_OPM; // one-shot

    TIM7->DIER = TIM_DIER_UIE; // interrupt on update event, which is overflow I think.

    NVIC_SetPriority(TIM7_IRQn, 1);
    NVIC_EnableIRQ(TIM7_IRQn);

    // DMA

  // Turn on DMA
  // DMA 2 Stream 0 set to Channel 3 for SPI1_RX
  // DMA 2 Stream 3 set to Channel 3 for SPI1_TX


  
  // This is the TX stream
  dma.DMA_Channel = DMA_Channel_3; // For SPI1
  dma.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  dma.DMA_BufferSize = 3; // I think??
  dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16 bits
  dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  dma.DMA_Mode = DMA_Mode_Normal;
  dma.DMA_Priority = DMA_Priority_Medium;
  dma.DMA_FIFOMode = DMA_FIFOMode_Disable;
  dma.DMA_FIFOThreshold = 0;
  dma.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  dma.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;


  dma.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DR);
  dma.DMA_Memory0BaseAddr = 0;
  DMA_Init(DMA2_Stream3, &dma);

  // And the RX stream
  dma.DMA_DIR = DMA_DIR_PeripheralToMemory;
  dma.DMA_Memory0BaseAddr = 0;
  DMA_Init(DMA2_Stream0, &dma);

  SPI_InitTypeDef spi;

  // SPI_BaudRatePrescaler_16 = 5.25Mhz
  // SPI_BaudRatePrescaler_8 = 10.5Mhz
  // SPI_BaudRatePrescaler_4 = 21

  spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4; // This is about 20MHz
  spi.SPI_CPOL = SPI_CPOL_Low;
  spi.SPI_CPHA = SPI_CPHA_1Edge; // 1st edge, which is the rising edge because idle clock is low
  spi.SPI_CRCPolynomial = 7; // default
  spi.SPI_DataSize = SPI_DataSize_16b;
  spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  spi.SPI_FirstBit = SPI_FirstBit_MSB;
  spi.SPI_Mode = SPI_Mode_Master;
  spi.SPI_NSS = SPI_NSS_Soft;

  SPI_Init(SPI1, &spi);

  SPI1->CR2 |= SPI_CR2_RXDMAEN;
  SPI1->CR2 |= SPI_CR2_TXDMAEN;
  
  NVIC_SetPriority(DMA2_Stream0_IRQn, 1);
  DMA2_Stream0->CR |=  DMA_SxCR_TCIE;
  
}


volatile int total_slots = 0;
volatile int slots_done = 0;
volatile bool mem_done_flag = true;
volatile uint16_t tx_buffer[3*4];
volatile uint16_t rx_buffer[3*4];

void _mem_start_dma()
{
  SPI1_NSS::Reset(); // CS low

  DMA2_Stream3->M0AR = ((uint32_t)tx_buffer) + slots_done * 6;
  DMA2_Stream3->NDTR = 3;

  DMA2_Stream0->M0AR = ((uint32_t)rx_buffer) + slots_done * 6;
  DMA2_Stream0->NDTR = 3;

  slots_done++;

  DMA2->LIFCR = 0xffffffff; // clear all flags
  DMA_Cmd(DMA2_Stream0, ENABLE);
  DMA_Cmd(DMA2_Stream3, ENABLE);

  NVIC_EnableIRQ(DMA2_Stream0_IRQn);  

  SPI_Cmd(SPI1, ENABLE);  
}

volatile bool trigger_next_dma = false;
extern "C" void DMA2_Stream0_IRQHandler()
{
  if (DMA2->LISR & DMA_LISR_TCIF0)
  {
    trigger_next_dma = true;
    // wait for SPI to be done
    //while(SPI1->SR & SPI_SR_BSY);
    SPI1_NSS::Set(); // CS high
    TIM7->CR1 |= TIM_CR1_CEN; // start timer
  }
  DMA2->LIFCR |= DMA_LIFCR_CTCIF0; // clear flag
}

extern "C" void TIM7_IRQHandler() 
{
  TIM7->SR = 0;

  if (trigger_next_dma)
  {
    trigger_next_dma = false;    
    
    if (slots_done < total_slots)
    {
      // let's set up another
      
      _mem_start_dma();
    }
    else
    {
      mem_done_flag = true;
      BlueDebugLead::Reset();
      NVIC_DisableIRQ(DMA2_Stream0_IRQn);  
    }
  }
}


void mem_sched_write(int slot, uint32_t addr, uint16_t val)
{

    tx_buffer[slot*3] = (0x02 << 8) | ((addr & 0xFF0000) >> 16); // write cmd and top 8 bits of 24 bit addr
    tx_buffer[slot*3+1] = (addr & 0xFFFF); // bottom 16 bits of address
    tx_buffer[slot*3+2] = val;

}
void mem_sched_read(int slot, uint32_t addr)
{
    tx_buffer[slot*3] = (0x03 << 8) | ((addr & 0xFF0000) >> 16); // write cmd and top 8 bits of 24 bit addr
    tx_buffer[slot*3+1] = (addr & 0xFFFF); // bottom 16 bits of address
    tx_buffer[slot*3+2] = 0;

}
uint16_t mem_get_read(int slot)
{
    return rx_buffer[slot*3+2];
}
bool mem_done()
{
    return mem_done_flag;
}
void mem_start(int n_slots)
{   
    BlueDebugLead::Set();
    total_slots = n_slots;
    slots_done = 0;
    mem_done_flag = false;
    _mem_start_dma();
}
