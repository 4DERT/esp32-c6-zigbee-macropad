idf_component_register(
  SRCS "lsfx.c" "lsfx_fx_rainbow.c" "lsfx_fx_police.c" "lsfx_fx_static.c" "lsfx_fx_bicolor.c" "lsfx_fx_loading.c"
  INCLUDE_DIRS "include"
  PRIV_REQUIRES "led_strip esp_driver_gpio freertos"
)
