#pragma once
#include "gpio.h"
// P. Clock is 84MHz

using GpioA = Gpio<GPIOA_BASE>;
using GpioB = Gpio<GPIOB_BASE>;
using GpioC = Gpio<GPIOC_BASE>;
using GpioD = Gpio<GPIOD_BASE>;
using GpioE = Gpio<GPIOE_BASE>;

using MainLED = GpioA::Port<1>;

using DebugPin = GpioE::Port<1>;

using PurpleDebugLead = GpioE::Port<11>;
using BlueDebugLead = GpioB::Port<12>;

using Dials_A = GpioD::Port<0>;
using Dials_B = GpioD::Port<2>;
using Dials_C = GpioD::Port<4>;

using Panel1_LED1 = GpioD::Port<8>;
using Panel2_LED1 = GpioB::Port<15>;

using Panel1_LED2 = GpioD::Port<10>;
using Panel2_LED2 = GpioD::Port<9>;


using Panel1_Switch = GpioD::Port<12>;
using Panel2_Switch = GpioD::Port<11>;

/*
// On proto board
using Panel4_Switch = GpioE::Port<12>;
using Panel4_LED2 = GpioE::Port<14>;
using Panel4_LED1 = GpioB::Port<10>;

using Panel3_Switch = GpioE::Port<11>;
using Panel3_LED2 = GpioE::Port<13>;
using Panel3_LED1 = GpioE::Port<15>;
*/


// On new board
using Panel4_Switch = GpioB::Port<6>;
using Panel4_LED2 = GpioB::Port<7>;
using Panel4_LED1 = GpioB::Port<8>;

using Panel3_Switch = GpioD::Port<5>;
using Panel3_LED1 = GpioD::Port<6>;
using Panel3_LED2 = GpioD::Port<7>;

