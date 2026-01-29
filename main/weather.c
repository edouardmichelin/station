#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"

#include "aht20.h"
#include "bmp280.h"

#include "weather.h"

#define I2C_MASTER_FREQ_HZ 100000
#define ADDR AHT_I2C_ADDRESS_GND
#define AHT_TYPE AHT_TYPE_AHT20

#define SENSORS_REFRESH_RATE 10000

static const char *TAG = "weather";

static float aht20_temperature;
static float aht20_humidity;

static float bmp280_temperature;
static float bmp280_pressure;

static uint32_t aht20_status_led_gpio;
static uint32_t bmp280_status_led_gpio;
static bool aht20_failure;
static bool bmp280_failure;

static aht20_dev_handle_t aht20_handle = NULL;
static bmp280_handle_t bmp280_handle = NULL;

static void init_status_led(unsigned int led_gpio)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << led_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);
}

static void bmp280_poll_task(void *arg)
{
    esp_err_t rc;
    float temp, pressure;

    for(;;) {
        rc = bmp280_get_measurements(bmp280_handle, &temp, &pressure);

        if(rc != ESP_OK) {
            ESP_LOGE(TAG, "bmp280 device read failed (%s)", esp_err_to_name(rc));
            gpio_set_level(bmp280_status_led_gpio, 1);
            bmp280_failure = 1;
        } else {
            pressure = pressure / 100;
            ESP_LOGI(TAG, "air temperature:     %.2f °C", temp);
            ESP_LOGI(TAG, "barometric pressure: %.2f hPa", pressure);

            bmp280_temperature = temp;
            bmp280_pressure = pressure;
            gpio_set_level(bmp280_status_led_gpio, 0);
            bmp280_failure = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(SENSORS_REFRESH_RATE));
    }

    bmp280_delete(bmp280_handle);
    vTaskDelete(NULL);
}

static esp_err_t init_bmp280(i2c_master_bus_handle_t i2c_bus_handle, uint32_t led_status_gpio)
{
    ESP_RETURN_ON_FALSE(i2c_bus_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "init_bmp280: i2c_bus_handle is NULL");

    esp_err_t rc;

    bmp280_status_led_gpio = led_status_gpio;
    init_status_led(led_status_gpio);

    bmp280_config_t dev_cfg = I2C_BMP280_CONFIG_DEFAULT;

    rc = bmp280_init(i2c_bus_handle, &dev_cfg, &bmp280_handle);

    if (bmp280_handle == NULL) {
        ESP_LOGE(TAG, "bmp280 handle init failed");
        gpio_set_level(bmp280_status_led_gpio, 1);
        return rc;
    }

    xTaskCreate(bmp280_poll_task, "bmp280_poll_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL);

    return ESP_OK;
}

static void aht20_poll_task(void *arg)
{
    esp_err_t rc;
    float temp, hum;

    for(;;) {
        rc = aht20_read_float(aht20_handle, &temp, &hum);

        if (rc != ESP_OK) {
            ESP_LOGE(TAG, "Reading AHT20 device failed: %s", esp_err_to_name(rc));
            gpio_set_level(aht20_status_led_gpio, 1);
            aht20_failure = 1;
        } else {
            ESP_LOGI(TAG, "Humidity      : %2.2f %%", hum);
            ESP_LOGI(TAG, "Temperature   : %2.2f degC", temp);

            aht20_temperature = temp;
            aht20_humidity = hum;
            gpio_set_level(aht20_status_led_gpio, 0);
            aht20_failure = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(SENSORS_REFRESH_RATE));
    }

    aht20_del_sensor(&aht20_handle);
    vTaskDelete(NULL);
}

static esp_err_t init_aht20(i2c_master_bus_handle_t i2c_bus_handle, uint32_t led_status_gpio)
{
    ESP_RETURN_ON_FALSE(i2c_bus_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "init_aht20: i2c_bus_handle is NULL");

    esp_err_t rc;

    aht20_status_led_gpio = led_status_gpio;
    init_status_led(led_status_gpio);

    i2c_aht20_config_t aht20_i2c_config = {
        .i2c_config.device_address = AHT20_ADDRESS_0,
        .i2c_config.scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .i2c_timeout = 100,
    };

    rc = aht20_new_sensor(i2c_bus_handle, &aht20_i2c_config, &aht20_handle);

    if (aht20_handle == NULL) {
        ESP_LOGE(TAG, "aht20 handle init failed");
        gpio_set_level(aht20_status_led_gpio, 1);
        return rc;
    }

    xTaskCreate(aht20_poll_task, "aht20_poll_task", configMINIMAL_STACK_SIZE * 8, NULL, 1, NULL);

    return ESP_OK;
}

esp_err_t weather_init_sensors(i2c_master_bus_handle_t i2c_bus_handle,
                               uint32_t sensor1_led_status_gpio,
                               uint32_t sensor2_led_status_gpio)
{
    ESP_RETURN_ON_FALSE(i2c_bus_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "weather_init_sensors: i2c_bus_handle is NULL");

    esp_err_t rc1, rc2;

    rc1 = init_aht20(i2c_bus_handle, sensor1_led_status_gpio);
    rc2 = init_bmp280(i2c_bus_handle, sensor2_led_status_gpio);

    if (rc1 != ESP_OK) {
        return rc1;
    }

    return rc2;
}

float weather_get_temperature(void)
{
    if (aht20_failure) {
        return bmp280_failure ? 0 : bmp280_temperature;
    }

    if (bmp280_failure) {
        return aht20_temperature;
    }

    return (aht20_temperature + bmp280_temperature) / 2;
}

float weather_get_pressure(void)
{
    return bmp280_pressure;
}

float weather_get_humidity(void)
{
    return aht20_humidity;
}
