# ESP32 firmware test – Yêu cầu

## Test case / artifact

- **Thay thế:** Test OLED LCD → **Test TFT LCD** (TFT LCD I2C 160×128).
- Test plan và artifact test dùng: **tft_lcd_test** thay cho **oled_display_test**.

## Điều kiện Pass/Fail (UART log)

- **PASS:** Trong UART log (boot_messages.log / uart_boot.log) **có chuỗi** `Hello RKTech`.
- **FAIL:** Không có log, hoặc log khác (ví dụ không có "Hello RKTech", hoặc chỉ có "Hello Picopiece" cho bản cũ).

## Firmware version string (ví dụ)

- **v1.0:** log in "Hello Picopiece".
- **v1.1:** log in "Hello RKTech" (bản dùng cho pass theo yêu cầu trên).

## TFT LCD 160×128 (SPI)

- **Kích thước:** 1.77" 160(RGB)×128.
- **Giao tiếp:** SPI.
- **Pin ESP32 (phần cứng hiện tại):**

  | Tên trên module | GPIO | Ghi chú   |
  |-----------------|------|-----------|
  | SCK             | 18   | SPI clock |
  | SDA             | 23   | SPI MOSI  |
  | Res             | 15   | Reset     |
  | Rs              | 32   | DC/RS     |
  | CS              | 4    | Chip select |
  | VCC, GND        | 3.3V, GND | Nguồn |

  Firmware khởi tạo SPI với các pin trên.

## Tóm tắt

| Nội dung | Giá trị |
|----------|--------|
| Test case | TFT LCD (thay OLED) |
| Pass | Log có **"Hello RKTech"** |
| Fail | Không có log hoặc log khác |
| TFT interface | SPI: SCK=18, SDA/MOSI=23, CS=4, Res=15, Rs=32 |
