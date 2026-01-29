#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/lock.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/gpio.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lwip/sys.h"

#include "buzzer.h"
#include "clock.h"

static esp_err_t clock_set_time(void);
static void ring_alarm_task(void *arg);

static const char *TAG = "CLOCK";

static clock_time_t clock_time = { 0 };
static clock_time_t clock_alarm_time = { 0 };

static uint8_t is_set_time_mode = 0;
static uint8_t is_set_alarm_mode = 0;

static int alarm_status = 0;
static int has_alarm_tripped = 0;

static uint32_t status_led_gpio;

int clock_is_alarm_on(void)
{
    return alarm_status;
}

int clock_is_alarm_ringing(void)
{
    return alarm_status && has_alarm_tripped;
}

void clock_enable_alarm(void)
{
    alarm_status = 1;
}

void clock_disable_alarm(void)
{
    alarm_status = 0;
    has_alarm_tripped = 0;
}

void clock_enter_set_time(void)
{
    gpio_set_level(status_led_gpio, 1);
    clock_get_time(&clock_time, NULL);
    clock_time.sec = 0;
    is_set_time_mode = true;
}

void clock_exit_set_time(void)
{
    gpio_set_level(status_led_gpio, 0);
    clock_set_time();
    is_set_time_mode = false;
}

void clock_adjust_time_min(int32_t delta)
{
    if (!is_set_time_mode) return;

    if (((int32_t) clock_time.min + delta) < 0) {
        clock_time.min = 59;
        return;
    }

    clock_time.min = (clock_time.min + delta) % 60;
}

void clock_adjust_time_hour(int32_t delta)
{
    if (!is_set_time_mode) return;

    if (((int32_t) clock_time.hour + delta) < 0) {
        clock_time.hour = 23;
        return;
    }

    clock_time.hour = (clock_time.hour + delta) % 24;
}

void clock_enter_set_alarm(void)
{
    gpio_set_level(status_led_gpio, 1);
    memset(&clock_alarm_time, 0, sizeof(clock_time_t));
    is_set_alarm_mode = true;
}

void clock_exit_set_alarm(void)
{
    gpio_set_level(status_led_gpio, 0);
    is_set_alarm_mode = false;
}

void clock_adjust_alarm_min(int32_t delta)
{
    if (!is_set_alarm_mode) return;

    if (((int32_t) clock_alarm_time.min + delta) < 0) {
        clock_alarm_time.min = 59;
        return;
    }

    clock_alarm_time.min = (clock_alarm_time.min + delta) % 60;
}

void clock_adjust_alarm_hour(int32_t delta)
{
    if (!is_set_alarm_mode) return;

    if (((int32_t) clock_alarm_time.hour + delta) < 0) {
        clock_alarm_time.hour = 23;
        return;
    }

    clock_alarm_time.hour = (clock_alarm_time.hour + delta) % 24;
}

esp_err_t clock_get_time(clock_time_t *stime, bool *is_being_modified)
{
    ESP_RETURN_ON_FALSE(stime != NULL, ESP_ERR_INVALID_ARG, TAG, "clock_get_time: Pointer argument is NULL");

    if (is_being_modified != NULL) {
        *is_being_modified = is_set_time_mode | is_set_alarm_mode;
    }

    if (is_set_time_mode) {
        stime->sec = clock_time.sec;
        stime->min = clock_time.min;
        stime->hour = clock_time.hour;

        return ESP_OK;
    } else if (is_set_alarm_mode) {
        stime->sec = clock_alarm_time.sec;
        stime->min = clock_alarm_time.min;
        stime->hour = clock_alarm_time.hour;

        return ESP_OK;
    }

    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    stime->sec = timeinfo.tm_sec;
    stime->min = timeinfo.tm_min;
    stime->hour = timeinfo.tm_hour;

    /*
     * Trip the alarm for 2 seconds.
     * This shall leave plenty of time given the frequency at which this
     * function is normally called.
     */
    if (timeinfo.tm_min == clock_alarm_time.min &&
        timeinfo.tm_hour == clock_alarm_time.hour &&
        timeinfo.tm_sec < 2) {
        if (!has_alarm_tripped) {
            has_alarm_tripped = 1;
            xTaskCreate(ring_alarm_task, "ring_alarm_task", configMINIMAL_STACK_SIZE, NULL, 10, NULL);
        }
    }

    return ESP_OK;
}

static esp_err_t clock_set_time(void)
{
    time_t now, temp;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    timeinfo.tm_sec = 0;
    timeinfo.tm_min = clock_time.min;
    timeinfo.tm_hour = clock_time.hour;

    temp = mktime(&timeinfo);
    struct timeval new_now = { .tv_sec = temp, .tv_usec = 0 };

    settimeofday(&new_now, NULL);

    return ESP_OK;
}

static void ring_alarm_task(void *arg)
{
    while (clock_is_alarm_ringing()) {
        gpio_set_level(status_led_gpio, 1);
        buzzer_beep(1000, 200);
        gpio_set_level(status_led_gpio, 0);
        vTaskDelay(pdMS_TO_TICKS(100));

        gpio_set_level(status_led_gpio, 1);
        buzzer_beep(1000, 200);
        gpio_set_level(status_led_gpio, 0);
        vTaskDelay(pdMS_TO_TICKS(100));

        gpio_set_level(status_led_gpio, 1);
        buzzer_beep(1000, 200);
        gpio_set_level(status_led_gpio, 0);
        vTaskDelay(pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    buzzer_stop();
    gpio_set_level(status_led_gpio, 0);

    vTaskDelete(NULL);
}

esp_err_t init_status_led(uint32_t gpio)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    status_led_gpio = gpio;

    return gpio_config(&io_conf);
}


esp_err_t clock_init(uint32_t status_led_gpio, uint32_t buzzer_gpio)
{
    esp_err_t rc;
    
    ESP_LOGI(TAG, "Initializing time");

    setenv("TZ", "Europe/Zurich", 1);
    tzset();

    srand(time(NULL));

    rc = buzzer_init(buzzer_gpio);
    if (rc) {
        ESP_LOGE(TAG, "Buzzer initialization failed. (%s)", esp_err_to_name(rc));
        return rc;
    }

    rc = init_status_led(status_led_gpio);
    if (rc) {
        ESP_LOGE(TAG, "Status LED initialization failed. (%s)", esp_err_to_name(rc));
        return rc;
    }

    return ESP_OK;
}
