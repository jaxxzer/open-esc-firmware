#pragma once

void commutation_timer_initialize();
void commutation_timer_enable_interrupts();
void commutation_timer_disable_interrupts();

void zc_timer_initialize();
void zc_timer_enable_interrupts();
void zc_timer_disable_interrupts();

void stop_motor();
void start_motor();
