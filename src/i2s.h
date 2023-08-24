#pragma once
#include "stm32f4xx_conf.h"
#include "ports.h"

void _16_bit_I2S_Init_output()
{
    I2S_InitTypeDef I2;

    I2.I2S_AudioFreq =  I2S_AudioFreq_44k;
    I2.I2S_CPOL = I2S_CPOL_High; // data is sampled on rising clock
    I2.I2S_DataFormat = I2S_DataFormat_24b;
    I2.I2S_MCLKOutput = I2S_MCLKOutput_Enable; // do not need a master clock, DAC derives it from bit clock
    I2.I2S_Mode = I2S_Mode_MasterTx;
    I2.I2S_Standard = I2S_Standard_Phillips;


    // Serial data is clocked into the PCM510xA on the rising edge of BCK.
    
    // We want 44.1kHz at 16-bit stereo
    // 1,411,200 Hz

    // PLLI2S is at this point = 96 Mhz

    I2S_Init(SPI2, &I2);

    // Enale interrupt
    SPI2->CR2 |= SPI_CR2_TXEIE;

    NVIC_SetPriority(SPI2_IRQn, 0);
    NVIC_EnableIRQ(SPI2_IRQn);  

    I2S_Cmd(SPI2, ENABLE);

    // PLLI2S is at this point = 96 Mhz

    // Select pins

    // I2S2 (SPI2) uses 
    // SPI1_MOSI (as Serial Data)
    // SPI1_NSS (as Word Select)
    // SPI1_SCK (as Clock)

    // SPI2_MOSI is on 
    // PB15 (as Alternate Function 5) 
    // PC3 (as Alternate Function 5)     *

    // SPI2_NSS is on
    // PB9  (as Alternate Function 5)  *
    // PB12 (as Alternate Function 5) 

    // SPI2_SCK is on
    // PB10 (as Alternate Function 5) 
    // PB13 (as Alternate Function 5) *

    GpioC::Port<3>::AlternateFunction(5);
    GpioB::Port<9>::AlternateFunction(5);
    GpioB::Port<13>::AlternateFunction(5);



}


union ADCValue {
    int32_t val;
    struct 
    {
        uint16_t l;
        uint16_t h;
    };
};

volatile ADCValue dac_send_buffer[16];
volatile int dac_send_buffer_write_pos = 0;
volatile int dac_send_buffer_read_pos = 8;

volatile int dac_send_value_frame = 0;
volatile bool dac_was_left_side = false;

// This function seems delicate. It can stop working if it gets rearranged
// slightly. First place to look if the output stops working.
extern "C" void SPI2_IRQHandler()
{
    int16_t sendval = 0;

    bool is_left_side = (SPI2->SR & SPI_SR_CHSIDE) == SPI_SR_CHSIDE;
    // if side changed, then reset dac_send_value_frame
    if (is_left_side != dac_was_left_side)
    {
        // output. high first
        SPI_I2S_SendData(SPI2, dac_send_buffer[dac_send_buffer_read_pos].h);
    }
    else
    {
        // then low
        SPI_I2S_SendData(SPI2, dac_send_buffer[dac_send_buffer_read_pos].l);
        if (is_left_side)
        {
            // every other transition, increment the buffer.
            dac_send_buffer_read_pos++;
            dac_send_buffer_read_pos %= 16;
        }
    }
        
    dac_was_left_side = is_left_side;
}


void QueueDACOutput(int32_t val)
{
    NVIC_DisableIRQ(SPI2_IRQn);  

    // left side
    dac_send_buffer[dac_send_buffer_write_pos].val = val;
    dac_send_buffer_write_pos++;
    dac_send_buffer_write_pos%=16;

    NVIC_EnableIRQ(SPI2_IRQn);  
}


void _16_bit_I2S_Init_input()
{
    I2S_InitTypeDef I3;

    I3.I2S_AudioFreq = I2S_AudioFreq_44k;
    I3.I2S_CPOL = I2S_CPOL_High; // data is sampled on rising clock
    I3.I2S_DataFormat = I2S_DataFormat_24b;
    I3.I2S_MCLKOutput = I2S_MCLKOutput_Enable; // need a master clock
    I3.I2S_Mode = I2S_Mode_MasterRx;

    // Phillips standard - first bit is ignored
    I3.I2S_Standard = I2S_Standard_Phillips;


    // We want 44.1kHz at 16-bit stereo
    // 1,411,200 Hz

    // PLLI2S is at this point = 96 Mhz

    I2S_Init(SPI3, &I3);

    // Enale interrupt
    SPI3->CR2 |= SPI_CR2_RXNEIE;

    NVIC_SetPriority(SPI3_IRQn, 0);
    NVIC_EnableIRQ(SPI3_IRQn);  

    I2S_Cmd(SPI3, ENABLE);

    // Sel pins
    GpioA::Port<4>::AlternateFunction(6); // PA4 - WS
    GpioC::Port<12>::AlternateFunction(6); // PC12 - Data 
    GpioC::Port<7>::AlternateFunction(6); // Master Clock
    GpioC::Port<10>::AlternateFunction(6); // Bit Clock



}


volatile ADCValue adc_receive_value;
volatile bool adc_receive_flag = false;
int adc_receive_value_frame = 0;
volatile bool adc_was_left_side = false;
extern "C" void SPI3_IRQHandler()
{
    

    auto rxval =  SPI_I2S_ReceiveData(SPI3);

    bool is_left_side = (SPI3->SR & SPI_SR_CHSIDE) == SPI_SR_CHSIDE;
    if (is_left_side)
    {
        // if side changed, time to receive data
        if (is_left_side != adc_was_left_side)
        {
            adc_receive_value.h = rxval;            
        }
        else
        {
            adc_receive_value.l = rxval;
            adc_receive_flag = true;
        }
    }
    adc_was_left_side = is_left_side;

}
