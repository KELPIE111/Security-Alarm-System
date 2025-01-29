/*-------------------------------------------------------------------------
 * Technika Mikroprocesorowa 2 - Project
 * Project: Security Alarm System
 * Author: Jakub Marszałek
 * File: TPM.c
 * 
 * This file implements Timer/PWM Module functionality:
 * - Input Capture and Output Compare initialization
 * - TPM0 initialization for microsecond delays
 * - Microsecond delay function
 *-------------------------------------------------------------------------*/

#include "TPM.h"

/*-------------------------------------------------------------------------
 * Constants
 *-------------------------------------------------------------------------*/
#define SYSTEM_CLOCK_HZ     41943040    // MCGFLLCLK frequency
#define TPM0_CLOCK_HZ       48000000    // TPM0 clock frequency
#define TICKS_PER_US        48          // Clock ticks per microsecond for TPM0
#define MAX_TIMER_COUNT     0xFFFF      // Maximum 16-bit timer value

/*-------------------------------------------------------------------------
 * Function: InCap_OutComp_Init
 * Author: dr inż. Mariusz Sokołowski
 * Purpose: Initialize Input Capture and Output Compare functionality
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void InCap_OutComp_Init(void)
{
    // Enable clock for Port B
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    
    // Configure pin multiplexing
    PORTB->PCR[0] |= PORT_PCR_MUX(2);      // PTB0 - EXTRG_IN
    PORTB->PCR[13] |= PORT_PCR_MUX(2);     // PTB13 - TPM1_CH1 (Input Capture)
    PORTB->PCR[11] |= PORT_PCR_MUX(1);     // PTB11 - GPIO
    PORTB->PCR[11] &= (~PORT_PCR_SRE_MASK);// Disable Slew Rate
    
    // Configure GPIO output
    PTB->PDDR |= (1 << 11);                // Set PTB11 as output
    PTB->PCOR = (1 << 11);                 // Clear PTB11
    
    // Enable TPM1 clock and configure clock source
    SIM->SCGC6 |= SIM_SCGC6_TPM1_MASK;
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1);     // Select MCGFLLCLK as clock source
    
    // Configure TPM1
    TPM1->SC &= ~(TPM_SC_CPWMS_MASK);      // Up counting mode
    TPM1->SC &= ~(TPM_SC_PS_MASK);         // Prescaler = 1
    
    // Configure TPM1 trigger settings
    TPM1->CONF |= (TPM_CONF_CSOT_MASK |    	// Counter Start on Trigger
                   TPM_CONF_CROT_MASK |     // Counter Reload on Trigger
                   TPM_CONF_CSOO_MASK);     // Counter Stop on Overflow
    TPM1->CONF &= ~(TPM_CONF_TRGSEL_MASK); // External trigger EXTRG_IN
    
    // Configure Channel 1 for Input Capture
    TPM1->CONTROLS[1].CnSC = (TPM_CnSC_ELSB_MASK | // Falling edge
                              TPM_CnSC_CHIE_MASK);  // Enable channel interrupt
    
    // Configure TPM1 interrupts
    TPM1->SC |= TPM_SC_TOIE_MASK;          // Enable overflow interrupt
    NVIC_ClearPendingIRQ(TPM1_IRQn);
    NVIC_EnableIRQ(TPM1_IRQn);
    
    // Start TPM1
    TPM1->SC |= TPM_SC_CMOD(1);
}

/*-------------------------------------------------------------------------
 * Function: Init_TPM0
 * Purpose: Initialize TPM0 for microsecond delay functionality
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void Init_TPM0(void) {
    // Enable TPM0 clock and configure clock source
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1);     // Select MCGFLLCLK
    
    // Configure TPM0
    TPM0->SC = 0;                          // Disable timer during configuration
    TPM0->MOD = TICKS_PER_US - 1;          // Set modulo for 1µs period
    TPM0->SC = TPM_SC_PS(0);               // Set prescaler to 1
    TPM0->SC |= TPM_SC_CMOD(1);            // Start timer
}

/*-------------------------------------------------------------------------
 * Function: TPM0_Delay_us
 * Purpose: Generate precise microsecond delay
 * Parameters: 
 * us - Number of microseconds to delay
 * Returns: None
 *-------------------------------------------------------------------------*/
void TPM0_us(uint32_t us) {
    uint32_t target_ticks = us * TICKS_PER_US;
    
    while (target_ticks > 0) {
        // Handle delays longer than maximum timer value
        uint32_t current_delay = (target_ticks > MAX_TIMER_COUNT) ? 
                                MAX_TIMER_COUNT : target_ticks;
        
        TPM0->CNT = 0;                     // Reset counter
        TPM0->MOD = current_delay;         // Set target value
        
        // Wait for overflow
        while (!(TPM0->SC & TPM_SC_TOF_MASK));
        TPM0->SC |= TPM_SC_TOF_MASK;       // Clear overflow flag
        
        target_ticks -= current_delay;
    }
}