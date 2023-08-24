#include "lowpass.h"
#include <iostream>
int main(int argc, char** argv)
{
    std::cout << "struct Coeffs \n";
    std::cout << "{\n";
    std::cout << "  float m_Den1, m_Num0, m_Num1;\n";
    std::cout << "};\n";
    std::cout << "const Coeffs filter_coeffs[] = {\n";
    for(int i = 0; i < 4096; i++)
    {
        float cutoff = 500.0f + float(double(i)* (20000.0 - 500.0) / 4095.0);
        CFirstOrderLowPassFilter<float> lp;
        lp.SetCutoffFrequency(cutoff / 44100.0f);
        std::cout << "{" << lp.m_Den1 << "," << lp.m_Num0 << "," << lp.m_Num1 << "},\n";
    }
    std::cout << "{0,0,0}};\n";
    return 0;
}