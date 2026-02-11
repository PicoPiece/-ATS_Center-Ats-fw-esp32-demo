#!/bin/bash
# Chạy trên Pi: ./pi-boot-delay.sh   (hoặc bash pi-boot-delay.sh)
# Thêm boot_delay=1 vào /boot/config.txt để 5V ổn định trước xHCI init (-110)

CONFIG=/boot/config.txt
if grep -q "boot_delay=1" "$CONFIG"; then
  echo "boot_delay=1 already in $CONFIG"
  exit 0
fi
echo "" | sudo tee -a "$CONFIG"
echo "# Delay 1s before boot (may help xHCI VL805 init -110)" | sudo tee -a "$CONFIG"
echo "boot_delay=1" | sudo tee -a "$CONFIG"
echo "Added boot_delay=1 to $CONFIG. Reboot to apply."
