#pragma once
#include <limits>

template <typename input_type, typename output_type>
output_type Clamp(input_type v)
{
  if (v > std::numeric_limits<output_type>::max())
  {
    return std::numeric_limits<output_type>::max();
  }
  if (v < std::numeric_limits<output_type>::min())
  {
    return std::numeric_limits<output_type>::min();
  }
  return v;
}
