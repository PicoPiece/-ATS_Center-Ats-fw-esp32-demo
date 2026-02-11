# Khắc phục USB không ổn định (CP210x -32 / Errno 5) trên Pi

Mục tiêu: **giải quyết dứt điểm** tại sao USB serial (CP210x) không ổn định khi dùng với Raspberry Pi làm ATS agent, không dựa vào workaround trong code.

---

## 1. Nguyên nhân thường gặp

- **Dòng USB (line) gây impact**: cáp USB kém chất lượng, cổng USB lỏng/oxi hóa, hoặc tải trên bus USB (nhiều thiết bị) làm sụt áp / nhiễu → VL805/xHCI timeout (-110) hoặc CP210x -32. **Đổi cáp tốt, cổng khác, hoặc hub có nguồn** thường cải thiện rõ.
- **ModemManager** probe các cổng serial → mở/đóng device → CP210x có thể vào trạng thái lỗi (-32, “Unable to enable UART”).
- **Nguồn Pi** yếu / sụt áp → USB không ổn định, device không trả lời control request đúng.
- **Controller/hub (VIA trên Pi)**: sau lỗi, port có thể “dính” trạng thái lỗi đến khi reboot.
- **Cáp / cổng**: cáp kém, tiếp xúc kém → control transfer fail (-32).

---

## 2. Các bước khắc phục (làm trên Pi – ATS agent)

### Bước 1: Blacklist CP210x khỏi ModemManager (bắt buộc)

Dùng rule trong `docs/udev/99-mm-ignore-cp210x-esp32.rules` (copy vào `/etc/udev/rules.d/`), rồi `sudo udevadm control --reload-rules`.

**Không** ghi udev vào `power/autosuspend` của USB device — có thể gây re-enumerate → auto disconnect. Rule CP210x chỉ set quyền tty (MODE, GROUP).

Nếu vẫn bị **cắm → detect → auto disconnect**: tắt ModemManager thử: `sudo systemctl stop ModemManager`, cắm lại ESP32, xem có còn tự ngắt không.

---

### Bước 2: Giảm impact từ dòng USB (cáp / cổng)

- **Boot không cắm ESP32**: Nếu Pi bị xHCI -110 khi boot (lsusb trống), **rút hết thiết bị USB** (kể cả ESP32), reboot, đợi vào OS xong rồi **mới cắm ESP32**. Đã xác nhận: boot không cắm ESP → USB work; cắm lại sau khi boot → dùng bình thường cho ATS.
- **Cắm ESP sau boot nhưng vẫn không auto-detect (không có /dev/ttyUSB0)**: Kiểm tra `dmesg` ngay sau khi cắm. Nếu thấy `device descriptor read/64, error -32` hoặc `unable to enumerate USB device` → CP210x **không enumerate được** trên cổng đó. Làm lần lượt:
  1. **Đổi cổng USB trên Pi**: Trong dmesg, `usb 1-1.1` = cổng USB **thứ nhất** (trái), `1-1.2` = cổng 2, `1-1.3` = 3, `1-1.4` = 4. Rút ESP → cắm vào **cổng khác** (vd từ 1-1.1 sang 1-1.2 hoặc 1-1.3) → chạy `dmesg | tail -15` và `ls /dev/ttyUSB*`. Nếu thấy `cp210x ... detected` và có `/dev/ttyUSB0` là OK.
  2. **Đổi cáp**: Dùng cáp USB **data** (không chỉ sạc), ngắn, chắc. Thử lại một cổng đã thử.
  3. **Cáp khác**: Nếu **đổi cổng vẫn -32/-71** (vd 1-1.1 và 1-1.3 đều `unable to enumerate`) → nhiều khả năng **cáp** hoặc **tương thích Pi–ESP**. Thử **cáp USB data khác** (ngắn, chắc, cáp đã dùng tốt trên PC). Sau đó thử lại một cổng.
  4. **Hub có nguồn**: Nếu đổi cáp vẫn lỗi, thử **USB hub tự cấp nguồn** (powered hub), cắm ESP vào hub rồi hub vào Pi.
  5. **Cả 4 cổng đều -32/-71**: Đã thử 1-1.1, 1-1.3, 1-1.4 (và 1-1.2) mà vẫn `unable to enumerate` → **cáp** hoặc **Pi này không ổn định với CP210x**. Thử cáp data khác (đã dùng tốt trên PC); nếu vẫn không được thì chạy flash/test ESP32 trên **máy PC** hoặc **Pi 4 khác**, rồi dùng host đó làm ATS agent hoặc đồng bộ kết quả.
- **Cáp**: Dùng cáp USB **data** (không phải cáp chỉ sạc), ngắn, chất lượng tốt. Tránh cáp rẻ, dài, tiếp xúc kém.
- **Cổng**: Tránh cổng lỏng hoặc bụi/oxi hóa.
- **Hub có nguồn**: Nếu cắm nhiều thiết bị, dùng **USB hub tự cấp nguồn** (powered hub) để không kéo dòng lớn từ Pi → giảm sụt áp và -110 / -32.

### Bước 3: Kiểm tra nguồn Pi

- Dùng **adapter 5V chính hãng** cho Pi (đủ dòng theo model, ví dụ 5V/2.5A cho Pi 4).
- Kiểm tra **undervoltage**:
  ```bash
  vcgencmd get_throttled
  ```
  Nếu khác `0x0` thì có lúc Pi bị sụt nguồn.
- Hạn chế cắm nhiều thiết bị USB cùng lúc (camera, ổ cứng, v.v.) nếu không cần.

---

### Bước 4: Không cho user/process khác mở serial khi không cần

- Trên agent chỉ chạy ATS test (Docker); không mở serial monitor / script khác trỏ vào `/dev/ttyUSB0` khi đang chạy pipeline.
- Nếu có cron/script nào dùng serial thì tắt hoặc đổi sang port khác.

---

### Bước 5: (Tùy chọn) Tắt hẳn ModemManager trên agent

Nếu Pi **không dùng modem/4G**:

```bash
sudo systemctl stop ModemManager
sudo systemctl disable ModemManager
```

Sau đó reboot và test lại. Chỉ làm nếu bạn chắc không cần modem.

---

### Bước 6: (Tùy chọn) Cập nhật kernel Pi

Nếu auto-detect / đổi cổng vẫn không ổn định, có thể do kernel cũ (ví dụ 6.1.x). Cập nhật kernel và firmware lên bản mới (rpi-6.12.y):

```bash
# Cập nhật qua apt (an toàn)
sudo apt update && sudo apt upgrade -y

# Cập nhật lên kernel/firmware mới nhất (rpi-update)
sudo -E SKIP_WARNING=1 rpi-update
sudo reboot
```

Sau reboot: `uname -r` sẽ là 6.12.x. Nếu Pi không boot được sau rpi-update, khôi phục bản ổn định:

```bash
sudo apt update
sudo apt install --reinstall libraspberrypi0 libraspberrypi-{bin,dev,doc} raspberrypi-bootloader raspberrypi-kernel
sudo reboot
```

### Bước 7: Sau khi áp dụng

- **Reboot Pi** một lần (bắt buộc sau rpi-update).
- Cắm ESP32, kiểm tra `ls /dev/ttyUSB*`, rồi chạy test Jenkins.
- Nếu vẫn lỗi: kiểm tra lại udev (`ls -la /etc/udev/rules.d/99-mm-ignore*`), `get_throttled`, và `dmesg` khi mở port.

---

## 3. Tóm tắt

| Việc | Mục đích |
|------|----------|
| **Cáp/cổng USB tốt, hub có nguồn** | Giảm impact dòng USB → ít sụt áp, nhiễu; giảm -110 / -32 |
| udev `ID_MM_DEVICE_IGNORE` cho 10c4:ea60 | ModemManager không đụng CP210x → giảm -32 do probe |
| Nguồn đủ, kiểm tra throttled | USB ổn định, không sụt áp |
| Không mở ttyUSB0 ngoài ATS | Tránh conflict, tránh device bị đưa vào trạng thái lỗi |
| (Tùy chọn) disable ModemManager | Loại bỏ hoàn toàn tác nhân từ ModemManager |

Áp dụng **Bước 1** là quan trọng nhất; kết hợp Bước 2–4 để USB ổn định lâu dài.
