#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_smartconfig.h"
#include "esp_sntp.h"

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

/* TFT 128x160 SPI – use VSPI (SPI3); pins 18/23/5 are native VSPI on ESP32 */
#if defined(SPI3_HOST)
#define TFT_SPI_HOST         SPI3_HOST   /* VSPI – native pins: SCK=18, MOSI=23, CS=5 */
#elif defined(VSPI_HOST)
#define TFT_SPI_HOST         VSPI_HOST   /* IDF 4.x */
#else
#define TFT_SPI_HOST         SPI2_HOST   /* fallback */
#endif
#define TFT_SCK_GPIO         18  /* SCLK */
#define TFT_MOSI_GPIO        23  /* SDA */
#define TFT_CS_GPIO          5   /* CS = 5 (sample uses 5; if older board uses 4, change here) */
#define TFT_RST_GPIO         15  /* RST */
#define TFT_DC_GPIO          32  /* DC/AO */
#define TFT_SPI_CLOCK_HZ     (TFT_SPI_CLOCK_SLOW ? (4 * 1000 * 1000) : (10 * 1000 * 1000))

#define TFT_WIDTH            160  /* Sau rotation 1: width=160, height=128 */
#define TFT_HEIGHT           128
#define TFT_COL_OFFSET       0   /* Rotation 1 swap: xstart=rowstart=1 */
#define TFT_ROW_OFFSET       0   /* Rotation 1 swap: ystart=colstart=2 */
#define TFT_SPI_CLOCK_SLOW   1

/* ST7735 / ST77xx commands */
#define ST77XX_SWRESET   0x01
#define ST77XX_SLPOUT    0x11
#define ST77XX_COLMOD    0x3A
#define ST77XX_MADCTL    0x36
#define ST77XX_CASET     0x2A
#define ST77XX_RASET     0x2B
#define ST77XX_RAMWR     0x2C
#define ST77XX_DISPON    0x29
#define ST77XX_NORON     0x13
#define ST77XX_INVOFF    0x20
#define ST77XX_INVON     0x21
#define ST7735_FRMCTR1   0xB1
#define ST7735_FRMCTR2   0xB2
#define ST7735_FRMCTR3   0xB3
#define ST7735_INVCTR    0xB4
#define ST7735_PWCTR1    0xC0
#define ST7735_PWCTR2    0xC1
#define ST7735_PWCTR3    0xC2
#define ST7735_PWCTR4    0xC3
#define ST7735_PWCTR5    0xC4
#define ST7735_VMCTR1    0xC5
#define ST7735_GMCTRP1   0xE0
#define ST7735_GMCTRN1   0xE1

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
    uint16_t c0 = x0 + TFT_COL_OFFSET, c1 = x1 + TFT_COL_OFFSET;
    uint16_t r0 = y0 + TFT_ROW_OFFSET, r1 = y1 + TFT_ROW_OFFSET;
    tft_write_cmd(ST77XX_CASET);
    uint8_t col[] = { (uint8_t)(c0 >> 8), (uint8_t)(c0 & 0xFF), (uint8_t)(c1 >> 8), (uint8_t)(c1 & 0xFF) };
    tft_write_data(col, 4);
    tft_write_cmd(ST77XX_RASET);
    uint8_t row[] = { (uint8_t)(r0 >> 8), (uint8_t)(r0 & 0xFF), (uint8_t)(r1 >> 8), (uint8_t)(r1 & 0xFF) };
    tft_write_data(row, 4);
    tft_write_cmd(ST77XX_RAMWR);
}

/* Fill full screen with RGB565. Some panels need low byte first: use buf[i]=lo, buf[i+1]=hi */
static void tft_fill_screen_rgb565(uint16_t color)
{
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    tft_set_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
    gpio_set_level(TFT_DC_GPIO, 1);
    const size_t chunk = 1024;
    uint8_t buf[chunk];
    for (size_t i = 0; i < chunk; i += 2) { buf[i] = hi; buf[i + 1] = lo; }  /* high byte first */
    size_t total = (size_t)TFT_WIDTH * TFT_HEIGHT * 2;
    for (size_t sent = 0; sent < total; sent += chunk) {
        size_t n = (total - sent) < chunk ? (total - sent) : chunk;
        spi_transaction_t t = { .length = n * 8, .tx_buffer = buf };
        spi_device_polling_transmit(tft_spi_handle, &t);
    }
}

/* Fill rectangle (x,y) size (w,h) with RGB565 color. */
static void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0) return;
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    tft_set_window(x, y, x + w - 1, y + h - 1);
    gpio_set_level(TFT_DC_GPIO, 1);
    const size_t chunk = 512;
    uint8_t buf[chunk];
    for (size_t i = 0; i < chunk; i += 2) { buf[i] = hi; buf[i + 1] = lo; }
    size_t total = (size_t)w * h * 2;
    for (size_t sent = 0; sent < total; sent += chunk) {
        size_t n = (total - sent) < chunk ? (total - sent) : chunk;
        spi_transaction_t t = { .length = n * 8, .tx_buffer = buf };
        spi_device_polling_transmit(tft_spi_handle, &t);
    }
}

/* Draw one 5x7 character at (x,y) with scale factor, fg in RGB565. */
static void tft_draw_char_scaled(uint16_t x, uint16_t y, char c, uint16_t fg, int scale)
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
    /* Each font pixel → scale×scale pixel block on LCD */
    for (int cx = 0; cx < FONT5X7_W; cx++) {
        for (int row = 0; row < FONT5X7_H; row++) {
            if ((col[cx] >> row) & 1) {
                uint16_t px0 = x + cx * scale;
                uint16_t py0 = y + row * scale;
                tft_set_window(px0, py0, px0 + scale - 1, py0 + scale - 1);
                gpio_set_level(TFT_DC_GPIO, 1);
                uint8_t buf[scale * scale * 2];
                for (int i = 0; i < scale * scale; i++) {
                    buf[i * 2]     = fg_hi;
                    buf[i * 2 + 1] = fg_lo;
                }
                spi_transaction_t t = { .length = scale * scale * 16, .tx_buffer = buf };
                spi_device_polling_transmit(tft_spi_handle, &t);
            }
        }
    }
}

/* Draw string at (x,y) with scale, return pixel width. */
static int tft_draw_string_scaled(uint16_t x, uint16_t y, const char *s, uint16_t fg, int scale)
{
    int cx = 0;
    int stride = FONT5X7_STRIDE * scale;
    while (*s) {
        tft_draw_char_scaled(x + cx, y, *s, fg, scale);
        cx += stride;
        s++;
    }
    return cx;
}

/* String pixel width with scale (for centering). */
static int tft_string_width_scaled(const char *s, int scale)
{
    return (int)strlen(s) * FONT5X7_STRIDE * scale;
}

/* Backward compat wrappers (scale=1) */
static int tft_draw_string_5x7(uint16_t x, uint16_t y, const char *s, uint16_t fg)
{ return tft_draw_string_scaled(x, y, s, fg, 1); }
static int tft_string_width_px(const char *s)
{ return tft_string_width_scaled(s, 1); }

/* If display is noisy/wrong colors, try: (1) Swap INVON <-> INVOFF  (2) Try madctl: 0xC8, 0xA8, 0x68, 0xC0 */
static void tft_display_init(void)
{
    tft_hw_reset();

    tft_write_cmd(ST77XX_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(200));

    tft_write_cmd(ST77XX_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(255));

    /* ST7735R-style init for 1.77" 160x128 - frame rate, power, inversion */
    uint8_t d3[] = { 0x01, 0x2C, 0x2D };
    tft_write_cmd(ST7735_FRMCTR1);
    tft_write_data(d3, 3);
    tft_write_cmd(ST7735_FRMCTR2);
    tft_write_data(d3, 3);
    uint8_t d6[] = { 0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D };
    tft_write_cmd(ST7735_FRMCTR3);
    tft_write_data(d6, 6);
    uint8_t inv = 0x07;  /* no inversion */
    tft_write_cmd(ST7735_INVCTR);
    tft_write_data(&inv, 1);
    uint8_t pw1[] = { 0xA2, 0x02, 0x84 };
    tft_write_cmd(ST7735_PWCTR1);
    tft_write_data(pw1, 3);
    uint8_t pw2 = 0xC5;
    tft_write_cmd(ST7735_PWCTR2);
    tft_write_data(&pw2, 1);
    uint8_t pw3[] = { 0x0A, 0x00 };
    tft_write_cmd(ST7735_PWCTR3);
    tft_write_data(pw3, 2);
    uint8_t pw4[] = { 0x8A, 0x2A };
    tft_write_cmd(ST7735_PWCTR4);
    tft_write_data(pw4, 2);
    uint8_t pw5[] = { 0x8A, 0xEE };
    tft_write_cmd(ST7735_PWCTR5);
    tft_write_data(pw5, 2);
    uint8_t vm = 0x0E;
    tft_write_cmd(ST7735_VMCTR1);
    tft_write_data(&vm, 1);

    tft_write_cmd(ST77XX_INVOFF);  /* INITR_GREENTAB: no invert */
    /* MADCTL 0xA8 = setRotation(1) green tab: MY|MV|BGR (same as sample ESP32TFT_128x160B) */
    uint8_t madctl = 0xA8;
    tft_write_cmd(ST77XX_MADCTL);
    tft_write_data(&madctl, 1);
    uint8_t colmod = 0x05;          /* 16-bit RGB565 */
    tft_write_cmd(ST77XX_COLMOD);
    tft_write_data(&colmod, 1);

    /* Gamma (optional but helps stability/colors) */
    uint8_t gmp[] = { 0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10 };
    tft_write_cmd(ST7735_GMCTRP1);
    tft_write_data(gmp, 16);
    uint8_t gmn[] = { 0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10 };
    tft_write_cmd(ST7735_GMCTRN1);
    tft_write_data(gmn, 16);

    tft_write_cmd(ST77XX_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));
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
        .max_transfer_sz = TFT_WIDTH * TFT_HEIGHT * 2 + 8,
    };
    err = spi_bus_initialize(TFT_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) return err;

    spi_device_interface_config_t dev_cfg = {
        .mode = 0,                                   /* SPI mode 0 (CPOL=0, CPHA=0) */
        .clock_speed_hz = TFT_SPI_CLOCK_HZ,
        .spics_io_num = TFT_CS_GPIO,
        .flags = SPI_DEVICE_NO_DUMMY,                /* same as TFT_eSPI; no dummy cycle */
        .queue_size = 1,
    };
    esp_err_t ret = spi_bus_add_device(TFT_SPI_HOST, &dev_cfg, &tft_spi_handle);
    ESP_LOGI("SPI", "bus_init OK, add_device ret=%s, host=%d, cs=%d, clk=%d",
             esp_err_to_name(ret), (int)TFT_SPI_HOST, TFT_CS_GPIO, TFT_SPI_CLOCK_HZ);
    return ret;
}

/* TEST_PASS: 1 = pass build (UART "Hello RKTech", LCD "Welcome RKTech"), 0 = fail build */
#ifndef TEST_PASS
#define TEST_PASS 1
#endif

/* ================================================================
 * WiFi / SmartConfig (ESP-Touch) / NTP
 * ================================================================ */

#define WIFI_NVS_NS           "wifi"
#define WIFI_NVS_SSID_KEY     "ssid"
#define WIFI_NVS_PASS_KEY     "pass"
#define WIFI_CONN_TIMEOUT_MS  15000   /* 15 s – connect with saved credentials */
#define SC_TIMEOUT_MS         90000   /* 90 s – SmartConfig (ESP-Touch) window  */
#define NTP_TIMEOUT_MS        10000   /* 10 s – wait for first NTP sync         */
#define NTP_SERVER            "pool.ntp.org"
/* POSIX TZ: offset sign is opposite of common notation.
 * "ICT-7" means local = UTC + 7 h  (Vietnam / Indochina Time). */
#define TZ_VIETNAM            "ICT-7"

#define EVT_WIFI_OK   BIT0
#define EVT_SC_GOT    BIT1
#define EVT_NTP_OK    BIT2

static EventGroupHandle_t s_evt_grp;
static char s_pending_ssid[32];
static char s_pending_pass[64];

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(s_evt_grp, EVT_WIFI_OK);
    } else if (base == SC_EVENT) {
        if (id == SC_EVENT_GOT_SSID_PSWD) {
            smartconfig_event_got_ssid_pswd_t *e =
                (smartconfig_event_got_ssid_pswd_t *)data;
            memcpy(s_pending_ssid, e->ssid,     sizeof(s_pending_ssid));
            memcpy(s_pending_pass, e->password, sizeof(s_pending_pass));
            xEventGroupSetBits(s_evt_grp, EVT_SC_GOT);
        } else if (id == SC_EVENT_SEND_ACK_DONE) {
            esp_smartconfig_stop();
        }
    }
}

static void ntp_sync_cb(struct timeval *tv)
{
    if (s_evt_grp) {
        xEventGroupSetBits(s_evt_grp, EVT_NTP_OK);
    }
}

static void nvs_save_wifi(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    if (nvs_open(WIFI_NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, WIFI_NVS_SSID_KEY, ssid);
        nvs_set_str(h, WIFI_NVS_PASS_KEY, pass);
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGI(TAG, "WiFi credentials saved to NVS (SSID: %s)", ssid);
    }
}

static bool nvs_load_wifi(char ssid[32], char pass[64])
{
    nvs_handle_t h;
    if (nvs_open(WIFI_NVS_NS, NVS_READONLY, &h) != ESP_OK) return false;
    size_t sl = 32, pl = 64;
    bool ok = (nvs_get_str(h, WIFI_NVS_SSID_KEY, ssid, &sl) == ESP_OK &&
               nvs_get_str(h, WIFI_NVS_PASS_KEY, pass, &pl) == ESP_OK &&
               strlen(ssid) > 0);
    nvs_close(h);
    return ok;
}

/* Show a one-line status message at the top of the LCD (white text). */
static void lcd_show_wifi_status(const char *msg)
{
    tft_fill_rect(0, 2, TFT_WIDTH, FONT5X7_H + 2, 0x0000);
    int w = tft_string_width_px(msg);
    tft_draw_string_5x7((TFT_WIDTH - w) / 2, 3, msg, 0xFFFF);
}

/*
 * Full provisioning flow:
 *   1. Try saved credentials from NVS → connect directly.
 *   2. On failure (or no saved creds) → show SmartConfig/ESP-Touch screen.
 *   3. When WiFi is up → sync NTP, set timezone to Vietnam (UTC+7).
 * Returns true  → NTP synced (real internet time).
 * Returns false → timeout / failure, caller should use RTC default time.
 */
static bool wifi_provision_and_sync(void)
{
    s_evt_grp = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&init_cfg);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID,    wifi_event_handler, NULL);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    bool wifi_ok = false;
    char ssid[32] = {0}, pass[64] = {0};

    if (nvs_load_wifi(ssid, pass)) {
        ESP_LOGI(TAG, "Trying saved WiFi SSID: %s", ssid);
        lcd_show_wifi_status("Connecting WiFi...");

        wifi_config_t wcfg = {0};
        strlcpy((char *)wcfg.sta.ssid,     ssid, sizeof(wcfg.sta.ssid));
        strlcpy((char *)wcfg.sta.password, pass, sizeof(wcfg.sta.password));
        esp_wifi_set_config(WIFI_IF_STA, &wcfg);
        esp_wifi_connect();

        EventBits_t b = xEventGroupWaitBits(s_evt_grp, EVT_WIFI_OK,
                                             pdFALSE, pdFALSE,
                                             pdMS_TO_TICKS(WIFI_CONN_TIMEOUT_MS));
        wifi_ok = (b & EVT_WIFI_OK) != 0;
        if (!wifi_ok) {
            ESP_LOGW(TAG, "Saved WiFi failed, launching SmartConfig...");
            esp_wifi_disconnect();
        }
    }

    if (!wifi_ok) {
        /* SmartConfig / ESP-Touch instruction screen */
        tft_fill_screen_rgb565(0x0000);
        const char *t0 = "WiFi Setup";
        tft_draw_string_5x7((TFT_WIDTH - tft_string_width_px(t0)) / 2, 12, t0, 0xFFE0);
        const char *t1 = "1. Install ESP-Touch";
        tft_draw_string_5x7(2, 28, t1, 0xFFFF);
        const char *t2 = "2. Open app on phone";
        tft_draw_string_5x7(2, 40, t2, 0xFFFF);
        const char *t3 = "3. Enter SSID/Pass";
        tft_draw_string_5x7(2, 52, t3, 0xFFFF);
        const char *t4 = "4. Tap Connect";
        tft_draw_string_5x7(2, 64, t4, 0xFFFF);
        lcd_show_wifi_status("Waiting ESP-Touch...");

        ESP_LOGI(TAG, "SmartConfig (ESP-Touch) started, waiting...");
        esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
        smartconfig_start_config_t sc_cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
        esp_smartconfig_start(&sc_cfg);

        EventBits_t sc_bits = xEventGroupWaitBits(s_evt_grp, EVT_SC_GOT,
                                                   pdFALSE, pdFALSE,
                                                   pdMS_TO_TICKS(SC_TIMEOUT_MS));
        if (!(sc_bits & EVT_SC_GOT)) {
            ESP_LOGW(TAG, "SmartConfig timeout — using RTC default time");
            esp_smartconfig_stop();
            vEventGroupDelete(s_evt_grp);
            s_evt_grp = NULL;
            return false;
        }

        /* Got credentials from phone — now connect */
        xEventGroupClearBits(s_evt_grp, EVT_WIFI_OK);
        esp_wifi_disconnect();
        wifi_config_t wcfg = {0};
        strlcpy((char *)wcfg.sta.ssid,     s_pending_ssid, sizeof(wcfg.sta.ssid));
        strlcpy((char *)wcfg.sta.password, s_pending_pass, sizeof(wcfg.sta.password));
        esp_wifi_set_config(WIFI_IF_STA, &wcfg);
        esp_wifi_connect();
        lcd_show_wifi_status("Connecting WiFi...");

        EventBits_t c = xEventGroupWaitBits(s_evt_grp, EVT_WIFI_OK,
                                             pdFALSE, pdFALSE,
                                             pdMS_TO_TICKS(WIFI_CONN_TIMEOUT_MS));
        if (!(c & EVT_WIFI_OK)) {
            ESP_LOGW(TAG, "WiFi connect failed after SmartConfig");
            vEventGroupDelete(s_evt_grp);
            s_evt_grp = NULL;
            return false;
        }
        nvs_save_wifi(s_pending_ssid, s_pending_pass);
    }

    /* WiFi connected — sync NTP */
    lcd_show_wifi_status("Syncing NTP...");
    ESP_LOGI(TAG, "WiFi connected. Syncing NTP via %s...", NTP_SERVER);

    setenv("TZ", TZ_VIETNAM, 1);
    tzset();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, (char *)NTP_SERVER);
    esp_sntp_set_time_sync_notification_cb(ntp_sync_cb);
    esp_sntp_init();

    EventBits_t ntp_bits = xEventGroupWaitBits(s_evt_grp, EVT_NTP_OK,
                                                pdFALSE, pdFALSE,
                                                pdMS_TO_TICKS(NTP_TIMEOUT_MS));
    bool ntp_ok = (ntp_bits & EVT_NTP_OK) != 0;
    if (ntp_ok) {
        ESP_LOGI(TAG, "NTP synced (Vietnam UTC+7)");
    } else {
        ESP_LOGW(TAG, "NTP sync timeout — using RTC default time");
    }

    vEventGroupDelete(s_evt_grp);
    s_evt_grp = NULL;
    return ntp_ok;
}

/* Fallback: set a fixed starting time when no NTP is available.
 * The internal FreeRTOS tick counter keeps counting from here. */
static void rtc_use_default(void)
{
    setenv("TZ", TZ_VIETNAM, 1);
    tzset();
    struct tm t = {0};
    t.tm_year = 2025 - 1900;
    t.tm_mon  = 2 - 1;   /* February */
    t.tm_mday = 12;
    time_t sec = mktime(&t);
    if (sec != (time_t)-1) {
        struct timeval tv = { .tv_sec = sec, .tv_usec = 0 };
        settimeofday(&tv, NULL);
    }
    ESP_LOGI(TAG, "RTC default set: 2025-02-12 00:00:00 (UTC+7)");
}

void app_main(void)
{
#if (TEST_PASS == 1)
    ESP_LOGI(TAG, "Hello RKTech");
#else
    ESP_LOGI(TAG, "Hello PicoPiece");
#endif
    ESP_LOGI(TAG, "ATS ESP32 Firmware Demo");
    ESP_LOGI(TAG, "Build successful!");

    /* NVS must be initialized before WiFi (WiFi uses NVS internally). */
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_err_t err = tft_spi_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "TFT SPI init OK (SCK=%d, SDA/MOSI=%d, CS=%d, Res=%d, Rs=%d)",
                 TFT_SCK_GPIO, TFT_MOSI_GPIO, TFT_CS_GPIO, TFT_RST_GPIO, TFT_DC_GPIO);
        tft_display_init();

        /* Step 1: color test – red → green → blue, 1 s each */
        tft_fill_screen_rgb565(0xF800);
        ESP_LOGI(TAG, "LCD Red");
        vTaskDelay(pdMS_TO_TICKS(1000));

        tft_fill_screen_rgb565(0x07E0);
        ESP_LOGI(TAG, "LCD Green");
        vTaskDelay(pdMS_TO_TICKS(1000));

        tft_fill_screen_rgb565(0x001F);
        ESP_LOGI(TAG, "LCD Blue");
        vTaskDelay(pdMS_TO_TICKS(1000));

        /* Step 2: welcome text (shown for 2 s) */
        tft_fill_screen_rgb565(0x0000);

#if (TEST_PASS == 1)
        const char *line1 = "Welcome RKTech";
#else
        const char *line1 = "Welcome PicoPiece";
#endif
        int w1 = tft_string_width_px(line1);
        int x1 = (TFT_WIDTH - w1) / 2;
        int y1 = TFT_HEIGHT / 2 - FONT5X7_H - 5;
        tft_draw_string_5x7(x1, y1, line1, 0xFFFF);

        const char *line2 = "ATS_Demo";
        int w2 = tft_string_width_px(line2);
        int x2 = (TFT_WIDTH - w2) / 2;
        int y2 = TFT_HEIGHT / 2 - 1;
        tft_draw_string_5x7(x2, y2, line2, 0xFFE0);

        const char *line3 = "ESP32_IoT_LCD";
        int w3 = tft_string_width_px(line3);
        int x3 = (TFT_WIDTH - w3) / 2;
        int y3 = TFT_HEIGHT / 2 + FONT5X7_H + 2;
        tft_draw_string_5x7(x3, y3, line3, 0x07FF);

        ESP_LOGI(TAG, "Text displayed on LCD");
        vTaskDelay(pdMS_TO_TICKS(2000));

        /* Step 3: WiFi provisioning (SmartConfig / ESP-Touch) + NTP sync */
        bool ntp_synced = wifi_provision_and_sync();
        if (!ntp_synced) {
            rtc_use_default();   /* fallback: fixed start time */
        }

        /* Step 4: real-time clock display, updated every second */
        const int clock_scale = 2;
        const int title_y     = 10;
        const int date_y      = 48;
        const int time_y      = 70;
        const int line_h      = FONT5X7_H * clock_scale + 2;

        tft_fill_screen_rgb565(0x0000);
        const char *clk_title = "Real-time clock";
        int ctw = tft_string_width_px(clk_title);
        tft_draw_string_5x7((TFT_WIDTH - ctw) / 2, title_y, clk_title, 0x07FF);

        /* Source indicator: "NTP" (green) or "RTC" (yellow) in top-right corner */
        const char *src_label = ntp_synced ? "NTP" : "RTC";
        uint16_t    src_color = ntp_synced ? 0x07E0 : 0xFFE0;
        int src_w = tft_string_width_px(src_label);
        tft_draw_string_5x7(TFT_WIDTH - src_w - 2, title_y, src_label, src_color);

        char date_buf[32];
        char time_buf[32];
        while (1) {
            time_t now = time(NULL);
            struct tm tm_info;
            if (localtime_r(&now, &tm_info) != NULL) {
                snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d",
                         tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday);
                snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d",
                         tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec);
                int dw  = tft_string_width_scaled(date_buf, clock_scale);
                int xd  = (TFT_WIDTH - dw) / 2;
                int tw2 = tft_string_width_scaled(time_buf, clock_scale);
                int xt  = (TFT_WIDTH - tw2) / 2;
                tft_fill_rect(0, date_y - 2, TFT_WIDTH, line_h * 2 + 4, 0x0000);
                tft_draw_string_scaled(xd, date_y, date_buf, 0xFFFF, clock_scale);
                tft_draw_string_scaled(xt, time_y, time_buf, 0xFFE0, clock_scale);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    } else {
        ESP_LOGW(TAG, "TFT SPI init failed: %s", esp_err_to_name(err));
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI(TAG, "Running...");
        }
    }
}

