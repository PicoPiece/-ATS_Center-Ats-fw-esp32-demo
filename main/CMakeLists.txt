# TEST_PASS: set env TEST_PASS=1 for pass build (Hello/Welcome RKTech), default 0 = PicoPiece
if(DEFINED ENV{TEST_PASS})
    add_compile_definitions(TEST_PASS=$ENV{TEST_PASS})
endif()

set(MY_REQUIRES nvs_flash esp_wifi esp_event esp_netif lwip)

# IDF 5.x: use split drivers + esp_smartconfig from registry (idf_component.yml)
# IDF 4.4+: use split drivers; older: use driver + esp_smartconfig from tree
idf_build_get_property(idf_ver_major IDF_VERSION_MAJOR)
idf_build_get_property(idf_ver_minor IDF_VERSION_MINOR)
if(idf_ver_major GREATER_EQUAL 5 OR idf_ver_minor GREATER_EQUAL 4)
    list(APPEND MY_REQUIRES esp_driver_spi esp_driver_gpio esp_smartconfig)
else()
    list(APPEND MY_REQUIRES driver esp_smartconfig)
endif()

idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES ${MY_REQUIRES}
)

