/*****************************************************************************
 *
 * @file:    bmp280.c
 * @brief:   BMP280 driver 
 * @author:  oikiou <oikiou@outlook.com>
 * @date:    2024/12/15
 * @version: v0.0.1
 * @history: 
 * <author>    <time>     <version>      <desc> 
 * | oikiou |  2024/12/15  | v0.0.1 | First version |
 *
 ****************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "aht20.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#include "aht20_reg.h"
/* Config --------------------------------------------------------------------*/

/* Macro ---------------------------------------------------------------------*/

/* Types ---------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

const static char *TAG = "AHT20";
/* Functions -----------------------------------------------------------------*/
static uint8_t aht20_calc_crc(uint8_t *data, uint8_t len);

/* Functions Prototypes ------------------------------------------------------*/
static uint8_t aht20_calc_crc(uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t byte;
    uint8_t crc = 0xFF;

    for (byte = 0; byte < len; byte++) {
        crc ^= data[byte];
        for (i = 8; i > 0; --i) {
            if ((crc & 0x80) != 0) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

esp_err_t aht20_read_float( aht20_dev_handle_t handle,
                            float *temperature,
                            float *humidity)
{
    uint8_t status;
    uint8_t buf[7];
    uint32_t raw_data;
    uint8_t timeout = 0;
    
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");
    
    buf[0] = AHT20_START_MEASURMENT_CMD;
    buf[1] = 0x33;
    buf[2] = 0x00;
    ESP_RETURN_ON_ERROR(i2c_master_transmit(handle->i2c_dev, buf, 3, handle->i2c_timeout), TAG, "");
    
    while (1) {
        ESP_RETURN_ON_ERROR(i2c_master_receive(handle->i2c_dev, &status, 1, handle->i2c_timeout), TAG, "");
        if ((status & BIT(AT581X_STATUS_BUSY_INDICATION)) == 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        if (++timeout >= 16) {
            ESP_LOGI(TAG, "Timeout waiting for IDLE");
            return ESP_ERR_NOT_FINISHED;
        }
    }
    
    if ((status & BIT(AT581X_STATUS_Calibration_Enable)) &&
            // (status & BIT(AT581X_STATUS_CRC_FLAG)) &&
            ((status & BIT(AT581X_STATUS_BUSY_INDICATION)) == 0)) {
        ESP_RETURN_ON_ERROR(i2c_master_receive(handle->i2c_dev, buf, 7, handle->i2c_timeout), TAG, "");
        ESP_RETURN_ON_ERROR((aht20_calc_crc(buf, 6) != buf[6]), TAG, "crc is error");

        raw_data = buf[1];
        raw_data = raw_data << 8;
        raw_data += buf[2];
        raw_data = raw_data << 8;
        raw_data += buf[3];
        raw_data = raw_data >> 4;
        *humidity = (float)raw_data * 100 / 1048576;

        raw_data = buf[3] & 0x0F;
        raw_data = raw_data << 8;
        raw_data += buf[4];
        raw_data = raw_data << 8;
        raw_data += buf[5];
        *temperature = (float)raw_data * 200 / 1048576 - 50;
        return ESP_OK;
    } else {
        ESP_LOGI(TAG, "data is not ready");
        return ESP_ERR_NOT_FINISHED;
    }
}

esp_err_t aht20_read_i16(   aht20_dev_handle_t handle,
                            int16_t *temperature,
                            int16_t *humidity)
{
    uint8_t status;
    uint8_t buf[7];
    uint32_t raw_data;
    uint8_t timeout = 0;
    
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");
    
    buf[0] = AHT20_START_MEASURMENT_CMD;
    buf[1] = 0x33;
    buf[2] = 0x00;
    ESP_RETURN_ON_ERROR(i2c_master_transmit(handle->i2c_dev, buf, 3, handle->i2c_timeout), TAG, "");
    
    while (1) {
        ESP_RETURN_ON_ERROR(i2c_master_receive(handle->i2c_dev, &status, 1, handle->i2c_timeout), TAG, "");
        if ((status & BIT(AT581X_STATUS_BUSY_INDICATION)) == 0) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        if (++timeout >= 16) {
            ESP_LOGE(TAG, "Timeout waiting for IDLE");
            return ESP_ERR_NOT_FINISHED;
        }
    }
    
    if ((status & BIT(AT581X_STATUS_Calibration_Enable)) &&
            (status & BIT(AT581X_STATUS_CRC_FLAG)) &&
            ((status & BIT(AT581X_STATUS_BUSY_INDICATION)) == 0)) {
        ESP_RETURN_ON_ERROR(i2c_master_receive(handle->i2c_dev, buf, 7, handle->i2c_timeout), TAG, "");
        ESP_RETURN_ON_ERROR((aht20_calc_crc(buf, 6) != buf[6]), TAG, "crc is error");

        raw_data = buf[1];
        raw_data = raw_data << 8;
        raw_data += buf[2];
        raw_data = raw_data << 8;
        raw_data += buf[3];
        raw_data = raw_data >> 4;
        *humidity = (raw_data+52)*625>>16;

        raw_data = buf[3] & 0x0F;
        raw_data = raw_data << 8;
        raw_data += buf[4];
        raw_data = raw_data << 8;
        raw_data += buf[5];
        *temperature = ((raw_data+26)*625>>15) - 5000;
        
        return ESP_OK;
    } else {
        ESP_LOGI(TAG, "data is not ready");
        return ESP_ERR_NOT_FINISHED;
    }
}

esp_err_t aht20_new_sensor(const i2c_master_bus_handle_t bus_handle, const i2c_aht20_config_t *i2c_config, aht20_dev_handle_t *out_handle)
{
    esp_err_t ret = ESP_OK;
    ESP_LOGI(TAG, "%-15s:", CHIP_NAME);
    ESP_LOGI(TAG, "%-15s: %1.1f - %1.1fV", "SUPPLY_VOLTAGE", SUPPLY_VOLTAGE_MIN, SUPPLY_VOLTAGE_MAX);
    ESP_LOGI(TAG, "%-15s: %.2f - %.2f℃", "TEMPERATURE", TEMPERATURE_MIN, TEMPERATURE_MAX);
    
    ESP_RETURN_ON_FALSE(bus_handle, ESP_ERR_INVALID_ARG, TAG, "invalid pointer");
    ESP_RETURN_ON_FALSE(i2c_config, ESP_ERR_INVALID_ARG, TAG, "invalid pointer");
    ESP_RETURN_ON_FALSE(out_handle, ESP_ERR_INVALID_ARG, TAG, "invalid pointer");
    
    aht20_dev_handle_t aht20_dev_handle = NULL;
    aht20_dev_handle = calloc(1, sizeof(struct aht20_dev_s));
    ESP_RETURN_ON_FALSE(aht20_dev_handle, ESP_ERR_NO_MEM, TAG, "no memory");
    
    i2c_device_config_t i2c_dev_conf = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = i2c_config->i2c_config.device_address,
        .scl_speed_hz = i2c_config->i2c_config.scl_speed_hz,
        .scl_wait_us = 0,
        .flags.disable_ack_check = false,
    };
    ESP_GOTO_ON_ERROR(i2c_master_bus_add_device(bus_handle, &i2c_dev_conf, &aht20_dev_handle->i2c_dev), ERR_EXIT, TAG, "i2c new bus failed");
    aht20_dev_handle->i2c_timeout = i2c_config->i2c_timeout;
    
    *out_handle = aht20_dev_handle;
    ESP_LOGD(TAG, "%s Success.[%p]", __func__, aht20_dev_handle);
    return ESP_OK;
    
ERR_EXIT:
    if (aht20_dev_handle != NULL) {
        free(aht20_dev_handle);
        aht20_dev_handle = NULL;
    }
    *out_handle = NULL;
    ESP_LOGW(TAG, "%s Faild.", __func__);
    return ret;
}

esp_err_t aht20_del_sensor(aht20_dev_handle_t *handle)
{
    aht20_dev_handle_t aht20_handle = *handle;
    ESP_RETURN_ON_FALSE(aht20_handle, ESP_ERR_INVALID_ARG, TAG, "invalid pointer");
    
    ESP_RETURN_ON_ERROR(i2c_master_bus_rm_device(aht20_handle->i2c_dev), TAG, "i2c rm bus failed");
    memset(aht20_handle, 0, sizeof(struct aht20_dev_s));
    free(aht20_handle);
    *handle = NULL;
    
    ESP_LOGD(TAG, "%s Success.", __func__);
    return ESP_OK;
}
