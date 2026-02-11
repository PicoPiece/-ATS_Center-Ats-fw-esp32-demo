# Khắc phục USB không ổn định (CP210x -32 / Errno 5) trên Pi

Mục tiêu: **giải quyết dứt điểm** tại sao USB serial (CP210x) không ổn định khi dùng với Raspberry Pi làm ATS agent, không dựa vào workaround trong code.

---

## 1. Nguyên nhân thường gặp

- **ModemManager** probe các cổng serial → mở/đóng device → CP210x có thể vào trạng thái lỗi (-32, “Unable to enable UART”).
- **Nguồn Pi** yếu / sụt áp → USB không ổn định, device không trả lời control request đúng.
- **Controller/hub (VIA trên Pi)**: sau lỗi, port có thể “dính” trạng thái lỗi đến khi reboot.
- **Cáp / cổng**: cáp kém, tiếp xúc kém → control transfer fail (-32).

---

## 2. Các bước khắc phục (làm trên Pi – ATS agent)

### Bước 1: Blacklist CP210x khỏi ModemManager (bắt buộc)

Để ModemManager **không bao giờ** mở/touch CP210x (ESP32):

```bash
sudo tee /etc/udev/rules.d/99-mm-ignore-cp210x-esp32.rules << 'EOF'
# ModemManager: bỏ qua CP210x (ESP32) để tránh probe gây lỗi -32
ACTION=="add", SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", ENV{ID_MM_DEVICE_IGNORE}="1"
EOF
sudo udevadm control --reload-rules
```

Rồi **rút cắm lại** ESP32 (hoặc reboot).

---

### Bước 2: Kiểm tra nguồn Pi

- Dùng **adapter 5V chính hãng** cho Pi (đủ dòng theo model, ví dụ 5V/2.5A cho Pi 4).
- Kiểm tra **undervoltage**:
  ```bash
  vcgencmd get_throttled
  ```
  Nếu khác `0x0` thì có lúc Pi bị sụt nguồn.
- Hạn chế cắm nhiều thiết bị USB cùng lúc (camera, ổ cứng, v.v.) nếu không cần.

---

### Bước 3: Không cho user/process khác mở serial khi không cần

- Trên agent chỉ chạy ATS test (Docker); không mở serial monitor / script khác trỏ vào `/dev/ttyUSB0` khi đang chạy pipeline.
- Nếu có cron/script nào dùng serial thì tắt hoặc đổi sang port khác.

---

### Bước 4: (Tùy chọn) Tắt hẳn ModemManager trên agent

Nếu Pi **không dùng modem/4G**:

```bash
sudo systemctl stop ModemManager
sudo systemctl disable ModemManager
```

Sau đó reboot và test lại. Chỉ làm nếu bạn chắc không cần modem.

---

### Bước 5: Sau khi áp dụng

- **Reboot Pi** một lần.
- Cắm ESP32, chạy test Jenkins.
- Nếu vẫn lỗi: kiểm tra lại udev (`ls -la /etc/udev/rules.d/99-mm-ignore*`), `get_throttled`, và dmesg khi mở port.

---

## 3. Tóm tắt

| Việc | Mục đích |
|------|----------|
| udev `ID_MM_DEVICE_IGNORE` cho 10c4:ea60 | ModemManager không đụng CP210x → giảm -32 do probe |
| Nguồn đủ, kiểm tra throttled | USB ổn định, không sụt áp |
| Không mở ttyUSB0 ngoài ATS | Tránh conflict, tránh device bị đưa vào trạng thái lỗi |
| (Tùy chọn) disable ModemManager | Loại bỏ hoàn toàn tác nhân từ ModemManager |

Áp dụng **Bước 1** là quan trọng nhất; kết hợp Bước 2–4 để USB ổn định lâu dài.
