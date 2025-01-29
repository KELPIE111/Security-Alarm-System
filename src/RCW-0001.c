/*-------------------------------------------------------------------------
 * Technika Mikroprocesorowa 2 - Project
 * Project: Security Alarm System
 * Author: Jakub Marsza³ek
 * File: RCW-0001.c
 * 
 * This file implements the RCW-0001 ultrasonic sensor interface:
 * - Trigger pin initialization
 * - Measurement control
 *-------------------------------------------------------------------------*/

#include "RCW-0001.h"
#include "TPM.h"

/*-------------------------------------------------------------------------
 * Constants
 *-------------------------------------------------------------------------*/
#define TRIGGER_PIN          11          // PTB11 pin number
#define TRIGGER_PULSE_US     10          // Trigger pulse duration in microseconds

/*-------------------------------------------------------------------------
 * Function: Init_Trigger_Pin
 * Purpose: Initialize the trigger pin for the ultrasonic sensor
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void Init_Trigger_Pin(void) {
    // Enable clock for Port B
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    
    // Configure pin as GPIO
    PORTB->PCR[TRIGGER_PIN] = PORT_PCR_MUX(1);
    
    // Set pin as output and initialize to low state
    PTB->PDDR |= (1 << TRIGGER_PIN);
    PTB->PCOR = (1 << TRIGGER_PIN);
}

/*-------------------------------------------------------------------------
 * Function: Start_Measurement
 * Purpose: Generate trigger pulse to start distance measurement
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void Start_Measurement(void) {
    // Generate trigger pulse
    PTB->PSOR = (1 << TRIGGER_PIN);     // Set trigger pin high
    TPM0_us(TRIGGER_PULSE_US);     			// Wait for specified duration
    PTB->PCOR = (1 << TRIGGER_PIN);     // Set trigger pin low
}