#include <string.h>
#include <stdio.h>

#include "driver/gpio.h"

#include "esp_timer.h"

#include "lwip/sys.h"

#include "lvgl.h"

#include "weather_images.h"
#include "weather.h"
#include "clock.h"

#define DEGREE_SYMBOL "\u00B0"

#define WEATHER_SCREEN_REFRESH_RATE CONFIG_WEATHER_SCREEN_REFRESH_RATE_MS

static lv_obj_t *text_label_alarm;
static lv_obj_t *text_label_time;
static lv_obj_t *text_label_temperature;
static lv_obj_t *text_label_humidity;
static lv_obj_t *text_label_pressure;

#define SENSOR_VAL_BUF_SZ 16

static char *get_time(bool *is_being_modified)
{
    clock_time_t stime;
    static char buf[SENSOR_VAL_BUF_SZ];

    memset(buf, 0, sizeof(buf));

    ESP_ERROR_CHECK(clock_get_time(&stime, is_being_modified));

    sprintf(buf, "%02d:%02d:%02d", stime.hour, stime.min, stime.sec);

    return buf;
}

static char *get_temperature(void)
{
    float temp;
    static char buf[SENSOR_VAL_BUF_SZ];

    memset(buf, 0, SENSOR_VAL_BUF_SZ);

    temp = weather_get_temperature();

    snprintf(buf, SENSOR_VAL_BUF_SZ - 1, "%.1f" DEGREE_SYMBOL, temp);

    return buf;
}

static char *get_humidity(void)
{
    float humidity;
    static char buf[SENSOR_VAL_BUF_SZ];

    memset(buf, 0, SENSOR_VAL_BUF_SZ);

    humidity = weather_get_humidity();

    snprintf(buf, SENSOR_VAL_BUF_SZ - 1, "%.0f%%", humidity);

    return buf;
}

static char *get_pressure(void)
{
    float pressure;
    static char buf[SENSOR_VAL_BUF_SZ];

    memset(buf, 0, SENSOR_VAL_BUF_SZ);

    pressure = weather_get_pressure();

    snprintf(buf, SENSOR_VAL_BUF_SZ - 1, "%.0f hPa", pressure);

    return buf;
}

static int is_alarm_set(void)
{
    return clock_is_alarm_on();
}

static uint8_t time_display_toggle = 0;

static void timer_cb(lv_timer_t * timer)
{
    LV_UNUSED(timer);

    char *time_str;
    bool time_is_being_modified;

    lv_label_set_text(text_label_temperature, get_temperature());
    lv_label_set_text(text_label_humidity, get_humidity());
    lv_label_set_text(text_label_pressure, get_pressure());
    lv_label_set_text(text_label_alarm, is_alarm_set() ? LV_SYMBOL_VOLUME_MAX : "");

    time_str = get_time(&time_is_being_modified);

    if (time_is_being_modified) {
        /* Hide text every 4 increments */
        if (time_display_toggle++ & 0b100) {
            lv_label_set_text(text_label_time, time_str);
        } else {
            lv_label_set_text(text_label_time, "");
        }
    } else {
        lv_label_set_text(text_label_time, time_str);
    }
}

void lv_create_main_gui(void)
{
  LV_IMAGE_DECLARE(image_weather_temperature);
  LV_IMAGE_DECLARE(image_weather_humidity);
  LV_IMAGE_DECLARE(image_weather_pressure);

  text_label_time = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_time, "00:00:00");
  lv_obj_align(text_label_time, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_time, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color((lv_obj_t*) text_label_time, lv_palette_main(LV_PALETTE_TEAL), 0);

  lv_obj_t * weather_image_temperature = lv_image_create(lv_screen_active());
  lv_image_set_src(weather_image_temperature, &image_weather_temperature);
  lv_obj_align(weather_image_temperature, LV_ALIGN_BOTTOM_LEFT, 35, 0);
  text_label_temperature = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_temperature, "00.0" DEGREE_SYMBOL);
  lv_obj_align(text_label_temperature, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_temperature, &lv_font_montserrat_16, 0);

  lv_obj_t * weather_image_humidity = lv_image_create(lv_screen_active());
  lv_image_set_src(weather_image_humidity, &image_weather_humidity);
  lv_obj_align(weather_image_humidity, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
  text_label_humidity = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_humidity, "00%");
  lv_obj_align(text_label_humidity, LV_ALIGN_BOTTOM_RIGHT, -25, 0);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_humidity, &lv_font_montserrat_16, 0);

  // lv_obj_t * weather_image_pressure = lv_image_create(lv_screen_active());
  // lv_image_set_src(weather_image_pressure, &image_weather_pressure);
  // lv_obj_align(weather_image_pressure, LV_ALIGN_CENTER, 40, 0);
  text_label_pressure = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_pressure, "0.000 hPa");
  lv_obj_align(text_label_pressure, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_pressure, &lv_font_montserrat_16, 0);

  text_label_alarm = lv_label_create(lv_screen_active());
  lv_label_set_text(text_label_alarm, LV_SYMBOL_VOLUME_MAX);
  lv_obj_align(text_label_alarm, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_text_font((lv_obj_t*) text_label_alarm, &lv_font_montserrat_16, 0);

  lv_timer_t * timer = lv_timer_create(timer_cb, WEATHER_SCREEN_REFRESH_RATE, NULL);
  lv_timer_ready(timer);
}