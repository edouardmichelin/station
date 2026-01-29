/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "unity.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "aht20.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "aht20 test";

#define TEST_MEMORY_LEAK_THRESHOLD (-400)

#define I2C_MASTER_SCL_IO   CONFIG_I2C_MASTER_SCL   /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO   CONFIG_I2C_MASTER_SDA   /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM      I2C_NUM_0               /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ  100000                  /*!< I2C master clock frequency */

static aht20_dev_handle_t aht20_handle = NULL;

static i2c_master_bus_handle_t i2c_bushandle = NULL;
/**
 * @brief i2c master initialization
 */
static void i2c_bus_init(void)
{
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    TEST_ASSERT_FALSE_MESSAGE( i2c_new_master_bus(&i2c_bus_config, &i2c_bushandle), "i2c_new_master_bus fail" );
}

static void i2c_sensor_ath20_init(void)
{
    i2c_bus_init();
    
    i2c_aht20_config_t aht20_i2c_config = {
        .i2c_config.device_address = AHT20_ADDRESS_0,
        .i2c_config.scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .i2c_timeout = 100,
    };
    TEST_ASSERT_FALSE_MESSAGE(aht20_new_sensor(i2c_bushandle, &aht20_i2c_config, &aht20_handle), "aht20_new_sensor fail");
    TEST_ASSERT_NOT_NULL_MESSAGE(aht20_handle, "AHT20 create returned NULL");
}

TEST_CASE("sensor aht20 test", "[aht20][iot][sensor]")
{
    esp_err_t ret = ESP_OK;
    int16_t temperature_i16;
    int16_t humidity_i16;
    float temperature;
    float humidity;

    i2c_sensor_ath20_init();

    TEST_ASSERT(ESP_OK == aht20_read_float(aht20_handle, &temperature, &humidity));
    ESP_LOGI(TAG, "%-20s: %2.2fdegC", "temperature is", temperature);
    ESP_LOGI(TAG, "%-20s: %2.2f%%", "humidity is", humidity);
    
    TEST_ASSERT(ESP_OK == aht20_read_i16(aht20_handle, &temperature_i16, &humidity_i16));
    ESP_LOGI(TAG, "%-20s: %d", "temperature is", temperature_i16);
    ESP_LOGI(TAG, "%-20s: %d", "humidity is", humidity_i16);

    aht20_del_sensor(aht20_handle);
    ret = i2c_del_master_bus(i2c_bushandle)
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

static size_t before_free_8bit;
static size_t before_free_32bit;

static void check_leak(size_t before_free, size_t after_free, const char *type)
{
    ssize_t delta = after_free - before_free;
    printf("MALLOC_CAP_%s: Before %u bytes free, After %u bytes free (delta %d)\n", type, before_free, after_free, delta);
    TEST_ASSERT_MESSAGE(delta >= TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}

void setUp(void)
{
    before_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    before_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
}

void tearDown(void)
{
    size_t after_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t after_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(before_free_8bit, after_free_8bit, "8BIT");
    check_leak(before_free_32bit, after_free_32bit, "32BIT");
}

void app_main(void)
{
    printf("AHT20 TEST \n");
    unity_run_menu();
}
