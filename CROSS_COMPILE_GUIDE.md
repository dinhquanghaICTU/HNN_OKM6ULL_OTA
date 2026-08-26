# HƯỚNG DẪN CÀI ĐẶT MÔI TRƯỜNG BIÊN DỊCH CHÉO & DEPLOY APP LÊN BO MẠCH OKM6ULL-S

Tài liệu này hướng dẫn chi tiết quy trình thiết lập môi trường biên dịch chéo (Cross-Compilation Environment) từ máy tính phát triển (Ubuntu / Windows WSL) sang bo mạch nhúng **Forlinx OKM6ULL-S (NXP i.MX6ULL - Cortex-A7)** mà **KHÔNG CẦN CÀI ĐẶT HỆ THỐNG YOCTO NẶNG NỀ**.

---

## 📌 TỔNG QUAN KIẾN TRÚC & LUỒNG LÀM VIỆC

```text
┌─────────────────────────────────────────────────────────────┐
│       MÁY TÍNH DEVELOPER / TEAM LEAD (Ubuntu / WSL 2)       │
│                                                             │
│   1. Chạy cài đặt tự động 1 lần:                            │
│        ./auto_setup_env/auto_setup_env.sh                   │
│        (Tự cài compiler ARM + Tự tải Sysroot từ GitHub)     │
│                                                             │
│   2. Sửa code và Deploy sang bo mạch:                       │
│        ./deploy.sh                                          │
│        (Tự động make + Tự động bắn SCP + Tự restart app)    │
└──────────────────────────────┬──────────────────────────────┘
                               │
                       [ Bắn qua mạng SCP ]
                               │
                               ▼
┌─────────────────────────────────────────────────────────────┐
│          BO MẠCH FORLINX OKM6ULL-S (i.MX6ULL Linux)         │
│                                                             │
│   • IP Bo mạch: 192.168.1.214 (Wi-Fi: Hunonic Wifi 32 Ky Tu) │
│   • Nhận file và thực thi trực tiếp: /usr/bin/mqtt_led_app  │
│   • MQTT Broker chạy sẵn tại port 1883                      │
│   • Quản lý tiến trình qua SysVinit: /etc/init.d/           │
└─────────────────────────────────────────────────────────────┘
```

---

## 🚀 PHẦN 1: THIẾT LẬP MÔI TRƯỜNG TỰ ĐỘNG (1 BƯỚC DUY NHẤT)

*(Áp dụng cho máy Ubuntu Native hoặc Windows 10/11 WSL 2)*.

Chỉ cần mở Terminal tại thư mục dự án `HNN_OKM6ULL_OTA` và chạy:

```bash
./auto_setup_env/auto_setup_env.sh
```

👉 **Script sẽ tự động làm toàn bộ mọi việc:**
1. Cài đặt trình biên dịch ARM `gcc-arm-linux-gnueabihf` và công cụ `make`.
2. Tự động tải gói thư viện `sysroot_okm6ull.tar.gz` (47 MB) từ GitHub Release `v1.1`.
3. Tự động giải nén thư mục `recipe-sysroot` (chứa `mosquitto.h`, `libmosquitto.so`, `glibc`...).
4. Tạo ví dụ mẫu `examples/mqtt_simple.c` và biên dịch thử nghiệm thành công 100%!

---

## ⚡ PHẦN 2: BIÊN DỊCH & DEPLOY SANG BO MẠCH (1 CHẠM)

Mỗi lần Developer / Team Lead sửa code xong, chỉ cần gõ:

```bash
./deploy.sh
```

*(Script sẽ tự động compile code C, bắn file qua SCP vào bo mạch và khởi động lại ứng dụng ngay lập tức)*.

---

## 📡 PHẦN 3: VÍ DỤ VÀ CÁCH TEST MQTT TRÊN BO MẠCH

Ứng dụng `mqtt_led_app` trên bo mạch đã tích hợp sẵn MQTT Client để nhận lệnh điều khiển LED và OTA.

### 1. Điều khiển Bo mạch từ xa qua lệnh MQTT (Từ máy tính / Điện thoại)

Trên máy tính (đã cài `mosquitto-clients`), bạn có thể bắn lệnh điều khiển đèn LED của bo mạch qua topic `test/topic`:

```bash
# 💡 Bật đèn LED:
mosquitto_pub -h 192.168.1.214 -t test/topic -m "on"

# 💡 Tắt đèn LED:
mosquitto_pub -h 192.168.1.214 -t test/topic -m "off"

# 💡 Nhấp nháy đèn LED (Heartbeat):
mosquitto_pub -h 192.168.1.214 -t test/topic -m "blink"

# 💡 Đảo trạng thái LED:
mosquitto_pub -h 192.168.1.214 -t test/topic -m "toggle"
```

---

### 2. Biên dịch & Chạy file code mẫu MQTT C (`examples/mqtt_simple.c`)

Trong thư mục `examples/` có sẵn file code mẫu đơn giản `mqtt_simple.c`.

**Cách biên dịch:**
```bash
arm-linux-gnueabihf-gcc --sysroot=./recipe-sysroot \
    -I./recipe-sysroot/usr/include \
    -L./recipe-sysroot/usr/lib \
    -march=armv7-a -mfpu=neon -mfloat-abi=hard \
    examples/mqtt_simple.c -o mqtt_simple -lmosquitto -lpthread
```

**Bắn sang bo mạch và chạy:**
```bash
scp mqtt_simple root@192.168.1.214:/usr/bin/
ssh root@192.168.1.214 "/usr/bin/mqtt_simple"
```

---

## 🛑 PHẦN 4: HƯỚNG DẪN QUẢN LÝ / DỪNG SERVICE TRÊN BO MẠCH

Khi đang debug hoặc test app mới, bạn có thể điều khiển dừng/bật app trực tiếp trên bo mạch (qua SSH hoặc Minicom UART):

### 1. Quản lý App chính (`hnn-okm6ull-ota`)

| Thao tác | Câu lệnh trên bo mạch |
| :--- | :--- |
| **Dừng app (Stop)** | `/etc/init.d/hnn-okm6ull-ota stop` |
| **Bật app (Start)** | `/etc/init.d/hnn-okm6ull-ota start` |
| **Khởi động lại (Restart)** | `/etc/init.d/hnn-okm6ull-ota restart` |
| **Tắt khẩn cấp tiến trình** | `killall -9 mqtt_led_app` |

---

### 2. Tắt hẳn đèn LED nhấp nháy khi app đã tắt
Do đèn LED được điều khiển qua Kernel trigger, khi tắt app bạn có thể tắt hẳn đèn bằng lệnh:
```bash
echo none > /sys/class/leds/led1/trigger
echo 0 > /sys/class/leds/led1/brightness
```

---

### 3. Quản lý các dịch vụ hệ thống khác

| Dịch vụ | Câu lệnh quản lý |
| :--- | :--- |
| **Mạng Wi-Fi / LAN** | `/etc/init.d/networking [restart|stop]` |
| **MQTT Broker (Mosquitto)** | `/etc/init.d/mosquitto [restart|stop]` |
| **SSH Server (SSHD)** | `/etc/init.d/sshd [restart|stop]` |

---

## 🪟 PHẦN 5: DÀNH CHO DEVELOPER DÙNG WINDOWS 10 / 11 (VS CODE + WSL 2)

1. **Bật WSL 2:** Mở PowerShell (Admin): `wsl --install -d Ubuntu`.
2. **Cài VS Code:** Cài phần mềm Visual Studio Code trên Windows + cài Extension **WSL**.
3. **Mở code:** Trong terminal Ubuntu WSL, gõ `code .` để mở dự án trên giao diện Windows.
4. **Cài đặt & Deploy:** Mở Terminal trong VS Code (`Ctrl + ~`):
   ```bash
   # Bước 1: Setup tự động (chỉ làm 1 lần)
   ./auto_setup_env/auto_setup_env.sh

   # Bước 2: Deploy mỗi lần sửa code
   ./deploy.sh
   ```

---

## 📋 THÔNG TIN KỸ THUẬT HỆ THỐNG BO MẠCH

| Thông số | Giá trị |
| :--- | :--- |
| **Phần cứng mục tiêu** | Forlinx OKM6ULL-S (NXP i.MX6ULL - ARM Cortex-A7 Hard-Float) |
| **Hệ điều hành** | Embedded Linux (Yocto Kirkstone 5.15 - SysVinit Fastboot) |
| **Thời gian Boot từ Nguồn** | **~3.0 giây** (App khởi động và Blink LED ngay lập tức) |
| **Địa chỉ IP Bo mạch** | `192.168.1.214` (Wi-Fi: *Hunonic Wifi 32 Ky Tu*) |
| **Tài khoản đăng nhập** | `root` (Không yêu cầu mật khẩu) |
| **Thư mục ứng dụng** | `/usr/bin/mqtt_led_app` |
| **Script quản lý dịch vụ** | `/etc/init.d/hnn-okm6ull-ota [start|stop|restart]` |
