# Khôi phục kernel Pi và xử lý USB không hoạt động

## A. Khôi phục kernel sau rpi-update

Nếu sau `rpi-update` **USB hoàn toàn không work** (lsusb trống, exit 1), cần cài lại kernel/firmware ổn định từ repo.

## Trên Pi (SSH hoặc console)

Chạy lần lượt:

```bash
sudo apt update
sudo apt install --reinstall libraspberrypi0 libraspberrypi-bin libraspberrypi-dev libraspberrypi-doc raspberrypi-bootloader raspberrypi-kernel raspberrypi-kernel-headers
sudo reboot
```

Sau reboot: `uname -r` sẽ về **6.1.21-v8+** (hoặc bản 6.1.x tương ứng). Kiểm tra lại:

```bash
lsusb
ls /dev/ttyUSB*
```

---

## Nếu Pi không boot được (màn hình đen / kẹt)

1. Rút thẻ SD, cắm vào máy Linux/Mac.
2. Mount partition **boot** (FAT32, thường partition 1).
3. Trong partition boot, đổi tên hoặc xóa file kernel để buộc dùng kernel cũ:
   - Đổi `kernel8.img` → `kernel8.img.bak`
   - Đổi `kernel8.img.old` (backup do rpi-update tạo) → `kernel8.img`
4. Cắm SD lại vào Pi, boot. Vào được hệ thống thì chạy lệnh `apt install --reinstall` ở trên, rồi `reboot`.

---

## B. xHCI “can't setup: -110” ngay khi boot (lsusb vẫn trống)

Lỗi trong dmesg: `xhci_hcd 0000:01:00.0: can't setup: -110` → chip VL805 (USB 3.0 trên Pi 4) không kịp phản hồi khi kernel khởi tạo. **Không phải do kernel 6.12** – có thể do nguồn hoặc board.

### B1. Kiểm tra

```bash
dmesg | grep -i xhci
vcgencmd get_throttled   # 0x0 = OK, khác 0 = từng bị undervoltage
```

### B2. Thử trên Pi

1. **Boot không cắm USB:** Rút hết thiết bị USB (kể cả ESP32), reboot. Sau khi Pi vào OS (lsusb đã có root hub), **mới cắm ESP32**. Đã xác nhận: tháo ESP ra reboot → USB work; cắm lại sau → dùng bình thường.
2. **Nguồn:** Dùng adapter **5V 3A chính hãng** (hoặc official Pi PSU).
3. **Boot delay:** Trên partition **boot** (FAT32), sửa `/boot/config.txt`, trong section `[pi4]` thêm `boot_delay=1`, lưu, reboot (delay 1 giây trước boot để 5V ổn định).
4. **Rút nguồn hoàn toàn:** Tắt Pi, rút dây nguồn 30 giây, cắm lại rồi bật (đôi khi xHCI lên sau lần boot thứ 2).

### B3. Nếu đã thử boot_delay=1 + reboot mà vẫn -110

Phần mềm đã tối ưu (kernel 6.1, boot_delay, EEPROM up to date). Lỗi -110 lặp lại thường do **phần cứng**:

| Hướng xử lý | Gợi ý |
|-------------|--------|
| **Nguồn** | Đổi sang **adapter 5V 3A chính hãng** (hoặc official Pi PSU), cắm ổn định; thử ổ cắm khác, không dùng hub USB cấp nguồn cho Pi. |
| **Board** | Pi 4 Rev 1.5 dùng VL805 trên PCIe; nếu đổi nguồn vẫn -110 → khả năng **board lỗi**. Dùng **Pi 4 khác** làm ATS agent. |
| **Test ESP32** | Chạy flash/test ESP32 trên **máy PC** (USB ổn), sau đó đồng bộ artifact/log về CI. |

Không dùng `rpi-update` trên board này (kernel 6.12 làm USB mất hẳn). Giữ kernel từ `apt`.
