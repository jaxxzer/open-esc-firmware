#pragma once

// 'input signal' is a global name for the throttle signal
// the throttle is 11 bits (2048 steps)

// initialize the input interfaces that were selected at compile time
// (eg uart, can, i2c, pwm, dshot etc..)
bool input_initialize();

// see if an input signal is detected
bool input_detect();

// check if the input signal is valid
bool input_check();

// get the most recent (11 bit) input signal
uint16_t input_get();
