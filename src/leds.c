/*-------------------------------------------------------------------------
 * Technika Mikroprocesorowa 2 - Project
 * Project: Security Alarm System
 * Author: Jakub Marszałek
 * File: leds.c
 * 
 * This file implements the LED control interface:
 * - Initialization of RGB LED pins
 * - Configuration of GPIO for LED control
 *-------------------------------------------------------------------------*/

#include "leds.h"

/*-------------------------------------------------------------------------
 * Function: LED_Init
 * Purpose: Initialize the RGB LED interface
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void LED_Init(void)
{
    // Enable clock for Port B
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    // Configure LED pins as GPIO
    PORTB->PCR[RED] |= PORT_PCR_MUX(1);    // Red LED
    PORTB->PCR[GREEN] |= PORT_PCR_MUX(1);    // Green LED
    PORTB->PCR[BLUE] |= PORT_PCR_MUX(1);    // Blue LED

    // Set LED pins as outputs
    PTB->PDDR |= (RED_MASK | GREEN_MASK | BLUE_MASK);

    // Turn off all LEDs (set pins high for active-low LEDs)
    PTB->PDOR |= (RED_MASK | GREEN_MASK | BLUE_MASK);
}