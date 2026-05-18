# =========================================================
# Toolchain
# =========================================================
TOOLCHAIN = /opt/fsl-imx-x11/4.1.15-2.0.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi

CC      = arm-poky-linux-gnueabi-gcc
CFLAGS  = -march=armv7ve -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7 \
          --sysroot=/opt/fsl-imx-x11/4.1.15-2.0.0/sysroots/cortexa7hf-neon-poky-linux-gnueabi

# Tự động source toolchain
export PATH := /opt/fsl-imx-x11/4.1.15-2.0.0/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-gnueabi:$(PATH)
# =========================================================
# Project
# =========================================================
TARGET  = mqtt_led_app
BINDIR  = build

SRCS    = src/main.c \
          hardware/led/led.c \
		  hardware/button/button.c \
          middle/mqtt/mqtt.c \
          middle/ota/ota.c \
          third_party/jsmn/jsmn.c \
          third_party/lib_button/app_btn.c

OBJS    = $(patsubst %.c, $(BINDIR)/%.o, $(SRCS))


CFLAGS += -Ihardware \
          -Imiddle/mqtt \
          -Imiddle/ota \
          -Ithird_party/jsmn \
          -Ithird_party/lib_button \
          -Wall -Wextra

# =========================================================
# Rules
# =========================================================
all: $(BINDIR) $(BINDIR)/$(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR) \
	         $(BINDIR)/src \
			 $(BINDIR)/config \
	         $(BINDIR)/hardware/led \
			 $(BINDIR)/hardware/button \
	         $(BINDIR)/middle/mqtt \
	         $(BINDIR)/middle/ota \
	         $(BINDIR)/third_party/jsmn \
	         $(BINDIR)/third_party/lib_button

$(BINDIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "[OK] Built: $@"

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BINDIR)
	@echo "[OK] Cleaned"

install: $(BINDIR)/$(TARGET)
	cp $(BINDIR)/$(TARGET) $(ROOTFS)/usr/bin/
	@echo "[OK] Installed to rootfs"

.PHONY: all clean install