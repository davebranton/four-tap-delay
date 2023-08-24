#pragma once

#include "stm32f4xx_conf.h"


enum PortOutputSpeed {
  eLowSpeed,eMediumSpeed,eHighSpeed,eVeryHighSpeed
};

template <uint32_t iaddr>
class Gpio
{
  public:
    template <int i>
    static void Output() { Output(i); }
    static void Output(int i)
    {
      ((GPIO_TypeDef *)(iaddr))->MODER &= ~(1 << (1 + i * 2));
      ((GPIO_TypeDef *)(iaddr))->MODER |= (1 << (i * 2));
    }
    template <int i>
    static void Input() { Input(i); }
    static void Input(int i)
    {
      ((GPIO_TypeDef *)(iaddr))->MODER &= ~(1 << (1 + i * 2));
      ((GPIO_TypeDef *)(iaddr))->MODER &= ~(1 << (i * 2));
    }
    template <int i>
    static bool Value() { return Value(i); }
    static bool Value(int i)
    {
      return ((GPIO_TypeDef *)(iaddr))->ODR & (1 << i) != 0;
    }
    template <int i>
    static void Set() { Set(i); }
    static void Set(int i)
    {
      ((GPIO_TypeDef *)(iaddr))->ODR |= (1 << i);
    }
    template <int i>
    static void Reset() { Reset(i); }
    static void Reset(int i)
    {
      ((GPIO_TypeDef *)(iaddr))->ODR &= ~(1 << i);
    }
    template <int i>
    static void Toggle() { Toggle(i); }
    static void Toggle(int i)
    {
      ((GPIO_TypeDef *)(iaddr))->ODR ^= (1 << i);
    }

    template<int i>
    class Port
    {
    public:
      static void Analog()
      {
        ((GPIO_TypeDef *)(iaddr))->MODER |= (1 << (1 + i * 2));
        ((GPIO_TypeDef *)(iaddr))->MODER |= (1 << (i * 2));
      }
      static void Output()
      {
        ((GPIO_TypeDef *)(iaddr))->MODER &= ~(1 << (1 + i * 2));
        ((GPIO_TypeDef *)(iaddr))->MODER |= (1 << (i * 2));
      }
      static void Input()
      {
        ((GPIO_TypeDef *)(iaddr))->MODER &= ~(1 << (1 + i * 2));
        ((GPIO_TypeDef *)(iaddr))->MODER &= ~(1 << (i * 2));
      }
      static void Pullup()
      {
        ((GPIO_TypeDef *)(iaddr))->PUPDR &= ~(1 << (1 + i * 2));
        ((GPIO_TypeDef *)(iaddr))->PUPDR |= (1 << (i * 2));
      }
      static bool Read()
      {
        return (((GPIO_TypeDef *)(iaddr))->IDR & (1 << i)) != 0;
      }
      static void Set()
      {
        ((GPIO_TypeDef *)(iaddr))->ODR |= (1 << i);
      }
      static void Reset()
      {
        ((GPIO_TypeDef *)(iaddr))->ODR &= ~(1 << i);
      }
      static void Toggle()
      {
        ((GPIO_TypeDef *)(iaddr))->ODR ^= (1 << i);
      }
      static void OutputSpeed(PortOutputSpeed outs)
      {
        auto tmp = ((GPIO_TypeDef *)(iaddr))->OSPEEDR;
        tmp &= ~(0x03 << (i*2));
        tmp |= (outs & 0x03) << (i*2);
        ((GPIO_TypeDef *)(iaddr))->OSPEEDR = tmp;
      }
      static void AlternateFunction(uint8_t function)
      {
        if (i < 8)
        {
          ((GPIO_TypeDef *)(iaddr))->AFR[0] &= ~(uint32_t(0xF) << (i*4));
          ((GPIO_TypeDef *)(iaddr))->AFR[0] |= (uint32_t(function & 0x0F) << (i*4));
        }
        else
        {
          ((GPIO_TypeDef *)(iaddr))->AFR[1] &= ~(uint32_t(0xF) << ((i-8)*4));
          ((GPIO_TypeDef *)(iaddr))->AFR[1] |= (uint32_t(function & 0x0F) << ((i-8)*4));
        }
        // alternate function mode
        ((GPIO_TypeDef *)(iaddr))->MODER &= ~(1 << (i * 2));
        ((GPIO_TypeDef *)(iaddr))->MODER |= (1 << (1 + i * 2));

      }
    };
};