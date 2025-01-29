/*-------------------------------------------------------------------------
 * Technika Mikroprocesorowa 2 - Project
 * Project: Security Alarm System
 * Author: Jakub Marsza³ek
 * File: main.c
 * 
 * This file implements a security system with:
 * - Password protection with admin mode
 * - Accelerometer-based motion detection
 * - Distance sensor monitoring
 * - Alarm triggering and control
 *-------------------------------------------------------------------------*/

#include "MKL05Z4.h"
#include "TPM.h"
#include "leds.h"
#include "i2c.h"
#include "accelerometer.h"
#include "RCW-0001.h"
#include "DAC.h"
#include "keyboard.h"
#include "alarm.h"
#include <math.h>
#include <string.h>
#include "frdm_bsp.h"

/*-------------------------------------------------------------------------
 * Constants
 *-------------------------------------------------------------------------*/
#define INT2_PIN_MASK    (1 << 10)
#define ZYXDR_MASK       (1 << 3)
#define MAX_PASSWORD     4
#define MOTION_THRESHOLD 1.3f
#define DISTANCE_THRESHOLD 10.0f
#define BUTTON_DEBOUNCE_DELAY 70000
#define KEYBOARD_DEBOUNCE_DELAY 50000

/*-------------------------------------------------------------------------
 * Password Management Variables
 *-------------------------------------------------------------------------*/
volatile char button = 0;
char password[MAX_PASSWORD] = {'1', '2', '3', '4'};
char admin_password[MAX_PASSWORD] = {'4', '3', '2', '1'};
volatile uint8_t pass_counter = 0;
char input_password[MAX_PASSWORD];
volatile char button_buff = 0;
volatile uint8_t button_handled = 0;
volatile uint8_t type_count = 0;
volatile uint8_t admin_mode = 0;
uint8_t admin_new_password[MAX_PASSWORD];
volatile uint8_t admin_counter = 0;
volatile int button_pressed = 0;

/*-------------------------------------------------------------------------
 * Distance Sensor Variables
 *-------------------------------------------------------------------------*/
volatile uint32_t d = 0;
volatile float result = 0;
volatile float tick = 0, tick_head = 0;
volatile float distance = 0;
volatile uint8_t measure_ready = 0;
volatile uint32_t ps_value[] = {1, 2, 4, 8, 16, 32, 64, 128};

/*-------------------------------------------------------------------------
 * Accelerometer Variables
 *-------------------------------------------------------------------------*/
static uint8_t arrayXYZ[6];
double X, Y, Z;
volatile int motion_detected = 0;
static uint8_t status;

/*-------------------------------------------------------------------------
 * Alarm Control Variables
 *-------------------------------------------------------------------------*/
volatile uint8_t alarm = 0;
volatile uint8_t alarm_armed = 1;  // 1 = armed, 0 = disarmed

/*-------------------------------------------------------------------------
 * Keypad Matrix Configuration
 *-------------------------------------------------------------------------*/
char ButtonMatrix[3][4] = {
    {'1', '2', '3', 'C'},
    {'4', '5', '6', '#'},
    {'7', '8', '9', '0'}
};

/*-------------------------------------------------------------------------
 * Password Management Functions
 *-------------------------------------------------------------------------*/
uint8_t check_password(const char *expected_password) {
    for (uint8_t i = 0; i < MAX_PASSWORD; i++) {
        if (input_password[i] != expected_password[i]) {
            return 0;  // Password incorrect
        }
    }
    return 1;  // Password correct
}

void handle_password_input(char button) {
    if (admin_mode) {
        // Administrator mode - password modification
        if (admin_counter < MAX_PASSWORD) {
            admin_new_password[admin_counter++] = button;
        }

        if (admin_counter == MAX_PASSWORD) {
            // Save new password
            memcpy(password, admin_new_password, MAX_PASSWORD);
            admin_mode = 0;
            PTB->PDOR |= (1 << 9); 
            admin_counter = 0;
        }
    } else {
        // Standard mode - password verification
        if (pass_counter < MAX_PASSWORD) {
            input_password[pass_counter++] = button;
        }

        if (pass_counter == MAX_PASSWORD) {
            if (check_password(password)) {
                alarm_armed = !alarm_armed;
                alarm = 0;
            } else if (check_password(admin_password)) {
                // Enter administrator mode
                PTB->PDOR &= ~(1 << 9);
                admin_mode = 1;
                alarm_armed = 0;
                alarm = 0;
                admin_counter = 0;
            }

            // Reset after password check
            pass_counter = 0;
            memset(input_password, 0, MAX_PASSWORD);
        }
    }
}

/*-------------------------------------------------------------------------
 * Interrupt Handlers
 *-------------------------------------------------------------------------*/
void Row_Int(int row_number) {
    uint8_t column_number = Col_Det();
    TPM0_us(KEYBOARD_DEBOUNCE_DELAY);
    button = ButtonMatrix[row_number][column_number];
    button_pressed = 1;
}

void PORTA_IRQHandler(void) {
    uint32_t interrupt_flags = PORTA->ISFR;

    // Handle accelerometer interrupt
    if (interrupt_flags & INT2_PIN_MASK) {
        motion_detected = 1;
        PORTA->ISFR |= INT2_PIN_MASK;
    }

    // Handle keypad row interrupts
    if (PORTA->ISFR & (1 << ROW2)) {
        TPM0_us(BUTTON_DEBOUNCE_DELAY);
        if (!(PTA->PDIR & (1 << ROW2))) {
            Row_Int(2);
        }
        PORTA->ISFR |= (1 << ROW2);
    }
    if (PORTA->ISFR & (1 << ROW3)) {
        TPM0_us(BUTTON_DEBOUNCE_DELAY);
        if (!(PTA->PDIR & (1 << ROW3))) {
            Row_Int(1);
        }
        PORTA->ISFR |= (1 << ROW3);
    }
    if (PORTA->ISFR & (1 << ROW4)) {
        TPM0_us(BUTTON_DEBOUNCE_DELAY);
        if (!(PTA->PDIR & (1 << ROW4))) {
            Row_Int(0);
        }
        PORTA->ISFR |= (1 << ROW4);
    }

    // Clear all interrupt flags
		PORTA->ISFR |= (INT2_PIN_MASK | (1 << ROW2) | (1 << ROW3) | (1 << ROW4));
}

void TPM1_IRQHandler(void) {
    TPM1->SC = 0;  // Stop TPM1

    if (TPM1->STATUS & TPM_STATUS_TOF_MASK) {
        TPM1->SC = 0;
        result = 100000;
        d = (d + 1) % 8;
    }

    if (TPM1->STATUS & TPM_STATUS_CH1F_MASK) {
        result = TPM1->CONTROLS[1].CnV;
        measure_ready = 1;
    }

    // Clear interrupt flags
    TPM1->STATUS |= (TPM_STATUS_CH1F_MASK | TPM_STATUS_TOF_MASK);

    // Restore TPM1 configuration
    TPM1->SC = d | TPM_SC_TOIE_MASK | TPM_SC_CMOD(1);
}

/*-------------------------------------------------------------------------
 * Main Function
 *-------------------------------------------------------------------------*/
int main(void) {
    // Initialize peripherals
    LED_Init();
    I2C_Init();
    InitInterrupt();
    InitAccelerometer();
    Keyboard_Init();
    sin_init();
    DAC_Init();
    Init_Trigger_Pin();
    InCap_OutComp_Init();
    Init_TPM0();
	
		tick_head = 1000.0 / SystemCoreClock;  // Clock cycle duration in seconds

    while (1) {
        // Handle button input
        if (button_pressed) {
            handle_password_input(button);
            button_pressed = 0;
        }

        if (button == 'C') {
            memset(input_password, 0, MAX_PASSWORD);
            pass_counter = 0;
        }

        // Control alarm state
        if (alarm) {
            alarm_enable();
        } else {
            alarm_disable();
        }
				
				if (alarm_armed == 1 && alarm == 0) {
					PTB->PDOR &= ~(1<<10);
				} else {
					PTB->PDOR |= (1<<10);
				}
				

        // Check accelerometer
        I2C_ReadReg(0x1d, 0x0, &status);
        status &= ZYXDR_MASK;
        if (motion_detected && status) {
            motion_detected = 0;
            I2C_ReadRegBlock(0x1d, 0x1, 6, arrayXYZ);
            
            // Calculate acceleration values
            X = ((double)((int16_t)((arrayXYZ[0] << 8) | arrayXYZ[1]) >> 2) / (4096 >> 0));
            Y = ((double)((int16_t)((arrayXYZ[2] << 8) | arrayXYZ[3]) >> 2) / (4096 >> 0));
            Z = ((double)((int16_t)((arrayXYZ[4] << 8) | arrayXYZ[5]) >> 2) / (4096 >> 0));

            // Check for motion threshold
            if (alarm_armed && (fabs(X) > MOTION_THRESHOLD || 
																fabs(Y) > MOTION_THRESHOLD || 
																fabs(Z) > MOTION_THRESHOLD)) {
                alarm = 1;
            }
        }

        // Distance sensor monitoring
        if (alarm_armed) {
            Start_Measurement();
            if (measure_ready) {
                measure_ready = 0;
                tick = tick_head * ps_value[d];
                result *= tick;
                distance = result / 58 * 1000;

                if (distance > 0 && distance < DISTANCE_THRESHOLD) {
                    alarm = 1;
                }
            }
        }
    }
}