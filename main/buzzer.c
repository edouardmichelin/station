#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "buzzer.h"

static uint32_t bz_gpio = -1;

esp_err_t buzzer_init(uint32_t buzzer_gpio)
{
    bz_gpio = buzzer_gpio;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << buzzer_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&io_conf);
}

void buzzer_stop(void)
{
    gpio_set_level(bz_gpio, 0);
}

void buzzer_beep(uint32_t frequency, uint32_t time)
{
    if (frequency == 0 || time == 0) {
        gpio_set_level(bz_gpio, 0);
        return;
    }

    uint32_t half_period_us = 1000000 / (frequency * 2);
    uint32_t cycles = (frequency * time) / 1000;

    for (uint32_t i = 0; i < cycles; i++) {
        gpio_set_level(bz_gpio, 1);
        esp_rom_delay_us(half_period_us);

        gpio_set_level(bz_gpio, 0);
        esp_rom_delay_us(half_period_us);
    }
}
