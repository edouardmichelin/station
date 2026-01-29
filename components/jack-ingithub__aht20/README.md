[![Component Registry](https://components.espressif.com/components/espressif/aht20/badge.svg)](https://components.espressif.com/components/espressif/aht20)

# Component: AHT20
I2C driver and definition of AHT20 humidity and temperature sensor. 

Components compatible with AHT30 and AHT21 (AHT21 is deprecated).

See [AHT20 datasheet](http://www.aosong.com/en/products-32.html), [AHT30 datasheet](http://www.aosong.com/en/products-131.html).


## Usage

### Initialization I2C Master Bus
> Note: Note: You need to initialize the I2C bus first.
```c
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&i2c_bus_config, &i2c_bushandle);
```

### Create AHT20 Sensor
> Note: You need to create the AHT20 Sensor object first.
```c
    aht20_dev_handle_t aht20_handle = NULL;
    i2c_aht20_config_t aht20_i2c_config = {
        .i2c_config.device_address = AHT20_ADDRESS_0,
        .i2c_config.scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .i2c_timeout = 100,
    };
    aht20_new_sensor(i2c_bushandle, &aht20_i2c_config, &aht20_handle);
```

### Read data
> The user can periodically call the aht20_read_float API to retrieve real-time data.
```c
    float temp, hum;

    aht20_read_float(aht20, &temp, &hum);
    ESP_LOGI(TAG, "Humidity      : %2.2f %%", hum);
    ESP_LOGI(TAG, "Temperature   : %2.2f degC", temp);
```
