# TEST_PASS: set env TEST_PASS=1 for pass build (Hello/Welcome RKTech), default 0 = PicoPiece
if(DEFINED ENV{TEST_PASS})
    add_compile_definitions(TEST_PASS=$ENV{TEST_PASS})
endif()

idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_event esp_netif esp_smartconfig lwip
)

