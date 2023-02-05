/*============================================================================
 *  api/gpio.h
 *
 * Copyright (c)2022 Kevin Boone, GPL v3.0
 * ==========================================================================*/
#pragma once

// Default GPIO device
#define BEAROS_GPIO_DEVICE "p:/gpio"

// GPIO commands -- send to the GPIO device to read, write, or set config

// Initialize (reset) pin
#define GPIOCMD_INIT "RE" 
// Read value
#define GPIOCMD_READ "RD"
// Set high (in write mode) 
#define GPIOCMD_SET_HIGH "HI" 
// Set low (in write mode) 
#define GPIOCMD_SET_LOW "LO" 
// Pull up
#define GPIOCMD_PULL_UP "PU" 
// Pull down
#define GPIOCMD_PULL_DOWN "PD" 
// Set as input
#define GPIOCMD_SET_INPUT "IN" 
// Set as input
#define GPIOCMD_SET_OUTPUT "OU" 


