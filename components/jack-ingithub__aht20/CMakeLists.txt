idf_component_register(
    SRCS "aht20.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES "driver" "esp_driver_i2c" "esp_driver_gpio"
)

include(package_manager)
cu_pkg_define_version(${CMAKE_CURRENT_LIST_DIR})
