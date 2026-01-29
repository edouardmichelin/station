#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>

#include "esp_err.h"

typedef struct clock_time {
    unsigned int sec;
    unsigned int min;
    unsigned int hour;
} clock_time_t;

void clock_enter_set_time(void);
void clock_exit_set_time(void);
void clock_adjust_time_min(int32_t delta);
void clock_adjust_time_hour(int32_t delta);

void clock_enter_set_alarm(void);
void clock_exit_set_alarm(void);
void clock_adjust_alarm_min(int32_t delta);
void clock_adjust_alarm_hour(int32_t delta);

int clock_is_alarm_on(void);
int clock_is_alarm_ringing(void);
void clock_enable_alarm(void);
void clock_disable_alarm(void);

esp_err_t clock_get_time(clock_time_t *stime, bool *time_is_being_modified);

esp_err_t clock_init(uint32_t status_led_gpio, uint32_t buzzer_gpio);

#endif
