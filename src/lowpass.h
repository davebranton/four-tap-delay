

#pragma once
#include <math.h>
//-----------------------------------------------------------------------------

template<typename numtype>
class CFirstOrderLowPassFilter
{
public:

  numtype m_Num0{};
  numtype m_Num1{};
  numtype m_Den1{};
  numtype m_LastY{};
  numtype m_LastX{};
  bool m_bPrime{};

  
  //-----------------------------------------------------------------------------
  void Preload(numtype value)
  {
    m_LastY = value;
    m_LastX = value;
  }

  //-----------------------------------------------------------------------------
  void SetCutoffFrequency(numtype cutoffFrequencyOverSamplingFrequency)
  {
    numtype alpha;
    numtype B;
    numtype A;
    numtype Pi(3.141592654);
    numtype Quarter(0.25);

    B = Pi * (Quarter - cutoffFrequencyOverSamplingFrequency);
    A = Pi * (Quarter + cutoffFrequencyOverSamplingFrequency);


    alpha = sin(B) / sin(A);
    m_Num0 = m_Num1 = ((numtype(1.0) - alpha) / numtype(2.0));
    m_Den1 = alpha;
  }

  //-----------------------------------------------------------------------------
  numtype AddSample(numtype sample)
  {
    // % y(n) = a*x(n) + b*x(n-1) - alpha*y(n-1);
    numtype dOutput = (m_Num0*sample) + (m_Num1*m_LastX) + (m_Den1*m_LastY);
    m_LastX = sample;
    m_LastY = dOutput;

    return m_LastY;
  }

  //-----------------------------------------------------------------------------
  numtype GetOutput() const
  {
    return m_LastY;
  }
};
