#!/bin/bash
# =========================================================
# Script Biên Dịch & Ném File Sang Bo Mạch OKM6ULL-S
# =========================================================

# Địa chỉ IP của bo mạch (có thể thay đổi theo mạng của bạn)
TARGET_IP="192.168.1.214"
APP_NAME="mqtt_led_app"
REMOTE_PATH="/usr/bin"

echo "========================================="
echo "  🚀 BẮT ĐẦU QUY TRÌNH DEPLOY LÊN BO MẠCH"
echo "  🎯 Target IP: $TARGET_IP"
echo "========================================="

# 1. Kiểm tra compiler
if ! command -v arm-linux-gnueabihf-gcc &> /dev/null; then
    echo "❌ Lỗi: Chưa cài đặt arm-linux-gnueabihf-gcc!"
    echo "👉 Hãy chạy: sudo apt install gcc-arm-linux-gnueabihf"
    exit 1
fi

# 2. Kiểm tra thư viện sysroot
if [ ! -d "./recipe-sysroot" ]; then
    if [ -f "./sysroot_okm6ull.tar.gz" ]; then
        echo "📦 Đang giải nén sysroot_okm6ull.tar.gz..."
        tar -xzf ./sysroot_okm6ull.tar.gz
    else
        echo "❌ Lỗi: Không tìm thấy thư mục recipe-sysroot hoặc file sysroot_okm6ull.tar.gz!"
        exit 1
    fi
fi

# 3. Biên dịch
echo "🔨 Đang biên dịch mã nguồn C..."
make clean && make
if [ $? -ne 0 ]; then
    echo "❌ Biên dịch thất bại!"
    exit 1
fi

# 4. Gửi file sang bo mạch
echo "📤 Đang gửi file $APP_NAME sang bo mạch qua SCP..."
scp build/$APP_NAME root@$TARGET_IP:$REMOTE_PATH/
if [ $? -ne 0 ]; then
    echo "❌ Gửi file thất bại! Kiểm tra lại kết nối Wi-Fi/IP của bo mạch."
    exit 1
fi

# 5. Khởi động lại service trên bo mạch
echo "🔄 Đang khởi động lại service trên bo mạch..."
ssh root@$TARGET_IP "/etc/init.d/hnn-okm6ull-ota restart"

echo "========================================="
echo "  ✅ HOÀN TẤT! Ứng dụng đã chạy trên bo mạch!"
echo "========================================="
