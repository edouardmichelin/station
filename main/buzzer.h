#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

#include "esp_err.h"

esp_err_t buzzer_init(uint32_t buzzer_gpio);

void buzzer_stop(void);

/*
 * Beeps the buzzer at given frequency (in Hz) for the given duration (in ms).
 */
void buzzer_beep(uint32_t frequency, uint32_t time);

#endif