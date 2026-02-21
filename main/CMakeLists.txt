# TEST_PASS: set env TEST_PASS=1 for pass build (Hello/Welcome RKTech), default 0 = PicoPiece
if(DEFINED ENV{TEST_PASS})
    add_compile_definitions(TEST_PASS=$ENV{TEST_PASS})
endif()

set(MY_REQUIRES nvs_flash esp_wifi esp_event esp_netif lwip)

# IDF 5.4+: esp_smartconfig removed, SPI/GPIO split into separate components
# IDF 5.2:  esp_smartconfig exists, SPI/GPIO inside 'driver'
if(IDF_VERSION_MINOR GREATER_EQUAL 4)
    list(APPEND MY_REQUIRES esp_driver_spi esp_driver_gpio)
else()
    list(APPEND MY_REQUIRES driver esp_smartconfig)
endif()

idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES ${MY_REQUIRES}
)

