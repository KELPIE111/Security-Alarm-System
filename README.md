# Security Alarm System

## System Overview
The Security Alarm System is an advanced alarm system based on the FRDM-KL05Z platform, integrating various components to provide comprehensive object protection functionality.

## Key Components
* MMA8451Q Accelerometer with INT2 interrupt
* WaveShare 3972 Alarm Siren based on DAC DDS
* RCW-0001 Distance Sensor
* HW-834 4x4 Keyboard with interrupts on three rows

## Operating Modes
* Normal Mode: Arming and disarming the alarm
* Administrator Mode: Changing the arming and disarming code

## Status Indication (LED)
* Red LED: Alarm activation
* Green LED: Administrator mode
* Blue LED: System armed

## System Architecture

### 1. Accelerometer (INT2, I2C)
* Detects motion and generates INT2 interrupt when threshold values are exceeded
* Communication via I2C bus
* Real-time monitoring of accelerometer values
* Triggers alarm siren when anomalies are detected

### 2. Alarm Siren (DAC DDS)
* Generates audio signal using Digital-to-Analog Converter (DAC)
* Implements Direct Digital Synthesis (DDS) technique
* Uses SysTick for precise sinusoidal waveform generation
* Activates upon trigger from accelerometer or distance sensor

### 3. RCW-0001 Distance Sensor
* Utilizes TPM1 counter in Input Capture and Output Compare mode
* Operation sequence:
  * Sends 10μs pulse on PTB11 pin (TRIG)
  * Receives reflected pulse on PTB0 pin (ECHO), starting TPM1 counter
  * Pulse duration captured in CnV register and converted to distance
  * PTB0 and PTB13 pins are connected for falling edge detection

### 4. HW-834 4x4 Keyboard
* Operates with interrupts from three rows (12 buttons total)
* Special functions:
  * "C" button clears previously entered values
* Functionality:
  * Key press generates interrupt for code processing
  * Used for system arming/disarming and administrator code input

### 5. Interrupt Handling
* Manages interrupts for:
  * Accelerometer
  * Keyboard
  * Distance sensor
  * SysTick for DAC operations

## System Features

### Alarm Arming and Disarming
* System arms upon correct code entry (indicated by blue LED)
* Disarms when code is re-entered

### Administrator Mode
* Accessed via special code entry
* Allows modification of arming/disarming codes
* Indicated by green LED

### Alarm Activation
* Triggers when:
  * Accelerometer detects motion above threshold
  * Distance sensor detects object within range
* Activation indicated by:
  * Red LED illumination
  * Alarm siren activation
