#ifndef WEATHER_H
#define WEATHER_H

#include "driver/i2c_master.h"

#include "esp_err.h"

esp_err_t weather_init_sensors(i2c_master_bus_handle_t i2c_bus_handle,
                               uint32_t sensor1_led_status_gpio,
                               uint32_t sensor2_led_status_gpio);

float weather_get_temperature(void);
float weather_get_pressure(void);
float weather_get_humidity(void);

#endif