

#pragma once
#include <stdint.h>
#include "lowpass.h"
#include "clamp.h"
#include "ports.h"


class FP_DialFilter
{
  public:
  CFirstOrderLowPassFilter<float> filt;
  FP_DialFilter()
  {
    filt.SetCutoffFrequency(1.0f / 11000.0f);
  }
};

#define MIN_DELAY 4000

template<uint16_t delay_size_bits>
class Delay
{
public:

  constexpr int DelaySize() { return 1 << delay_size_bits; }

  Delay()
  {
    for(int i = 0; i < DelaySize(); i++) audio_buffer[i] = 0;
    for(int i = 0; i < 4; i++)
    {
      taps[i].filter.SetCutoffFrequency(22000.0f / 44000.0f);
    }
  }

  int16_t audio_buffer[1 << delay_size_bits];
  uint32_t write_head_position = 0;

  struct Tap
  {
    FP_DialFilter dial_filter_1;
    void SetRate(int16_t dial_value)
    {
      auto val = dial_filter_1.filt.AddSample(float(dial_value) / 4096.0f);
      //auto val = float(dial_value) / 4096.0f;
      rate = MIN_DELAY + int(float((1 << delay_size_bits) - MIN_DELAY) * val + 0.5f);
    }
    uint16_t rate = (1 << delay_size_bits) / 2;
    uint16_t counter_led = 0;
    uint16_t fb = 0;  // 12 bit integer
    uint16_t level = 0; // 12 bit integer
    CFirstOrderLowPassFilter<float> filter;
  };

  Tap taps[4];

  bool LedOn(int tap)
  {
    taps[tap].counter_led++;
    if (taps[tap].counter_led >= taps[tap].rate)
    {
      taps[tap].counter_led = 0;
    }
    return (taps[tap].counter_led < 500 );
  }

  // Add a signed integer value.
  // If bWrite is false, save zero to the buffer
  int16_t AddSample(int32_t sample)
  {
    int32_t output_sample = sample;

    // feed the LEDs
    if (LedOn(0)) Panel1_LED1::Set(); else Panel1_LED1::Reset();
    if (LedOn(1)) Panel2_LED1::Set(); else Panel2_LED1::Reset();
    if (LedOn(2)) Panel3_LED1::Set(); else Panel3_LED1::Reset();
    if (LedOn(3)) Panel4_LED1::Set(); else Panel4_LED1::Reset();

    // before the write apply the feedback to what we're going 
    // to write
    for(int i = 3; i >=0; i--)
    {
      // This is a signed value
      int32_t delay_sample = audio_buffer[(write_head_position + taps[i].rate) % DelaySize()];
      // filter it
      // feedback, add the feedback to the sample
      sample += int(delay_sample * taps[i].fb) / 8192; // fb is a 12-bit value, but the max feedback is just half this

      // level of filtered output
      output_sample += int(taps[i].filter.AddSample(delay_sample) * taps[i].level) / 4096;
      //output_sample += int(delay_sample * taps[i].level) / 4095;
      
    }
    // clip and write to buffer.
    audio_buffer[write_head_position] = Clamp<int32_t, int16_t>(sample);
    // Advance loop
    if (write_head_position == 0)
    {
      write_head_position = DelaySize() - 1;
    }
    else
    {
      write_head_position--;
    }
    return Clamp<int32_t, int16_t>(output_sample);
  }
};