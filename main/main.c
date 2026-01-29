#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/lock.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"

#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "lwip/sys.h"

#include "clock.h"
#include "weather.h"
#include "screen.h"
#include "buzzer.h"

static const char *TAG = "MAIN";

#define BUTTON_CTRL_GPIO GPIO_NUM_0
#define BUTTON_UP_GPIO GPIO_NUM_1
#define BUTTON_DOWN_GPIO GPIO_NUM_2
#define SWITCH_GPIO GPIO_NUM_3
#define ALARM_BUZZER_GPIO GPIO_NUM_4
#define AHT20_STATUS_LED_GPIO GPIO_NUM_10
#define BMP280_STATUS_LED_GPIO GPIO_NUM_20
#define CLOCK_STATUS_LED_GPIO GPIO_NUM_21

#define I2C_BUS_PORT 0
#define I2C_PIN_NUM_SDA GPIO_NUM_8
#define I2C_PIN_NUM_SCL GPIO_NUM_9

#define STATE_NORMAL 0
#define STATE_SET_TIME 1
#define STATE_SET_ALARM 2

#define MAX_SET_TIME_STAGE 2

enum set_time_stage {
    SET_HOURS = 0,
    SET_MINUTES = 1
};

uint8_t set_time_done;
static enum set_time_stage stage;

uint8_t station_state = STATE_NORMAL;

static int button_pressed = 0;
static int button_up_pressed = 0;
static int button_down_pressed = 0;
static int switch_on = 0;

static int consume_is_btn_pressed(void)
{
    int val = button_pressed;
    button_pressed = 0;
    return val;
}

static int consume_is_btn_up_pressed(void)
{
    int val = button_up_pressed;
    button_up_pressed = 0;
    return val;
}

static int consume_is_btn_down_pressed(void)
{
    int val = button_down_pressed;
    button_down_pressed = 0;
    return val;
}

static int get_is_switch_on(void)
{
    return switch_on;
}


static esp_err_t init_i2c_master_bus(i2c_master_bus_handle_t *i2c_bus_handle)
{
    ESP_RETURN_ON_FALSE(i2c_bus_handle != NULL, ESP_ERR_INVALID_ARG, TAG, "init_i2c_master_bus: pointer to I2C master bus handle is NULL");

    ESP_LOGI(TAG, "Initializing I2C master bus");

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = I2C_PIN_NUM_SDA,
        .scl_io_num = I2C_PIN_NUM_SCL,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&i2c_bus_config, i2c_bus_handle);
}

static void set_time_task(void *arg)
{
    int ctrl = 0, up = 0, down = 0;
    void (*enter_set)(void);
    void (*exit_set)(void);
    void (*adjust_hour)(int32_t);
    void (*adjust_min)(int32_t);

    stage = SET_HOURS;

    switch (station_state) {
        case STATE_SET_TIME:
            enter_set = clock_enter_set_time;
            exit_set = clock_exit_set_time;
            adjust_hour = clock_adjust_time_hour;
            adjust_min = clock_adjust_time_min;
            break;
        case STATE_SET_ALARM:
            enter_set = clock_enter_set_alarm;
            exit_set = clock_exit_set_alarm;
            adjust_hour = clock_adjust_alarm_hour;
            adjust_min = clock_adjust_alarm_min;
            break;
        default:
            vTaskDelete(NULL);
            return;
    }

    enter_set();

    while (!set_time_done) {
        ctrl = consume_is_btn_pressed();
        up = consume_is_btn_up_pressed();
        down = consume_is_btn_down_pressed();

        if (up) {
            switch (stage) {
                case SET_HOURS:
                    adjust_hour(1);
                    break;
                case SET_MINUTES:
                    adjust_min(1);
                    break;
                default: break;
            }
        } else if (down) {
            switch (stage) {
                case SET_HOURS:
                    adjust_hour(-1);
                    break;
                case SET_MINUTES:
                    adjust_min(-1);
                    break;
                default: break;
            }
        } else if (ctrl) {
            stage++;
        }

        set_time_done = stage >= MAX_SET_TIME_STAGE;

        vTaskDelay(pdMS_TO_TICKS(50));
    } 

    exit_set();

    vTaskDelete(NULL);
}


static void button_task(void *arg)
{
    uint8_t debounce;

    for(;;) {
        button_pressed |= !gpio_get_level(BUTTON_CTRL_GPIO);
        button_up_pressed |= !gpio_get_level(BUTTON_UP_GPIO);
        button_down_pressed |= !gpio_get_level(BUTTON_DOWN_GPIO);
        switch_on = !gpio_get_level(SWITCH_GPIO);

        debounce = button_pressed | button_up_pressed | button_down_pressed;

        switch (station_state) {
            case STATE_NORMAL:
                if (get_is_switch_on() && !clock_is_alarm_on()) {
                    set_time_done = 0;
                    station_state = STATE_SET_ALARM;
                    clock_enable_alarm();
                    xTaskCreate(set_time_task, "set_time_task", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
                } else if (!get_is_switch_on()) {
                    clock_disable_alarm();
                }
                
                if (consume_is_btn_pressed()) {
                    set_time_done = 0;
                    station_state = STATE_SET_TIME;
                    xTaskCreate(set_time_task, "set_time_task", configMINIMAL_STACK_SIZE, NULL, 3, NULL);
                }

                break;
            case STATE_SET_TIME:
                if (set_time_done) {
                    station_state = STATE_NORMAL;
                }
                break;
            case STATE_SET_ALARM:
                if (set_time_done) {
                    station_state = STATE_NORMAL;
                }
                break;
            default:
                break;
        }

        vTaskDelay(pdMS_TO_TICKS(50 + 300 * debounce));
    }

    vTaskDelete(NULL);
}

static void init_control_buttons(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_CTRL_GPIO) | (1ULL << BUTTON_UP_GPIO) | (1ULL << BUTTON_DOWN_GPIO) | (1ULL << SWITCH_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);

    xTaskCreate(button_task, "button_task", configMINIMAL_STACK_SIZE, NULL, 4, NULL);
}

void app_main(void)
{
    i2c_master_bus_handle_t i2c_bus_handle = NULL;

    station_state = STATE_NORMAL;

    ESP_ERROR_CHECK(init_i2c_master_bus(&i2c_bus_handle));

    clock_init(CLOCK_STATUS_LED_GPIO, ALARM_BUZZER_GPIO);

    init_control_buttons();

    weather_init_sensors(i2c_bus_handle, AHT20_STATUS_LED_GPIO, BMP280_STATUS_LED_GPIO);

    screen_init(i2c_bus_handle);
}
