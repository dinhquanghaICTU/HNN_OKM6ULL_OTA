#!/bin/bash

set -e

SYSROOT_URL="https://github.com/dinhquanghaiCTU/HNN_OKM6ULL_OTA/releases/download/v1.1/sysroot_okm6ull.tar.gz"
SYSROOT_ARCHIVE="sysroot_okm6ull.tar.gz"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=================================================================="
echo "  BẮT ĐẦU CÀI ĐẶT TỰ ĐỘNG MÔI TRƯỜNG BIÊN DỊCH CHO OKM6ULL-S"
echo "  Thư mục dự án: $PROJECT_ROOT"
echo "=================================================================="

# 1. Cài đặt Toolchain ARM và các công cụ cơ bản
echo ""
echo "Đang cài đặt Toolchain ARM (GCC/G++/Make/Wget)..."
sudo apt update
sudo apt install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf make wget tar

echo ""
cd "$PROJECT_ROOT"
if [ ! -d "recipe-sysroot" ]; then
    if [ ! -f "$SYSROOT_ARCHIVE" ]; then
        wget --progress=bar:force -O "$SYSROOT_ARCHIVE" "$SYSROOT_URL"
    fi
    tar -xzf "$SYSROOT_ARCHIVE"
    echo "Giải nén thành công!"
else
    echo "Thư mục recipe-sysroot đã tồn tại sẵn."
fi

# 3. Tạo file ví dụ MQTT cực kỳ đơn giản (examples/mqtt_simple.c)
echo ""
echo "Đang tạo ví dụ MQTT đơn giản (examples/mqtt_simple.c)..."
mkdir -p examples

cat << 'END_MQTT_C' > examples/mqtt_simple.c
#include <stdio.h>
#include <string.h>
#include <mosquitto.h>

#define BROKER_IP   "127.0.0.1"   // Hoặc đổi thành IP Bo mạch
#define BROKER_PORT 1883
#define TOPIC       "test/topic"

int main(void)
{
    struct mosquitto *mosq;
    const char *msg = "Hello OKM6ULL MQTT!";

    printf("--- [MQTT SIMPLE TEST] ---\n");
    mosquitto_lib_init();

    mosq = mosquitto_new("simple_pub", true, NULL);
    if (!mosq) {
        printf("Lỗi tạo instance mosquitto\n");
        return 1;
    }

    if (mosquitto_connect(mosq, BROKER_IP, BROKER_PORT, 60) == MOSQ_ERR_SUCCESS) {
        printf("Kết nối MQTT Broker (%s:%d) thành công!\n", BROKER_IP, BROKER_PORT);
        mosquitto_publish(mosq, NULL, TOPIC, strlen(msg), msg, 0, false);
        printf("Đã gửi tin nhắn: '%s' tới topic [%s]\n", msg, TOPIC);
    } else {
        printf("Không thể kết nối tới Broker (%s:%d)\n", BROKER_IP, BROKER_PORT);
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    printf("--- [HOÀN TẤT] ---\n");
    return 0;
}
END_MQTT_C

# 4. Biên dịch thử nghiệm
echo ""
echo "🔨 [4/4] Biên dịch dự án & ví dụ MQTT..."
make clean
make

arm-linux-gnueabihf-gcc --sysroot=./recipe-sysroot \
    -I./recipe-sysroot/usr/include \
    -L./recipe-sysroot/usr/lib \
    -march=armv7-a -mfpu=neon -mfloat-abi=hard \
    examples/mqtt_simple.c -o build/mqtt_simple -lmosquitto -lpthread

echo ""
echo "=================================================================="
echo "  
echo "  App chính: $PROJECT_ROOT/build/mqtt_led_app"
echo "  App test:  $PROJECT_ROOT/build/mqtt_simple"
echo "  Để deploy sang bo mạch: Chạy file ./deploy.sh"
echo "=================================================================="
