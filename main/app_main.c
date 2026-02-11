#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

static const char *TAG = "ats-fw-demo";

/* 5x7 font: 5 bytes per char (columns), bit 0 = top row. ASCII 0x20-0x5F, 0x61-0x7A */
#define FONT5X7_W 5
#define FONT5X7_H 7
#define FONT5X7_STRIDE 6  /* 5 px + 1 gap */
static const uint8_t font_5x7[][5] = {
    [0]  = { 0x00, 0x00, 0x00, 0x00, 0x00 }, /* space */
    [1]  = { 0x00, 0x00, 0x5F, 0x00, 0x00 }, /* ! */
    [2]  = { 0x00, 0x07, 0x00, 0x07, 0x00 }, /* " */
    [3]  = { 0x14, 0x7F, 0x14, 0x7F, 0x14 }, /* # */
    [4]  = { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, /* $ */
    [5]  = { 0x23, 0x13, 0x08, 0x64, 0x62 }, /* % */
    [6]  = { 0x36, 0x49, 0x56, 0x20, 0x50 }, /* & */
    [7]  = { 0x00, 0x08, 0x07, 0x03, 0x00 }, /* ' */
    [8]  = { 0x00, 0x1C, 0x22, 0x41, 0x00 }, /* ( */
    [9]  = { 0x00, 0x41, 0x22, 0x1C, 0x00 }, /* ) */
    [10] = { 0x2A, 0x1C, 0x7F, 0x1C, 0x2A }, /* * */
    [11] = { 0x08, 0x08, 0x3E, 0x08, 0x08 }, /* + */
    [12] = { 0x00, 0x80, 0x70, 0x30, 0x00 }, /* , */
    [13] = { 0x08, 0x08, 0x08, 0x08, 0x08 }, /* - */
    [14] = { 0x00, 0x00, 0x60, 0x60, 0x00 }, /* . */
    [15] = { 0x20, 0x10, 0x08, 0x04, 0x02 }, /* / */
    [16] = { 0x3E, 0x51, 0x49, 0x45, 0x3E }, /* 0 */
    [17] = { 0x00, 0x42, 0x7F, 0x40, 0x00 }, /* 1 */
    [18] = { 0x72, 0x49, 0x49, 0x49, 0x46 }, /* 2 */
    [19] = { 0x21, 0x41, 0x49, 0x4D, 0x33 }, /* 3 */
    [20] = { 0x18, 0x14, 0x12, 0x7F, 0x10 }, /* 4 */
    [21] = { 0x27, 0x45, 0x45, 0x45, 0x39 }, /* 5 */
    [22] = { 0x3C, 0x4A, 0x49, 0x49, 0x31 }, /* 6 */
    [23] = { 0x41, 0x21, 0x11, 0x09, 0x07 }, /* 7 */
    [24] = { 0x36, 0x49, 0x49, 0x49, 0x36 }, /* 8 */
    [25] = { 0x46, 0x49, 0x49, 0x29, 0x1E }, /* 9 */
    [26] = { 0x00, 0x36, 0x36, 0x00, 0x00 }, /* : */
    [27] = { 0x00, 0x56, 0x36, 0x00, 0x00 }, /* ; */
    [28] = { 0x08, 0x14, 0x22, 0x41, 0x00 }, /* < */
    [29] = { 0x14, 0x14, 0x14, 0x14, 0x14 }, /* = */
    [30] = { 0x00, 0x41, 0x22, 0x14, 0x08 }, /* > */
    [31] = { 0x02, 0x01, 0x59, 0x09, 0x06 }, /* ? */
    [32] = { 0x3E, 0x41, 0x5D, 0x59, 0x4E }, /* @ */
    [33] = { 0x7C, 0x12, 0x11, 0x12, 0x7C }, /* A */
    [34] = { 0x7F, 0x49, 0x49, 0x49, 0x36 }, /* B */
    [35] = { 0x3E, 0x41, 0x41, 0x41, 0x22 }, /* C */
    [36] = { 0x7F, 0x41, 0x41, 0x41, 0x3E }, /* D */
    [37] = { 0x7F, 0x49, 0x49, 0x49, 0x41 }, /* E */
    [38] = { 0x7F, 0x09, 0x09, 0x09, 0x01 }, /* F */
    [39] = { 0x3E, 0x41, 0x41, 0x51, 0x73 }, /* G */
    [40] = { 0x7F, 0x08, 0x08, 0x08, 0x7F }, /* H */
    [41] = { 0x00, 0x41, 0x7F, 0x41, 0x00 }, /* I */
    [42] = { 0x20, 0x40, 0x41, 0x3F, 0x01 }, /* J */
    [43] = { 0x7F, 0x08, 0x14, 0x22, 0x41 }, /* K */
    [44] = { 0x7F, 0x40, 0x40, 0x40, 0x40 }, /* L */
    [45] = { 0x7F, 0x02, 0x0C, 0x02, 0x7F }, /* M */
    [46] = { 0x7F, 0x04, 0x08, 0x10, 0x7F }, /* N */
    [47] = { 0x3E, 0x41, 0x41, 0x41, 0x3E }, /* O */
    [48] = { 0x7F, 0x09, 0x09, 0x09, 0x06 }, /* P */
    [49] = { 0x3E, 0x41, 0x51, 0x21, 0x5E }, /* Q */
    [50] = { 0x7F, 0x09, 0x19, 0x29, 0x46 }, /* R */
    [51] = { 0x26, 0x49, 0x49, 0x49, 0x32 }, /* S */
    [52] = { 0x01, 0x01, 0x7F, 0x01, 0x01 }, /* T */
    [53] = { 0x3F, 0x40, 0x40, 0x40, 0x3F }, /* U */
    [54] = { 0x1F, 0x20, 0x40, 0x20, 0x1F }, /* V */
    [55] = { 0x7F, 0x20, 0x18, 0x20, 0x7F }, /* W */
    [56] = { 0x63, 0x14, 0x08, 0x14, 0x63 }, /* X */
    [57] = { 0x07, 0x08, 0x70, 0x08, 0x07 }, /* Y */
    [58] = { 0x61, 0x59, 0x49, 0x4D, 0x43 }, /* Z */
    [59] = { 0x00, 0x7F, 0x41, 0x41, 0x41 }, /* [ */
    [60] = { 0x02, 0x04, 0x08, 0x10, 0x20 }, /* \ */
    [61] = { 0x00, 0x41, 0x41, 0x41, 0x7F }, /* ] */
    [62] = { 0x04, 0x02, 0x01, 0x02, 0x04 }, /* ^ */
    [63] = { 0x40, 0x40, 0x40, 0x40, 0x40 }, /* _ */
};
/* lowercase a-z: index 64-89 (0x61-0x7A mapped to 64-89) */
static const uint8_t font_5x7_lower[26][5] = {
    { 0x20, 0x54, 0x54, 0x54, 0x78 }, /* a */
    { 0x7F, 0x48, 0x44, 0x44, 0x38 }, /* b */
    { 0x38, 0x44, 0x44, 0x44, 0x20 }, /* c */
    { 0x38, 0x44, 0x44, 0x48, 0x7F }, /* d */
    { 0x38, 0x54, 0x54, 0x54, 0x18 }, /* e */
    { 0x08, 0x7E, 0x09, 0x01, 0x02 }, /* f */
    { 0x18, 0xA4, 0xA4, 0xA4, 0x7C }, /* g */
    { 0x7F, 0x08, 0x04, 0x04, 0x78 }, /* h */
    { 0x00, 0x44, 0x7D, 0x40, 0x00 }, /* i */
    { 0x20, 0x40, 0x44, 0x3D, 0x00 }, /* j */
    { 0x7F, 0x10, 0x28, 0x44, 0x00 }, /* k */
    { 0x00, 0x41, 0x7F, 0x40, 0x00 }, /* l */
    { 0x7C, 0x04, 0x18, 0x04, 0x78 }, /* m */
    { 0x7C, 0x08, 0x04, 0x04, 0x78 }, /* n */
    { 0x38, 0x44, 0x44, 0x44, 0x38 }, /* o */
    { 0xFC, 0x24, 0x24, 0x24, 0x18 }, /* p */
    { 0x18, 0x24, 0x24, 0x18, 0xFC }, /* q */
    { 0x7C, 0x08, 0x04, 0x04, 0x08 }, /* r */
    { 0x48, 0x54, 0x54, 0x54, 0x20 }, /* s */
    { 0x04, 0x3F, 0x44, 0x40, 0x20 }, /* t */
    { 0x3C, 0x40, 0x40, 0x20, 0x7C }, /* u */
    { 0x1C, 0x20, 0x40, 0x20, 0x1C }, /* v */
    { 0x3C, 0x40, 0x30, 0x40, 0x3C }, /* w */
    { 0x44, 0x28, 0x10, 0x28, 0x44 }, /* x */
    { 0x1C, 0xA0, 0xA0, 0xA0, 0x7C }, /* y */
    { 0x44, 0x64, 0x54, 0x4C, 0x44 }, /* z */
};

/* TFT 160x128 SPI pins - wire hardware to these GPIOs */
#if defined(SPI2_HOST)
#define TFT_SPI_HOST         SPI2_HOST
#else
#define TFT_SPI_HOST         HSPI_HOST   /* IDF 4.x */
#endif
#define TFT_SCK_GPIO         18
#define TFT_MOSI_GPIO        23   /* SDA on display module */
#define TFT_CS_GPIO          4
#define TFT_RST_GPIO         15  /* Res */
#define TFT_DC_GPIO          32  /* Rs (DC/register select) */
#define TFT_SPI_CLOCK_HZ     (10 * 1000 * 1000)

#define TFT_WIDTH            160
#define TFT_HEIGHT           128

/* ST7735 / ST77xx commands */
#define ST77XX_SWRESET   0x01
#define ST77XX_SLPOUT    0x11
#define ST77XX_COLMOD    0x3A
#define ST77XX_MADCTL    0x36
#define ST77XX_CASET     0x2A
#define ST77XX_RASET     0x2B
#define ST77XX_RAMWR     0x2C
#define ST77XX_DISPON    0x29

static spi_device_handle_t tft_spi_handle;

static void tft_write_cmd(uint8_t cmd)
{
    gpio_set_level(TFT_DC_GPIO, 0);
    spi_transaction_t t = { .length = 8, .tx_buffer = &cmd };
    spi_device_polling_transmit(tft_spi_handle, &t);
}

static void tft_write_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    gpio_set_level(TFT_DC_GPIO, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_polling_transmit(tft_spi_handle, &t);
}

static void tft_hw_reset(void)
{
    gpio_set_level(TFT_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(TFT_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
}

static void tft_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    tft_write_cmd(ST77XX_CASET);
    uint8_t col[] = { (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF), (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF) };
    tft_write_data(col, 4);
    tft_write_cmd(ST77XX_RASET);
    uint8_t row[] = { (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF), (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF) };
    tft_write_data(row, 4);
    tft_write_cmd(ST77XX_RAMWR);
}

/* Fill full screen with RGB565 color (big-endian as sent: high byte first). */
static void tft_fill_screen_rgb565(uint16_t color)
{
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    tft_set_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    gpio_set_level(TFT_DC_GPIO, 1);
    const size_t chunk = 1024;
    uint8_t buf[chunk];
    for (size_t i = 0; i < chunk; i += 2) { buf[i] = hi; buf[i + 1] = lo; }
    size_t total = (size_t)TFT_WIDTH * TFT_HEIGHT * 2;
    for (size_t sent = 0; sent < total; sent += chunk) {
        size_t n = (total - sent) < chunk ? (total - sent) : chunk;
        spi_transaction_t t = { .length = n * 8, .tx_buffer = buf };
        spi_device_polling_transmit(tft_spi_handle, &t);
    }
}

/* Draw one 5x7 character at (x,y), fg in RGB565. Background not drawn (transparent). */
static void tft_draw_char_5x7(uint16_t x, uint16_t y, char c, uint16_t fg)
{
    const uint8_t *col;
    if (c >= 0x20 && c <= 0x5F) {
        col = font_5x7[c - 0x20];
    } else if (c >= 'a' && c <= 'z') {
        col = font_5x7_lower[c - 'a'];
    } else {
        col = font_5x7[0];
    }
    uint8_t fg_hi = (uint8_t)(fg >> 8), fg_lo = (uint8_t)(fg & 0xFF);
    for (int cx = 0; cx < FONT5X7_W; cx++) {
        for (int row = 0; row < FONT5X7_H; row++) {
            if ((col[cx] >> row) & 1) {
                tft_set_window(x + cx, y + row, x + cx, y + row);
                gpio_set_level(TFT_DC_GPIO, 1);
                uint8_t px[] = { fg_hi, fg_lo };
                spi_transaction_t t = { .length = 16, .tx_buffer = px };
                spi_device_polling_transmit(tft_spi_handle, &t);
            }
        }
    }
}

/* Draw string at (x,y), return pixel width. */
static int tft_draw_string_5x7(uint16_t x, uint16_t y, const char *s, uint16_t fg)
{
    int cx = 0;
    while (*s) {
        tft_draw_char_5x7(x + cx, y, *s, fg);
        cx += FONT5X7_STRIDE;
        s++;
    }
    return cx;
}

/* String pixel width (for centering). */
static int tft_string_width_px(const char *s)
{
    return (int)strlen(s) * FONT5X7_STRIDE;
}

static void tft_display_init(void)
{
    tft_hw_reset();

    tft_write_cmd(ST77XX_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));

    tft_write_cmd(ST77XX_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(255));

    tft_write_cmd(ST77XX_COLMOD);
    uint8_t colmod = 0x05;  /* 16-bit RGB565 */
    tft_write_data(&colmod, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    tft_write_cmd(ST77XX_MADCTL);
    uint8_t madctl = 0x08;  /* Row/col, bottom-top */
    tft_write_data(&madctl, 1);

    tft_write_cmd(ST77XX_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static esp_err_t tft_spi_init(void)
{
    /* Control pins: RST, DC */
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << TFT_RST_GPIO) | (1ULL << TFT_DC_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io);
    if (err != ESP_OK) return err;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = TFT_MOSI_GPIO,
        .miso_io_num = -1,
        .sclk_io_num = TFT_SCK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    err = spi_bus_initialize(TFT_SPI_HOST, &bus_cfg, 0);
    if (err != ESP_OK) return err;

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = TFT_SPI_CLOCK_HZ,
        .spics_io_num = TFT_CS_GPIO,
        .queue_size = 7,
    };
    return spi_bus_add_device(TFT_SPI_HOST, &dev_cfg, &tft_spi_handle);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello RKTech");   /* v1.1 - used for test pass/fail */
    ESP_LOGI(TAG, "ATS ESP32 Firmware Demo");
    ESP_LOGI(TAG, "Build successful!");

    esp_err_t err = tft_spi_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "TFT SPI init OK (SCK=%d, SDA/MOSI=%d, CS=%d, Res=%d, Rs=%d)",
                 TFT_SCK_GPIO, TFT_MOSI_GPIO, TFT_CS_GPIO, TFT_RST_GPIO, TFT_DC_GPIO);
        tft_display_init();
        /* LCD colors RGB565: 0x0000=black, 0xFFFF=white, 0x0010=dark blue. Background black, text white for readability. */
        tft_fill_screen_rgb565(0x0000);  /* background black */
        {
            const char *line1 = "Welcome RKTech";
            const char *line2 = "ATS_Demo";
            const int line_h = FONT5X7_H + 1;
            const int gap = 4;
            int block_h = 2 * line_h + gap;
            int y0 = (TFT_HEIGHT - block_h) / 2;
            int x1 = (TFT_WIDTH - tft_string_width_px(line1)) / 2;
            int x2 = (TFT_WIDTH - tft_string_width_px(line2)) / 2;
            uint16_t fg = 0xFFFF;  /* white */
            tft_draw_string_5x7(x1, y0, line1, fg);
            tft_draw_string_5x7(x2, y0 + line_h + gap, line2, fg);
        }
        ESP_LOGI(TAG, "TFT display on (Welcome RKTech / ATS_Demo)");
    } else {
        ESP_LOGW(TAG, "TFT SPI init failed: %s", esp_err_to_name(err));
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Running...");
    }
}

