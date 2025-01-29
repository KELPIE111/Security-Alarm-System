/*-------------------------------------------------------------------------
 * Technika Mikroprocesorowa 2 - Project
 * Project: Security Alarm System
 * Author: Jakub Marsza³ek
 * File: alarm.c
 * 
 * This file implements the alarm siren functionality:
 * - Sine wave generation for audio output
 * - Frequency modulation for siren effect
 * - Alarm control functions
 *-------------------------------------------------------------------------*/

#include "alarm.h"
#include <math.h>
#include "DAC.h"
#include "TPM.h"

/*-------------------------------------------------------------------------
 * Constants
 *-------------------------------------------------------------------------*/
#define DIV_CORE            8192        // Interrupt frequency 8192 Hz
#define MASK_10BIT         0x03FF       // 10-bit mask for phase control
#define MIN_MOD            16           // Minimum modulator value (~128 Hz)
#define MAX_MOD            128          // Maximum modulator value (~1024 Hz)
#define STEP_MOD           1            // Modulator increment step
#define SINE_TABLE_SIZE    1024         // Size of sine wave lookup table
#define DAC_OFFSET         0x0800       // DAC middle level offset
#define SIN_ANGLE_RAD    0.0061359231515  // 2*PI/1024 for angle calculation
#define SIN_AMPLITUDE     2047.0        // Amplitude of sine wave
#define ALARM_LED_RED      8            // Red LED pin
#define ALARM_LED_BLUE     10           // Blue LED pin
#define US           1000         // How many us

/*-------------------------------------------------------------------------
 * Global Variables
 *-------------------------------------------------------------------------*/
volatile uint16_t Sinus[SINE_TABLE_SIZE];  // Sine wave lookup table
volatile uint16_t faza = 0;                // Phase accumulator
volatile uint16_t mod = MIN_MOD;           // Phase modulator
volatile int8_t direction = 1;             // Modulation direction
volatile int16_t time = 0;                 // Time counter
volatile int8_t time_ok = 0;               // Time flag

/*-------------------------------------------------------------------------
 * Function: SysTick_Handler
 * Author dr in¿. Mariusz Soko³owski
 * Purpose: Handle SysTick interrupt for DAC output
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
    // Calculate DAC value with offset
    uint16_t dac_value = Sinus[faza] + DAC_OFFSET;
    
    // Load value to DAC
    DAC_Load_Trig(dac_value);
    
    // Update and wrap phase
    faza += mod;
    faza &= MASK_10BIT;
}

/*-------------------------------------------------------------------------
 * Function: alarm_enable
 * Purpose: Enable alarm siren with frequency modulation
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void alarm_enable(void)
{
    // Set LED indicators
		PTB->PDOR |= (1 << ALARM_LED_BLUE);    // Turn off blue LED
    PTB->PDOR &= ~(1 << ALARM_LED_RED);    // Turn on red LED
    
    // Start SysTick for DAC updates
    SysTick_Config(SystemCoreClock / DIV_CORE);
    
    // Update frequency modulation
    mod += direction * STEP_MOD;
    if (mod >= MAX_MOD || mod <= MIN_MOD) {
        direction = -direction;             // Reverse modulation direction
    }
    
    TPM0_us(US);
}

/*-------------------------------------------------------------------------
 * Function: alarm_disable
 * Purpose: Disable alarm siren
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void alarm_disable(void)
{
    PTB->PDOR |= (1 << ALARM_LED_RED);     // Turn off red LED
    SysTick_Config(1);                      // Stop SysTick
}

/*-------------------------------------------------------------------------
 * Function: sin_init
 * Purpose: Initialize sine wave lookup table
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void sin_init(void)
{
    for (uint16_t i = 0; i < SINE_TABLE_SIZE; i++) {
        Sinus[i] = (sin((double)i * SIN_ANGLE_RAD) * SIN_AMPLITUDE);
    }
}