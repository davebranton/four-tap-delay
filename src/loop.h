
#pragma once
#include <stdint.h>
#include "dials.h"
#include "xfade_coeffs.h"

template <typename Button, typename LED, int slot>
class loop
{
public:
  bool recording = 0;
  int debounce = 0;
  int32_t recording_address = 0;
  int32_t record_length = 0;
  int32_t playback_address = 0;
  static constexpr int crossfade_buf_len = sizeof(xfade_coeffs);
  bool first_loop = false;

  static constexpr int32_t base_addr = (0x800000 / 4) * slot;
  static constexpr int32_t max_size = 0x800000 / 8; // 2 bytes per sample, 4 banks

  int16_t DoLoop(int16_t input_sample, int midi_command)
  {
    // RECORDING
    if (debounce > 0)
    {
      if (Button::Read())
      {
        debounce--;
      }
      else
      {
        debounce = 500;
      }
    }
    // HANDLE BUTTON
    if (!recording)
    {
      // not recording.
      if (midi_command == slot || (debounce == 0 && !Button::Read())) // switch is down
      {
        debounce = 500;
        recording = true;
        record_length = 0;
        recording_address = 0;
      }
    }
    else
    {
      // recording.
      if (midi_command == slot || (debounce == 0 && !Button::Read())) // switch is down
      {
        // stop
        debounce = 500;
        recording = false;
        record_length = recording_address;
        playback_address = 0;
        first_loop = true;
      }
    }

    int16_t recorded_sample = 0;

    if (recording)
    {
      // write to memory
      if (recording_address < crossfade_buf_len)
      {
        // Write the samples for the crossfade into the buffer
        // multiplying by the crossfade gain as we go. So when we come to playback, this will fade in.
        input_sample = int16_t((int32_t(input_sample) * int32_t(xfade_coeffs[recording_address])) / crossfade_buf_len);
      }
      mem_sched_write(slot, base_addr + recording_address*2, input_sample);
      recording_address ++;
      if (recording_address >= max_size)
      {
        // out of space
        recording = false;
        record_length = recording_address;
        playback_address = 0;
        first_loop = true;
      }
      // flash LED
      if ((recording_address >> 12) & 0x01)
      {
        LED::Set();
      }
      else
      {
        LED::Reset();
      }
    }
    else
    {
      if (record_length != 0)
      {
        // there's a playback
        if (first_loop)
        {
          first_loop = false;
        }
        else
        {
          // We're reading from playback address - 1 right here. Not playback_address, which we'll read next.
          // If playback_address was zero, then what we've just read was actually from the end of the buffer.
          auto playback_address_we_just_read = playback_address - 1;
          if (playback_address_we_just_read < 0)
          {
            playback_address_we_just_read = record_length - 1;
          }
          int32_t _samp = ((int16_t)mem_get_read(slot)); // cast this twice to make sure the sign is extended
          if (playback_address_we_just_read >= record_length - crossfade_buf_len)
          {
            // x-fade
            // crossfade_point goes from zero (when playback_address == record_length - crossfade_buf_len)
            // to crossfade_buf_len-1 when playback_address == record_length - 1
            int16_t crossfade_point = playback_address_we_just_read - (record_length - crossfade_buf_len);

            // Sample is faded out. 
            _samp *= (int32_t)xfade_coeffs[(crossfade_buf_len - 1) - crossfade_point]; // first value of this is zero.
            _samp /= (int32_t)crossfade_buf_len;
          }
          recorded_sample = (_samp * (Cal(dial_panel[slot][0].GetValue()) >> 4)) / 256;
        }
        mem_sched_read(slot, base_addr + playback_address*2);
        playback_address ++ ;
        if (playback_address == record_length)
        {
          playback_address = 0;
        }
        // Flash LED
        if (playback_address < 500)
        {
          LED::Set();
        }
        else
        {
          LED::Reset();
        }
      }
    }
    return recorded_sample;
  }
};