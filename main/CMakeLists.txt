# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES "driver" "esp_timer" "esp_lcd" "lwip" "esp_driver_gpio" "esp_driver_i2c")

set(COMPONENT_SRCS "main.c" "lvgl_demo_ui.c" "weather.c" "screen.c" "clock.c" "buzzer.c")
set(COMPONENT_ADD_INCLUDEDIRS "")


register_component()
