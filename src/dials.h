
#include <stdint.h>
#include <array>

#pragma once

#include "stm32f4xx_conf.h"
#include "ports.h"

int16_t Cal(int32_t uncal_value)
{
  // Maybe due to buffers, we see about 50 - 4060 instead of 0 - 4096
  uncal_value -= 50;
  if (uncal_value < 0) return 0;
  uncal_value = (uncal_value * 4010) / 4046;
  if (uncal_value > 0xFFF) return 0xFFF;
  return uncal_value;
}

template<int bits>
class MovingAverageFilter
{
  int i = 0;
  int16_t v[1 << bits];
  int32_t total = 0;
  int32_t frozen_total = 0;
  
  public: 
  int midi_value = -1;
  volatile int16_t last_v;
  MovingAverageFilter()
  {
    for(int i = 0; i < (1<<bits); i++) v[i] = 0;
  }
  void SetMidiValue(int16_t mv) {
    frozen_total = total;
    midi_value = mv;
  }
  int16_t GetValue() { 
    if (midi_value == -1)
    {
      return (total >> bits);
    }
    else
    {
      if (frozen_total > 0 && abs(frozen_total - total) > (3 << bits))
      {
        midi_value = -1;
        return (total >> bits);
      }
      return  midi_value;
    }
  }
  void NewValue(int16_t _v) { 
    last_v = _v;
    total += _v;
    total -= v[i];
    v[i] = _v; 
    i++; if (i==(1<<bits)) i = 0;
  }
};

using DialFilter = MovingAverageFilter<8>;


DialFilter output_volume;
DialFilter input_volume;
std::array<std::array<DialFilter,5>,4> dial_panel;

template <typename A, typename B, typename C>
class DialBank
{
public:

  uint8_t index = 0;

  void Setup()
  {
    A::Output();
    B::Output();
    C::Output();
  }

  void SetBits()
  {
    auto _index = index;
    if (_index & 1) A::Set(); else A::Reset();
    if (_index & 2) B::Set(); else B::Reset();
    if (_index & 4) C::Set(); else C::Reset();
  }

  void Next()
  {
    index++;
    if (index == 5) index = 0;
    SetBits();
  }
};

volatile uint16_t DMA_values[8];
DialBank<Dials_A, Dials_B, Dials_C> dial_counter;

void init_dial_readers()
{  
  GpioE::Port<0>::Output();

  dial_counter.Setup();

  // Power on the ADC for the dials  
  ADC_InitTypeDef adc_init;
  adc_init.ADC_Resolution = ADC_Resolution_12b;
  adc_init.ADC_ScanConvMode = ENABLE;
  adc_init.ADC_ContinuousConvMode = ENABLE;
  adc_init.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
  adc_init.ADC_ExternalTrigConv = 0;
  adc_init.ADC_DataAlign = ADC_DataAlign_Right;
  adc_init.ADC_NbrOfConversion = 8;
  ADC_Init(ADC1, &adc_init);

  // 
  ADC1->CR2 |= ADC_CR2_DDS;

  DMA_values[0] = 0;
  DMA_values[1] = 0;
  DMA_values[2] = 0;
  DMA_values[3] = 0;
  DMA_values[4] = 0;
  DMA_values[5] = 0;
  DMA_values[6] = 0;
  DMA_values[7] = 0;

  // Seven channels in order

  GpioB::Port<0>::Analog(); // ADC_Channel_8
  GpioA::Port<2>::Analog(); // ADC_Channel_2
  GpioB::Port<1>::Analog(); // ADC_Channel_9
  GpioA::Port<3>::Analog(); // ADC_Channel_3
  GpioA::Port<7>::Analog(); // ADC_Channel_7
  GpioA::Port<6>::Analog(); // ADC_Channel_6
  ADC_RegularChannelConfig(ADC1, ADC_Channel_Vbat, 1, ADC_SampleTime_144Cycles); // this one here so we ignore transition to the next multiplex output
  ADC_RegularChannelConfig(ADC1, ADC_Channel_Vbat, 2, ADC_SampleTime_144Cycles); // this one here so we ignore transition to the next multiplex output
  ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 3, ADC_SampleTime_144Cycles); //  PB0
  ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 4, ADC_SampleTime_144Cycles); //  PA2
  ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 5, ADC_SampleTime_144Cycles); //  PB1
  ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 6, ADC_SampleTime_144Cycles); //  PA3
  ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 7, ADC_SampleTime_144Cycles); //  PA7 // in vol
  ADC_RegularChannelConfig(ADC1, ADC_Channel_7, 8, ADC_SampleTime_144Cycles); //  PA6 // out vol

  // Common init for the clock
  ADC_CommonInitTypeDef adc_common_init;
  ADC_CommonStructInit(&adc_common_init);
  adc_common_init.ADC_Prescaler = ADC_Prescaler_Div8;
  // setting this stops the EOC interrupt for some reason
  //adc_common_init.ADC_DMAAccessMode = ADC_DMAAccessMode_1;
  ADC_CommonInit(&adc_common_init);

  // Turn on DMA
  // DMA 2 Stream 4 set to Channel 0 for ADC1
  
  DMA_InitTypeDef dma;
  dma.DMA_Channel = DMA_Channel_0;
  dma.DMA_PeripheralBaseAddr = (uint32_t)&(ADC1->DR);
  dma.DMA_Memory0BaseAddr = (uint32_t)DMA_values;
  dma.DMA_DIR = DMA_DIR_PeripheralToMemory;
  dma.DMA_BufferSize = 8; 
  dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  dma.DMA_Mode = DMA_Mode_Circular;
  dma.DMA_Priority = DMA_Priority_Medium;
  dma.DMA_FIFOMode = DMA_FIFOMode_Disable;
  dma.DMA_FIFOThreshold = 0;
  dma.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  dma.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

  DMA_Init(DMA2_Stream4, &dma);
  DMA_Cmd(DMA2_Stream4, ENABLE);
  ADC_DMACmd(ADC1, ENABLE); // enabling DMA stop the EOC interrupt??

  // transfer complete and also half complete
  DMA2_Stream4->CR |=  DMA_SxCR_TCIE | DMA_SxCR_HTIE;

  NVIC_EnableIRQ(DMA2_Stream4_IRQn);  
  NVIC_SetPriority(DMA2_Stream4_IRQn, 2);
  ADC_Cmd(ADC1, ENABLE);
  //ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
  ADC_SoftwareStartConv(ADC1); // ADC1 is in continous mode, and will convert from now on.
GpioE::Port<0>::Reset();

}

// Must be volatile or some kind of caching kills us
volatile uint16_t read_dials[6];

extern "C" void DMA2_Stream4_IRQHandler()
{
  //GpioE::Port<0>::Set();
  if (DMA2->HISR & DMA_HISR_TCIF4)
  {
    // complete
    GpioE::Port<0>::Set();
    auto counterval = dial_counter.index;
    dial_counter.Next();
    read_dials[2] = DMA_values[4];
    read_dials[3] = DMA_values[5];
    read_dials[4] = DMA_values[6];
    read_dials[5] = DMA_values[7];
    DMA2->HIFCR |= DMA_HIFCR_CTCIF4;   

    // We get in here at ~20kHz
    
    // We now have a set of analog reads, from the dials
    for(int i = 0; i < 4; i++)
    {
      // they're all backwards... so subtrack from 4095
      dial_panel[i][counterval].NewValue(4095 - read_dials[i]);
    }
    
    // and the volumes
    input_volume.NewValue(read_dials[4]);    
    output_volume.NewValue(read_dials[5]);
    
    GpioE::Port<0>::Reset();
  }
  if (DMA2->HISR & DMA_HISR_HTIF4)
  {
    // half complete
    //read_dials[0] = DMA_values[0]; don't need this value
    read_dials[0] = DMA_values[2];
    read_dials[1] = DMA_values[3];
    DMA2->HIFCR |= DMA_HIFCR_CHTIF4;
  }
  //GpioE::Port<0>::Reset();
  
}
