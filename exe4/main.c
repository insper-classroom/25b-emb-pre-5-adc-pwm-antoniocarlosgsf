/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;
const int ADC_PIN   = 28;

const float conversion_factor = 3.3f / (1 << 12);

/**
 * 0..1.0V: Desligado
 * 1..2.0V: 150 ms
 * 2..3.3V: 400 ms
 */
volatile bool adc_tick      = false; // Sinal para ler o ADC
volatile bool led_tick      = false; // Sinal para alternar o LED
volatile bool led_piscando  = false; // Se há led piscando
volatile int  period        = 0;
volatile bool led_on        = false; // Estado atual do LED

repeating_timer_t adc_timer;
repeating_timer_t led_timer;

bool timer_adc_callback(repeating_timer_t *rt) {
    (void)rt;
    adc_tick = true;
    return true;
}

bool timer_led_callback(repeating_timer_t *rt) {
    (void)rt;
    led_tick = true;
    return true;
}

int main() {
    stdio_init_all();

    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);

    adc_init();
    adc_gpio_init(ADC_PIN);

    // ---- mover para escopo local (sem volatile) ----
    bool led_piscando = false;   // Se há LED piscando
    int  period       = 0;       // período atual (ms)
    bool led_on       = false;   // estado atual do LED
    // ------------------------------------------------

    add_repeating_timer_ms(25, timer_adc_callback, NULL, &adc_timer);

    while (1) {
        if (adc_tick) {
            adc_tick = false;

            adc_select_input(2);
            uint16_t result = adc_read();
            float v = result * conversion_factor;

            uint32_t new_period;
            if (v < 1.0f)      new_period = 0;
            else if (v < 2.0f) new_period = 300;   // conforme enunciado
            else               new_period = 500;

            if (new_period != (uint32_t)period) {
                period = (int)new_period;

                if (period == 0) {
                    if (led_piscando) {
                        cancel_repeating_timer(&led_timer);
                        led_piscando = false;
                    }
                    if (led_on) {
                        led_on = false;
                        gpio_put(PIN_LED_B, 0);
                    }
                } else {
                    uint32_t half = (uint32_t)period / 2;
                    if (half == 0) half = 1;

                    if (led_piscando) {
                        cancel_repeating_timer(&led_timer);
                        led_piscando = false;
                    }

                    led_on = false;
                    gpio_put(PIN_LED_B, 0);

                    add_repeating_timer_ms((int)half, timer_led_callback, NULL, &led_timer);
                    led_piscando = true;
                }
            }
        }

        if (led_tick) {
            led_tick = false;

            if (period == 0) {
                if (led_on) {
                    led_on = false;
                    gpio_put(PIN_LED_B, 0);
                }
            } else {
                led_on = !led_on;
                gpio_put(PIN_LED_B, led_on ? 1 : 0);
            }
        }
    }
}
