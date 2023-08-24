#include <iostream>
#include <stdint.h>
#include <cmath>


int main(int argc, char**argv)
{
    std::cout << "static const uint8_t xfade_coeffs[256] = {\n";
    for(int i = 0; i < 256; i++)
    {
        double x = double(i) * 4.0 / 256.0;
        double y = 1.0 / (1.0+exp(-2.8*(x-2.0)));

        std::cout << int(y*256.0) << ",\n";
    }
    std::cout << "};\n";
    return 0;
}