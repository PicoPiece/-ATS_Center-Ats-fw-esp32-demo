# TEST_PASS: set env TEST_PASS=1 for pass build (Hello/Welcome RKTech), default 0 = PicoPiece
if(DEFINED ENV{TEST_PASS})
    add_compile_definitions(TEST_PASS=$ENV{TEST_PASS})
endif()

set(MY_REQUIRES nvs_flash esp_wifi esp_event esp_netif lwip)

# Detect IDF version via build property (works in both 5.2 and 5.4+)
idf_build_get_property(idf_ver_minor IDF_VERSION_MINOR)
if(idf_ver_minor GREATER_EQUAL 4)
    list(APPEND MY_REQUIRES esp_driver_spi esp_driver_gpio)
else()
    list(APPEND MY_REQUIRES driver esp_smartconfig)
endif()

idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES ${MY_REQUIRES}
)

