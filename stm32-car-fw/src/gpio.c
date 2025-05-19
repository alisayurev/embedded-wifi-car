/*
* gpio.c
* 
* Implementation of a GPIO control library for the STM32L432KC. Provides functions
* to control and configure GPIO. Provided as a starter file for EE14 Sprinf 2025
*
* Prof. Steven Bell <sbell@ece.tufts.edu>, EE14 Spring 2025
* Edited: Alisa Yurevich, edited comments
*/

#include "ee14lib.h"

// Mapping of Nucleo pin number to GPIO port
static GPIO_TypeDef * g_GPIO_port[D13+1] = {
  GPIOA,GPIOA,GPIOA,GPIOA,  // A0=PA0,A1=PA1,A2=PA3,A3=PA4
  GPIOA,GPIOA,GPIOA,GPIOA,  // A4=PA5,A5=PA6,A6=PA7,A7=PA2
  GPIOA,GPIOA,GPIOA,GPIOB,  // D0=PA10,D1=PA9,D2=PA12,D3=PB0
  GPIOB,GPIOB,GPIOB,GPIOC,  // D4=PB7,D5=PB6,D6=PB1,D7=PC14
  GPIOC,GPIOA,GPIOA,GPIOB,  // D8=PC15,D9=PA8,D10=PA11,D11=PB5
  GPIOB,GPIOB               // D12=PB4,D13=PB3.
};

// Mapping of Nucleo pin number to GPIO pin
// Using this plust g_GPIO_port[] above, we can translate a Nucleo pin name into
// the chip's actual GPIO port and pin number.
static uint8_t g_GPIO_pin[D13+1] = {
  0,1,3,4,    // A0=PA0,A1=PA1,A2=PA3,A3=PA4
  5,6,7,2,    // A4=PA5,A5=PA6,A6=PA7,A7=PA2
  10,9,12,0,  // D0=PA10,D1=PA9,D2=PA12,D3=PB0
  7,6,1,14,   // D4=PB7,D5=PB6,D6=PB1,D7=PC14
  15,8,11,5,  // D8=PC15,D9=PA8,D10=PA11,D11=PB5
  4,3         // D12=PB4,D13=PB3.
};

/*
* name:     gpio_enable_port
* purpose:  Enables a GPIO port (A, B, C, or H) by setting the appropriate bit in the RCC
*           clock enable register.  
* input:    pointer to GPIO port to enable, one of GPIOA, GPIOB, GPIOC, GPIOH
*/
static void gpio_enable_port (GPIO_TypeDef *gpio) {
    unsigned long field;
    if (gpio==GPIOA)      field=RCC_AHB2ENR_GPIOAEN;
    else if (gpio==GPIOB) field=RCC_AHB2ENR_GPIOBEN;
    else if (gpio==GPIOC) field=RCC_AHB2ENR_GPIOCEN;
    else           field=RCC_AHB2ENR_GPIOHEN;
    RCC->AHB2ENR |= field; // turn on the GPIO clock
}

/*
* name:     gpio_config_mode
* purpose:  configure direction for a GPIO pin (input, output, alt, analog)
* input:    pin ID, direction mode (INPUT (0b00) OUTPUT (0b01), etc)
* returns:  EE14Lib_ERR_INVALID_CONFIG for invalid direction value, otherwise
*           EE14Lib_Err_OK.
*/
EE14Lib_Err gpio_config_mode(EE14Lib_Pin pin, unsigned int mode)
{
    GPIO_TypeDef* port = g_GPIO_port[pin];
    uint8_t pin_offset = g_GPIO_pin[pin];

    if(mode & ~0b11UL){ // only bottom two bits are valid
        return EE14Lib_ERR_INVALID_CONFIG;
    }

    // enable the GPIO port in case it hasn't been already
    gpio_enable_port(port);

    port->MODER &= ~(0b11 << pin_offset*2); // clear both mode bits
    port->MODER |=  (mode << pin_offset*2); // set specified mode

    return EE14Lib_Err_OK;
}

/*
* name:     gpio_config_pullup
* purpose:  configure pull-up mode for an input configured GPIO pin
* input:    pin ID, pull mode (PULL_OFF (0b00), PULL_UP (0b01), PULL_DOWN (0b10))
* return:   EE14Lib_ERR_INVALID_CONFIG for invalid pullup mode value, otherwise
*           EE14Lib_Err_OK.
* note:     register will write even if GPIO is configured as output - generally this
*           function is only meaningful when GPIO pin is used as input. 0b11 mode produces an error
*/
EE14Lib_Err gpio_config_pullup(EE14Lib_Pin pin, unsigned int mode)
{
    GPIO_TypeDef* port = g_GPIO_port[pin];
    uint8_t pin_offset = g_GPIO_pin[pin];

    if(mode & ~0b11UL){ // only bottom two bits are are valid
        return EE14Lib_ERR_INVALID_CONFIG;
    }

    port->PUPDR &= ~(0b11 << pin_offset*2); // clear both mode bits
    port->PUPDR |=  (mode << pin_offset*2); // configure pull mode

    return EE14Lib_Err_OK;
}

/*
* name:     gpio_config_otype 
* purpose:  configure output type of GPIO Pin (push/pull or open-drain) 
* input:    pin ID, output mode (PUSH_PULL (0b0) or OPEN_DRAIN (0b01))
* return:   EE14Lib_ERR_INVALID_CONFIG for invalid otype value, otherwise
*           EE14Lib_Err_OK.
*/
EE14Lib_Err gpio_config_otype(EE14Lib_Pin pin, unsigned int otype)
{
    GPIO_TypeDef* port = g_GPIO_port[pin];
    uint8_t pin_offset = g_GPIO_pin[pin];

    if(otype & ~0b1UL){ // only bottom bit is valid
        return EE14Lib_ERR_INVALID_CONFIG;
    }

    port->OTYPER &= ~(0b1 << pin_offset); // clear mode bit
    port->OTYPER |=  (otype << pin_offset);

    return EE14Lib_Err_OK;
}

/*
* name:     gpio_config_ospead
* purpose:  configure output speed of GPIO Pin 
* input:    pin ID, output speed (LOW_SPD 0b00 MED_SPD 0b01 HI_SPD  0b10 V_HI_SPD 0b11)
* return:   EE14Lib_ERR_INVALID_CONFIG for invalid ospeed value, otherwise
*           EE14Lib_Err_OK.
*/
EE14Lib_Err gpio_config_ospeed(EE14Lib_Pin pin, unsigned int ospeed)
{
    GPIO_TypeDef* port = g_GPIO_port[pin];
    uint8_t pin_offset = g_GPIO_pin[pin];

    if(ospeed & ~0b11UL){ // only bottom two bits are valid
        return EE14Lib_ERR_INVALID_CONFIG;
    }

    port->OSPEEDR &= ~(0b11 << pin_offset*2); // clear both speed bits
    port->OSPEEDR |=  (ospeed << pin_offset*2);

    return EE14Lib_Err_OK;
}

/*
* name:     gpio_config_alternate_functions
* purpose:  configure a GPIO pin for an alternate function. 
* input:    pin ID, integer 0-15 to select function
* output:   EE14Lib_ERR_INVALID_CONFIG for invalid alt function value, otherwise
*           EE14Lib_Err_OK.
* note:     see Tables 15 and 16 of the STM32L432KC datasheet for a complete listing of the alternate
*           functions for each pin. 
*/
EE14Lib_Err gpio_config_alternate_function(EE14Lib_Pin pin, unsigned int function)
{
    if (function > 15) {
        return EE14Lib_ERR_INVALID_CONFIG;
    }

    GPIO_TypeDef* port = g_GPIO_port[pin];
    uint8_t pin_offset = g_GPIO_pin[pin];

    // Enable the GPIO port in case it hasn't been already
    gpio_enable_port(port);

    // Set the GPIO pin mode to "alternate function" (0b10)
    port->MODER &= ~(0b11 << pin_offset*2); // Clear both bits
    port->MODER |=  (0b10 << pin_offset*2); // 0b10 = alternate function mode

    // Set the AFR register
    unsigned int afr_offset = pin_offset * 4; // 4 bits per pin -> value from 0 to 60

    port->AFR[afr_offset >> 5] &= ~(0xF << (0x1F & afr_offset));
    port->AFR[afr_offset >> 5] |=  (function << (0x1F & afr_offset));

    return EE14Lib_Err_OK;
}

/*
* name:     gpio_write
* purpose:  set the value of a single GPIO output pin
* input:    pin ID, bool value to send to the pin
*/
void gpio_write(EE14Lib_Pin pin, bool value)
{
    GPIO_TypeDef* port = g_GPIO_port[pin];
    uint8_t pin_offset = g_GPIO_pin[pin];
    if(value){
      port->BSRR = 1 << pin_offset;
    }
    else{
      port->BRR = 1 << pin_offset; 
    }
}

/*
* name:     gpio_read
* purpose:  read value of a single GPIO pin
* input:    pin ID
*/
bool gpio_read(EE14Lib_Pin pin)
{
    GPIO_TypeDef* port = g_GPIO_port[pin];
    uint8_t pin_offset = g_GPIO_pin[pin];

    return (port->IDR >> pin_offset) & 1UL;
}