/*****************************************************************************
 *
 * @file:    aht20.h
 * @brief:   
 * @author:  oikiou <oikiou@outlook.com>
 * @date:    2024/12/15
 * @version: v0.0.1
 * @history: 
 * <author>    <time>     <version>      <desc> 
 * | oikiou | 2024/12/15 | v0.0.1 | First version |
 *
 ****************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Includes ------------------------------------------------------------------*/
#include "esp_types.h"
#include "esp_err.h"

#include "driver/i2c_master.h"

/* Config --------------------------------------------------------------------*/

/* AHT20 address: CE pin low - 0x38, CE pin high - 0x39 */
#define AHT20_ADDRESS_0         (0x38)
#define AHT20_ADDRESS_1         (0x39)

/* Macro ---------------------------------------------------------------------*/

/* Types ---------------------------------------------------------------------*/

/**
 * @brief   AHT20 device struct
 */
typedef struct aht20_dev_s{
    i2c_master_dev_handle_t     i2c_dev;
    uint16_t                    i2c_timeout;    /*!< i2c operation timeout */
} aht20_dev_t;

/**
 * @brief Type of AHT20 device handle
 */
typedef struct aht20_dev_s *aht20_dev_handle_t;

/**
 * @brief   AHT20 I2C config struct
 */
typedef struct {
    i2c_device_config_t i2c_config;             /*!< Configuration for eeprom device */
    uint16_t            i2c_timeout;            /*!< i2c operation timeout */
} i2c_aht20_config_t;

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/

/**
 * @brief Create new AHT20 device handle.
 *
 * @param[in]  bus_handle I2C master bus handle
 * @param[in]  i2c_conf Config for I2C used by AHT20
 * @param[out] handle_out New AHT20 device handle
 * @return
 *          - ESP_OK                  Device handle creation success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 *          - ESP_ERR_NO_MEM          Memory allocation failed.
 *
 */
esp_err_t aht20_new_sensor(const i2c_master_bus_handle_t bus_handle, const i2c_aht20_config_t *i2c_config, aht20_dev_handle_t *out_handle);

/**
 * @brief Delete AHT20 device handle.
 *
 * @param[in] handle AHT20 device handle
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 *
 */
esp_err_t aht20_del_sensor(aht20_dev_handle_t *handle);

/**
 * @brief read the temperature and humidity data float
 *
 * @param[in]  *handle points to an aht20 handle structure
 * @param[out] *temperature points to a converted temperature buffer
 * @param[out] *humidity points to a converted humidity buffer
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t aht20_read_float(     aht20_dev_handle_t handle,
                                float *temperature,
                                float *humidity);

/**
 * @brief read the temperature and humidity data 
 * int16 Data expanded a hundred times.
 *
 * @param[in]  *handle points to an aht20 handle structure
 * @param[out] *temperature points to a converted temperature buffer
 * @param[out] *humidity points to a converted humidity buffer
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t aht20_read_i16(       aht20_dev_handle_t handle,
                                int16_t *temperature,
                                int16_t *humidity);


#ifdef __cplusplus /* end of __cplusplus */
}
#endif
/* --------------------------------END OF FILE------------------------------- */
