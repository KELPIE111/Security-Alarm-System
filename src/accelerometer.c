/*-------------------------------------------------------------------------
 * Technika Mikroprocesorowa 2 - Project
 * Project: Security Alarm System
 * Author: Jakub Marsza³ek
 * File: accelerometer.c
 * 
 * This file implements the accelerometer interface:
 * - Interrupt configuration
 * - Accelerometer initialization and configuration
 * - MMA8451Q sensor setup
 *-------------------------------------------------------------------------*/

#include "accelerometer.h"
#include "i2c.h"

/*-------------------------------------------------------------------------
 * Constants
 *-------------------------------------------------------------------------*/
#define INT2_PIN         10          // Interrupt pin number
#define MMA8451Q_ADDR   0x1D        // Device I2C address
#define CTRL_REG1       0x2A        // Control register 1
#define CTRL_REG4       0x2D        // Control register 4
#define CTRL_REG5       0x2E        // Control register 5
#define XYZ_DATA_CFG    0x0E        // Sensitivity configuration register
#define STATUS_REG      0x00        // Status register

/*-------------------------------------------------------------------------
 * Static Variables
 *-------------------------------------------------------------------------*/
static uint8_t sens = 0;            // Sensitivity setting

/*-------------------------------------------------------------------------
 * Function: InitInterrupt
 * Purpose: Initialize interrupt for accelerometer
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void InitInterrupt(void) {
    // Enable clock for Port A
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    
    // Configure interrupt pin
    PORTA->PCR[INT2_PIN] = PORT_PCR_MUX(1) |        // GPIO mode
                           PORT_PCR_IRQC(0xa);       // Rising edge interrupt
    
    // Set pin as input
    PTA->PDDR &= ~(1 << INT2_PIN);
    
    // Configure NVIC for Port A interrupt
    NVIC_SetPriority(PORTA_IRQn, 1);                // Set interrupt priority
    NVIC_ClearPendingIRQ(PORTA_IRQn);              // Clear any pending interrupts
    NVIC_EnableIRQ(PORTA_IRQn);                     // Enable Port A interrupts
}

/*-------------------------------------------------------------------------
 * Function: InitAccelerometer
 * Author: dr in¿. Mariusz Soko³owski 
 * Purpose: Initialize and configure the MMA8451Q accelerometer
 * Parameters: None
 * Returns: None
 *-------------------------------------------------------------------------*/
void InitAccelerometer(void) {
    sens = 0;  // Initialize sensitivity setting
    
    // Configuration sequence for MMA8451Q
    I2C_WriteReg(MMA8451Q_ADDR, CTRL_REG1, 0x00);    // Set to standby mode
    I2C_WriteReg(MMA8451Q_ADDR, XYZ_DATA_CFG, sens); // Set sensitivity
    I2C_WriteReg(MMA8451Q_ADDR, CTRL_REG4, 0x01);    // Enable ZYXDR data ready interrupt
    I2C_WriteReg(MMA8451Q_ADDR, CTRL_REG5, 0x02);    // Route ZYXDR interrupt to INT2 pin
    I2C_WriteReg(MMA8451Q_ADDR, CTRL_REG1, 0x01);    // Activate the device
    I2C_WriteReg(MMA8451Q_ADDR, STATUS_REG, 0x00);   // Clear ZYXDR flag
}