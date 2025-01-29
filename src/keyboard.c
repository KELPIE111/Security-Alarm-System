/*-------------------------------------------------------------------------
 * Technika Mikroprocesorowa 2 - Project
 * Project: Security Alarm System
 * Author: Jakub Marsza³ek
 * File: keyboard.c
 * 
 * This file implements the matrix keyboard interface:
 * - Keyboard initialization
 * - Column detection for key press identification
 * - GPIO configuration for rows and columns
 *-------------------------------------------------------------------------*/

#include "MKL05Z4.h"
#include "keyboard.h"
#include "TPM.h"

/*-------------------------------------------------------------------------
 * Constants
 *-------------------------------------------------------------------------*/
#define NUM_ROWS 3
#define NUM_COLS 4
#define SIGNAL_STABILIZATION_DELAY 25

/*-------------------------------------------------------------------------
 * Static Arrays
 *-------------------------------------------------------------------------*/
static const uint8_t rows[] = {ROW2, ROW3, ROW4};
static const uint8_t cols[] = {COL1, COL2, COL3, COL4};

/*-------------------------------------------------------------------------
 * Function: Keyboard_Init
 * Purpose: Initialize the matrix keyboard interface
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void Keyboard_Init(void) {
    // Enable clock for Port A
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    // Configure row pins
    for (int i = 0; i < NUM_ROWS; i++) {
        PORTA->PCR[rows[i]] = PORT_PCR_MUX(1) |      // GPIO mode
                             PORT_PCR_PE_MASK |       // Enable pull resistor
                             PORT_PCR_PS_MASK |       // Pull-up select
                             PORT_PCR_IRQC(0xa);      // Interrupt on falling edge
    }
    
    // Configure column pins
    for (int i = 0; i < NUM_COLS; i++) {
        PORTA->PCR[cols[i]] = PORT_PCR_MUX(1);       // GPIO mode
        PTA->PDDR |= (1 << cols[i]);                 // Set as output
        PTA->PCOR |= (1 << cols[i]);                 // Set low state
    }
    
    // Configure interrupt handling
    NVIC_SetPriority(PORTA_IRQn, 3);                 // Set interrupt priority
    NVIC_ClearPendingIRQ(PORTA_IRQn);               // Clear any pending interrupts
    NVIC_EnableIRQ(PORTA_IRQn);                     // Enable Port A interrupts
}

/*-------------------------------------------------------------------------
 * Function: Col_Det
 * Purpose: Detect which column was pressed in the matrix keyboard
 * Parameters: None
 * Returns: int - Column number (0-3) or -1 if no column detected
 *-------------------------------------------------------------------------*/
int Col_Det(void) {
    int column = -1;
    uint32_t row_state;
    uint32_t col_mask = (1 << COL1) | (1 << COL2) | (1 << COL3) | (1 << COL4);
    
    // Sequential column scanning
    for (int col = 0; col < NUM_COLS; col++) {
        // Set all columns high
        PTA->PSOR = col_mask;
        
        // Set current column low
        switch(col) {
            case 0: PTA->PCOR = (1 << COL4); break;
            case 1: PTA->PCOR = (1 << COL3); break;
            case 2: PTA->PCOR = (1 << COL2); break;
            case 3: PTA->PCOR = (1 << COL1); break;
        }
        			
				TPM0_us(SIGNAL_STABILIZATION_DELAY);
        
        // Check row states
        row_state = PTA->PDIR;
        if (!(row_state & (1 << ROW2)) || 
            !(row_state & (1 << ROW3)) || 
            !(row_state & (1 << ROW4))) {
            column = col;
            break;
        }
    }
    
    // Restore all columns to low state
    PTA->PCOR = col_mask;
    
    return column;
}