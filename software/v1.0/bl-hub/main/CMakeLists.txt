set(srcs
     "itag.c"
     "ab_shutter.c"
     "init.c"
     "nvs_store.c")
set(include_dirs ".")

idf_component_register(SRCS "main.c" ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES esp_hid driver
                       PRIV_REQUIRES nvs_flash)