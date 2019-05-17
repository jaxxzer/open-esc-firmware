#pragma once

#include <inttypes.h>

enum PwmType
{
    CLASSIC,
    ONESHOT125,
    ONESHOT42,
    MULTISHOT,
    DSHOT,
    PROSHOT
};

enum InputType
{
    PWM,
    UART,
    I2C,
    SPI,
    CAN
};

enum FirmwareVersionType
{
    DEVELOPMENT,
    BETA,
    RELEASE
};

struct FirmwareVersion
{
    FirmwareVersionType type;
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
};

// TODO volatile/nonvolatile
struct GscRegisters
{
    int16_t control_input;
    uint16_t rotor_position;
    int16_t rotor_speed;
    uint16_t bus_current;
    uint16_t bus_voltage;
    uint16_t voltage_phase_a;
    uint16_t voltage_phase_b;
    uint16_t voltage_phase_c;
    uint16_t voltage_neutral;
    uint16_t current_phase_a;
    uint16_t current_phase_b;
    uint16_t current_phase_c;
    uint16_t pwm_input_duty;
    uint16_t pwm_input_period;
    InputType input_type;
    uint8_t audio_volume;
    EscState state;
    uint8_t commutation_step;
    uint16_t commutation_period;
    uint16_t audio_frequency; 

    // non-volatile starts here
    FirmwareVersion firmwareVersion;
    uint32_t system_clock_frequency;
    uint32_t uart_baudrate;
    uint16_t drive_pwm_frequency
    uint8_t drive_pwm_method;
    uint8_t drive_direction;
    uint8_t bus_current_limit;
    uint8_t magnet_poles;
    uint8_t demag_compensation;
    uint8_t field_weakening;
    uint8_t audio_fifo_pointer;
    uint16_t audio_fifo;
}(__attribute__)(packed);