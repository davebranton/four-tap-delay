#include "stm32f4xx_conf.h"
#include "ports.h"
#include "lowpass.h"
#include "i2s.h"
#include "delay.h"
#include "sin.h"
#include "dials.h"
#include "mem_dma.h"
#include "filter_coeffs.h"
#include "loop.h"
#include "midi.h"
#include <limits>

int main(void)
{
  // called by the .s code
  //SystemInit();

    // 32768 @ 44.1kHz is somewhat less than 1s
  // this is 1.5s

  

  Delay<15> delay; // ~128k of RAM required


  auto hse = HSE_VALUE;

  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // enable the clock to GPIOA
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // enable the clock to GPIOB
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // enable the clock to GPIOC
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // enable the clock to GPIOD
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN; // enable the clock to GPIOE

  RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; // Enable SPI1
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN; // Enable USART1
  RCC->APB1ENR |= RCC_APB1ENR_SPI2EN; // Enable SPI2
  RCC->APB1ENR |= RCC_APB1ENR_SPI3EN; // Enable SPI3
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN; // Enable ADC1 clock

  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN; // Enable DMA2

  DebugPin::Output();

  GpioC::Port<0>::Output();

  MainLED::Output();
  MainLED::Reset();

  // Panel LEDs
  Panel1_LED1::Output();
  Panel2_LED1::Output();
  Panel3_LED1::Output();
  Panel4_LED1::Output();
  Panel1_LED2::Output();
  Panel2_LED2::Output();
  Panel3_LED2::Output();
  Panel4_LED2::Output();

  // Switches
  Panel1_Switch::Input();
  Panel2_Switch::Input();
  Panel3_Switch::Input();
  Panel4_Switch::Input();
  Panel1_Switch::Pullup();
  Panel2_Switch::Pullup();
  Panel3_Switch::Pullup();
  Panel4_Switch::Pullup();

  int n = 0;
  
  mem_init();
  
  init_dial_readers();

  _16_bit_I2S_Init_output();
  _16_bit_I2S_Init_input();

  init_midi();

  // mem test
  volatile int errors = 0;
  uint32_t max_test_addr =  0x3FFFF;

  if (!Panel1_Switch::Read())
  {
    max_test_addr = 0x800000; // full memory test
  }

  for(volatile uint32_t addr = 0; addr < max_test_addr; addr+=2)
  {
    n++;
    if (addr > max_test_addr/5)
    {
      Panel1_LED1::Set();
      Panel1_LED2::Set();
    }
    if (addr > 2*(max_test_addr/5))
    {
      Panel2_LED1::Set();
      Panel2_LED2::Set();
    }
    if (addr > 3*(max_test_addr/5))
    {
      Panel3_LED1::Set();
      Panel3_LED2::Set();
    }
    if (addr > 4*(max_test_addr/5))
    {
      Panel4_LED1::Set();
      Panel4_LED2::Set();
    }
    while (!mem_done());
    if (addr >= 4)
    {
      auto x = mem_get_read(1);
      if (x == ((addr - 4) & 0xFFFF))
      {
        if (n > 5000)
        {
          MainLED::Toggle();
          n = 0;
        }
      }
      else
      {
        MainLED::Reset();
        Panel1_LED1::Set();
        Panel2_LED1::Set();
        Panel3_LED1::Set();
        Panel4_LED1::Set();
        Panel1_LED2::Set();
        Panel2_LED2::Set();
        Panel3_LED2::Set();
        Panel4_LED2::Set();
        while(true);
      }
    }
    mem_sched_write(0, addr, addr & 0xFFFF);
    mem_sched_read(1, addr - 2);
    mem_start(2);
  }

  while (!mem_done());

  //#define POT_OUTPUT

  //dials go from 20mV to 1.5V 
  //50 ms to output all dials (other than volume controls)
  //dips to -1.5V inbetween outputs

  Panel1_LED2::Reset();
  Panel2_LED2::Reset();
  Panel3_LED2::Reset();
  Panel4_LED2::Reset();  

  PurpleDebugLead::Output();
  BlueDebugLead::Output();

  // MIDI mappings
  DialFilter* midi_mappings[] = 
  {
    &dial_panel[0][0],
    &dial_panel[0][1],
    &dial_panel[0][2],
    &dial_panel[0][3],
    &dial_panel[0][4],
    &dial_panel[1][0],
    &dial_panel[1][1],
    &dial_panel[1][2],
    &dial_panel[1][3],
    &dial_panel[1][4],
    &dial_panel[2][0],
    &dial_panel[2][1],
    &dial_panel[2][2],
    &dial_panel[2][3],
    &dial_panel[2][4],
    &dial_panel[3][0],
    &dial_panel[3][1],
    &dial_panel[3][2],
    &dial_panel[3][3],
    &dial_panel[3][4],
    &output_volume,
    &input_volume
  };


  #ifdef POT_OUTPUT
  volatile int bank = 0;
  volatile int dial = 0;
  volatile int pot_output_divider = 1000;
  #endif

  loop<Panel1_Switch, Panel1_LED2, 0> L1;
  loop<Panel2_Switch, Panel2_LED2, 1> L2;
  loop<Panel3_Switch, Panel3_LED2, 2> L3;
  loop<Panel4_Switch, Panel4_LED2, 3> L4;

  int16_t recorded_sample = 0;
  uint8_t dial_update_index = 0;

  uint16_t midi_dials[20];

  volatile int32_t xx = 0;

  while (1)
  {
    while(!adc_receive_flag)
    {
    }
    adc_receive_flag = false;

    DebugPin::Set();

    int64_t adc = adc_receive_value.val >> 8;
    // scale integer by volume value
    // only need gain from 0 to 256. Any more than that
    // and we'll just be pushing the LSB of the input up and zero-padding. Which is pointless.
    int volume = input_volume.GetValue() >> 4; 
    adc = (adc * volume) >> 12;

    auto input_sample = Clamp<int32_t,  int16_t>(adc);

    auto val = delay.AddSample(input_sample + recorded_sample);
    #ifdef POT_OUTPUT
    pot_output_divider--;
    if (pot_output_divider == 0)
    {
      dial++;
      pot_output_divider = 100;
      if (dial == 5)
      {
        bank++;
        dial = 0;
        if (bank == 5)
        {
          bank = 0;
        }
      }
    }
    if (bank == 4)
    {
      QueueDACOutput(-1073741824);
    }
    else
    {
      QueueDACOutput(dial_panel[bank][dial].GetValue() << 18);
    }
    #else
    ADCValue output_val;

    // 16 bit value, shift up to 24 bit, which is max output volume
    adc = val << 8;
    // scale integer by volume value
    volume = output_volume.GetValue();
    adc = (adc * volume) >> 4;  // shift down by 12, then back up by 8 leaves a shift of 4
    output_val.val = Clamp<int64_t, int32_t>(adc);
    QueueDACOutput(output_val.val);
    //QueueDACOutput( (int32_t)dial_panel[0][1].GetValue() * 1024 );
    //QueueDACOutput( engine.delay.taps[0].fb );
    //QueueDACOutput((dial_panel[0][3].GetValue() - dial_panel[0][3].last_v) * volume * 4096);
    #endif
    int midi_command = -1;

    // midi commands
    NVIC_DisableIRQ(USART1_IRQn);  
    if (rx_buf[0] == 0xC0 && rx_buf_at == 2)
    {
      // PC message
      rx_buf_at = 0;
      midi_command = rx_buf[1];
    }
    else if (rx_buf[0] == 0xB0 && rx_buf_at == 3)
    {
      // CC message
      rx_buf_at = 0;
      if (rx_buf[1] < sizeof(midi_mappings))
      {
        midi_mappings[rx_buf[1]]->SetMidiValue(31 + (rx_buf[2] << 5));
      }
      
    }
    NVIC_EnableIRQ(USART1_IRQn);  

    while (!mem_done());
    PurpleDebugLead::Set();
    recorded_sample = L1.DoLoop(input_sample, midi_command);
    recorded_sample += L2.DoLoop(input_sample, midi_command);
    recorded_sample += L3.DoLoop(input_sample, midi_command);
    recorded_sample += L4.DoLoop(input_sample, midi_command);

    PurpleDebugLead::Reset();
    mem_start(4); 

    delay.taps[dial_update_index].SetRate(Cal(dial_panel[dial_update_index][4].GetValue()));
    delay.taps[dial_update_index].fb = Cal(dial_panel[dial_update_index][3].GetValue());
    delay.taps[dial_update_index].level = Cal(dial_panel[dial_update_index][2].GetValue());
    auto filter_selection = dial_panel[dial_update_index][1].GetValue();
    delay.taps[dial_update_index].filter.m_Den1 = filter_coeffs[filter_selection].m_Den1;
    delay.taps[dial_update_index].filter.m_Num0 = filter_coeffs[filter_selection].m_Num0;
    delay.taps[dial_update_index].filter.m_Num1 = filter_coeffs[filter_selection].m_Num1;
    dial_update_index = (dial_update_index+1)&3;

    n++;
    if (n > 40000)
    {
      MainLED::Toggle();
      n = 0;
    }

    DebugPin::Reset();
  }
}