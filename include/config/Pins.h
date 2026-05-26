#pragma once

#include "driver/gpio.h"
#include "driver/adc.h"

// Central GPIO and ADC channel assignments for the Aerisys controller.
// Update this file when the PCB layout changes — no other source file
// should hard-code a pin number.
namespace Aerisys::Controller::Pins
{
    // ---------------------------------------------------------------------
    // Joysticks (ADC1, 12-bit, 11dB attenuation)
    // ---------------------------------------------------------------------
    static constexpr adc1_channel_t JOY_LEFT_X  = ADC1_CHANNEL_6;  // GPIO 35
    static constexpr adc1_channel_t JOY_LEFT_Y  = ADC1_CHANNEL_7;  // GPIO 34
    static constexpr adc1_channel_t JOY_RIGHT_X = ADC1_CHANNEL_4;  // GPIO 32
    static constexpr adc1_channel_t JOY_RIGHT_Y = ADC1_CHANNEL_5;  // GPIO 33

    // ---------------------------------------------------------------------
    // Buttons (active-low with internal pull-up)
    // ---------------------------------------------------------------------
    static constexpr gpio_num_t BTN_ARMING      = GPIO_NUM_14;
    static constexpr gpio_num_t BTN_MOTOR_STATE = GPIO_NUM_13;
    static constexpr gpio_num_t BTN_ASSOCIATION = GPIO_NUM_16;

    // ---------------------------------------------------------------------
    // Status outputs
    // ---------------------------------------------------------------------
    static constexpr gpio_num_t LED_STATUS = GPIO_NUM_2;
}
