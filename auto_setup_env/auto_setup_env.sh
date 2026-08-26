#!/bin/bash
# ==============================================================================
# Script Tự Động Cài Đặt Môi Trường Biên Dịch Chéo OKM6ULL-S
# Repository: dinhquanghaiCTU / HNN_OKM6ULL_OTA
# ==============================================================================

set -e

SYSROOT_URL="https://github.com/dinhquanghaiCTU/HNN_OKM6ULL_OTA/releases/download/v1.1/sysroot_okm6ull.tar.gz"
SYSROOT_ARCHIVE="sysroot_okm6ull.tar.gz"

# Tìm thư mục gốc của project (thư mục cha của auto_setup_env)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=================================================================="
echo "  🚀 BẮT ĐẦU CÀI ĐẶT TỰ ĐỘNG MÔI TRƯỜNG BIÊN DỊCH CHO OKM6ULL-S"
echo "  📂 Thư mục dự án: $PROJECT_ROOT"
echo "=================================================================="

# 1. Cài đặt các gói công cụ cần thiết từ Ubuntu
echo ""
echo "📦 [1/5] Đang cập nhật và cài đặt Toolchain ARM (GCC/G++/Make/Wget)..."
sudo apt update
sudo apt install -y gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf make wget tar

# 2. Tải Sysroot Thư viện từ GitHub Releases nếu chưa có
echo ""
cd "$PROJECT_ROOT"
if [ ! -d "recipe-sysroot" ]; then
    if [ ! -f "$SYSROOT_ARCHIVE" ]; then
        echo "📥 [2/5] Đang tải sysroot_okm6ull.tar.gz (47 MB) từ GitHub Release v1.1..."
        wget --progress=bar:force -O "$SYSROOT_ARCHIVE" "$SYSROOT_URL"
    else
        echo "✅ [2/5] Đã tìm thấy file $SYSROOT_ARCHIVE có sẵn."
    fi

    # 3. Giải nén Sysroot
    echo ""
    echo "📂 [3/5] Đang giải nén thư viện hệ thống (recipe-sysroot)..."
    tar -xzf "$SYSROOT_ARCHIVE"
    echo "✅ Giải nén thành công!"
else
    echo "✅ [2/5] & [3/5] Thư mục recipe-sysroot đã tồn tại sẵn, bỏ qua bước tải."
fi

# 4. Tự động tạo mã nguồn ví dụ MQTT (examples/mqtt_demo.c)
echo ""
echo "📝 [4/5] Đang tạo file mã nguồn ví dụ MQTT (examples/mqtt_demo.c)..."
mkdir -p examples

cat << 'END_MQTT_C' > examples/mqtt_demo.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mosquitto.h>

#define BROKER_HOST "127.0.0.1"
#define BROKER_PORT 1883
#define TOPIC       "test/topic"

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    if (rc == 0) {
        printf("✅ [MQTT] Kết nối thành công tới %s:%d!\n", BROKER_HOST, BROKER_PORT);
        mosquitto_subscribe(mosq, NULL, TOPIC, 0);
        printf("📡 [MQTT] Đã Subscribe topic: [%s]\n", TOPIC);
    } else {
        printf("❌ [MQTT] Kết nối thất bại (Mã lỗi: %d)\n", rc);
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    printf("📩 [MQTT NHẬN ĐƯỢC] Topic: %s | Payload: %s\n", msg->topic, (char *)msg->payload);
    if (strcmp((char *)msg->payload, "ping") == 0) {
        printf("👉 Gửi lại phản hồi 'pong'...\n");
        mosquitto_publish(mosq, NULL, "test/response", 4, "pong", 0, false);
    }
}

int main(void)
{
    struct mosquitto *mosq;
    printf("====================================================\n");
    printf("  🚀 VÍ DỤ MQTT CLIENT TRÊN FORLINX OKM6ULL-S\n");
    printf("====================================================\n");

    mosquitto_lib_init();
    mosq = mosquitto_new("okm6ull_demo_client", true, NULL);
    if (!mosq) {
        fprintf(stderr, "❌ Lỗi: Không thể tạo đối tượng mosquitto!\n");
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    printf("🔄 Đang kết nối tới MQTT Broker (%s:%d)...\n", BROKER_HOST, BROKER_PORT);
    if (mosquitto_connect(mosq, BROKER_HOST, BROKER_PORT, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "❌ Lỗi: Không thể kết nối tới Broker!\n");
        return 1;
    }

    const char *hello_msg = "Hello from OKM6ULL ARM Board!";
    mosquitto_publish(mosq, NULL, TOPIC, strlen(hello_msg), hello_msg, 0, false);
    printf("📤 [MQTT] Đã gửi tin nhắn: '%s'\n", hello_msg);

    printf("⏳ Đang lắng nghe tin nhắn trong 30 giây (Nhấn Ctrl+C để thoát)...\n");
    for (int i = 0; i < 30; i++) {
        mosquitto_loop(mosq, 1000, 1);
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    printf("✅ Đã kết thúc chương trình MQTT Demo.\n");
    return 0;
}
END_MQTT_C

# 5. Kiểm tra và Biên dịch thử nghiệm cả App chính và Ví dụ MQTT
echo ""
echo "🔨 [5/5] Kiểm tra biên dịch mã nguồn dự án & ví dụ MQTT..."
make clean
make

# Biên dịch ví dụ MQTT
arm-linux-gnueabihf-gcc --sysroot=./recipe-sysroot \
    -I./recipe-sysroot/usr/include \
    -L./recipe-sysroot/usr/lib \
    -march=armv7-a -mfpu=neon -mfloat-abi=hard \
    examples/mqtt_demo.c -o build/mqtt_demo -lmosquitto -lpthread

echo ""
echo "=================================================================="
echo "  🎉 CHÚC MỪNG! MÔI TRƯỜNG ĐÃ ĐƯỢC THIẾT LẬP THÀNH CÔNG 100%!"
echo "  📁 File thực thi App chính: $PROJECT_ROOT/build/mqtt_led_app"
echo "  📁 File thực thi Demo MQTT: $PROJECT_ROOT/build/mqtt_demo"
echo "  👉 Để deploy sang bo mạch: Chạy file ./deploy.sh"
echo "=================================================================="
