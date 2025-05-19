/*
* dac.cpp
*
* DAC and DMA configuration for STM32L432KC to drive audio utput via PA4. Sin
* 
* Wilson Wu, Tufts University, Spring 2025
* Edited: Alisa Yurevich - comments + small code edits
*/

#include "ee14lib.h"
#include "dac.h"

uint16_t sin_table[NUM_SAMPLES];

/*
* name:     dac_init
* purpose:  intializes dac channel 1 for buffered analog output, config
*           tim6 as trigger source
*/
void dac_init (void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_DAC1EN; // enable dac clock
    gpio_init(); // enable PA4

    DAC1->CR &= ~DAC_CR_EN1;            // disable prior to config
    DAC1->CR |= DAC_CR_TEN1;            // DAC Channel 1 trigger enabled
    DAC1->CR &= ~DAC_CR_TSEL1_Msk;      // DAC Trigger Selection, TIM6_TRGO
    DAC1->CR &= ~DAC_CR_WAVE1;          // disable waveform generation

    // buffer to amplify the signal
    // enable normal mode for the DAC (0xx)
    DAC1->MCR &= ~DAC_MCR_MODE1;
    DAC1->CR |= DAC_CR_EN1;         // enable channel 1
}
 
/*
* name:     gpio_init
* purpose:  helper function, configure PA4/A3 as an analog ouput
*/
void gpio_init (void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;    // periphal clock
    gpio_config_mode(A3, 0b11);             // analog mode
}

/*
* name:     clock_init
* purpose:  intializes TIM6 to act as DAC trigger source
*/
void clock_init (int freq) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM6EN;   // peripheral clock

    freq = freq * 100;                      // sample 100 times per period
    // TIM6 = CLK / (PSC + 1) * (ARR + 1) 
    uint16_t psc = (4000000 / (freq * 65535));
    TIM6->PSC = psc;
    TIM6->ARR = (4000000 / (freq * (psc + 1))) - 1;

    TIM6->DIER |= TIM_DIER_UDE;             // enable DMA request signal on overflow (update events)
    TIM6->CR2 &= ~(TIM_CR2_MMS_Msk);        
    TIM6->CR2 |= TIM_CR2_MMS_1;             // set (trigger output) TRGO on an update event
    TIM6->CR1 |= TIM_CR1_CEN;               // enable clock
}

/*
* name:     dma_init
* purpose:  intializes DMA1 channel 3 for DAC1 use. enables 16 bit transfer from
*           memory to peripheral (right holding register of DAC). enables 100 samples
*           of a sine table + circular mode for continous noise production.
* note:     DMA is not enabled, must be done outside of function for use. 
*/
void dma_init (void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;                 // periph clock
    DMA1_Channel3->CCR &= ~(DMA_CCR_EN_Msk);            // disable prior to config

    // map DMA1 channel 3 to DAC channel 1
    DMA1_CSELR->CSELR &= ~(DMA_CSELR_C3S_Msk);
    DMA1_CSELR->CSELR |= (0b0110 << DMA_CSELR_C3S_Pos);

    // set memory and peripheral size to 16 bits
    DMA1_Channel3->CCR &= ~(DMA_CCR_MSIZE_Msk);
    DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0;              // MSIZE = 01 = 16 bits
    DMA1_Channel3->CCR &= ~(DMA_CCR_PSIZE_Msk);
    DMA1_Channel3->CCR |= DMA_CCR_PSIZE_0;              // PSIZE = 01 = 16 bits

    DMA1_Channel3->CCR |= DMA_CCR_DIR;                  // set direction: mem -> peripheral
    DMA1_Channel3->CCR |= DMA_CCR_MINC;                 // enable mem increment
    DMA1_Channel3->CCR |= DMA_CCR_CIRC;                 // enable circular mode

    DMA1_Channel3->CNDTR = 100;                         // sample num to send
    DMA1_Channel3->CMAR = (uint32_t)sin_table;          // memory source address
    DMA1_Channel3->CPAR = (uint32_t)&DAC1->DHR12R1;     // peripheral destination address
}

/*
* name:     create_sin_table
* purpose:  populate sine table with 100 precomputed samples of a sine wave
* note:     sine wave freq: timer update freq / NUM_SAMPLES
*/
void create_sin_table (void) {
    for(int i = 0; i < NUM_SAMPLES; i++) { 
        uint16_t temp = MAX_DAC / 2 * (sin (3.14159 / 50 * i) + 1);
        sin_table[i] = temp;
    }
}

/*
* name:     trigger_horn
* purpose:  enable DMA to create honk (400Hz at current timer update) for 200ms
*/
void trigger_horn (void) {
    DMA1_Channel3->CCR |= DMA_CCR_EN;
    delay(200);
    DMA1_Channel3->CCR &= ~(DMA_CCR_EN);
}

/*
* name:     clock_set
* purpose:  adjusts TIM6 frequency dynamically while running
* note:     currently unused, tone change in real-time
*/
void clock_set (int freq) {
    TIM6->CR1 &= ~(TIM_CR1_CEN_Msk);    // disable clock
    freq = freq * 100;

    // update TIM6 values
    // TIM6 = CLK / (PSC + 1) * (ARR + 1) 
    uint16_t psc = (40000000 / (freq * 65535)) - 1;
    TIM6->PSC = psc;
    TIM6->ARR = (40000000 / (freq * (psc + 1))) - 1;
    TIM6->CR1 |= TIM_CR1_CEN;            // re-nable TIM6 Clock
}