# =========================================================
# Compiler & Flags
# =========================================================
CC = arm-linux-gnueabihf-gcc

SYSROOT_DIR ?= $(wildcard ./recipe-sysroot)
ifneq ($(SYSROOT_DIR),)
    SYSROOT_FLAGS = --sysroot=$(SYSROOT_DIR) -I$(SYSROOT_DIR)/usr/include -L$(SYSROOT_DIR)/usr/lib
endif

CFLAGS  = -march=armv7-a -mfpu=neon -mfloat-abi=hard \
          -Ihardware \
          -Iconfig \
          -Imiddle/mqtt \
          -Imiddle/ota \
          -Ithird_party/jsmn \
          $(SYSROOT_FLAGS) \
          -Wall -Wextra

LDFLAGS = -lmosquitto -lpthread

# =========================================================
# Project Output & Sources
# =========================================================
TARGET  = mqtt_led_app
BINDIR  = build

SRCS    = src/main.c \
          hardware/led/led.c \
          hardware/button/button.c \
          middle/mqtt/mqtt.c \
          middle/ota/ota.c \
          third_party/jsmn/jsmn.c

OBJS    = $(patsubst %.c, $(BINDIR)/%.o, $(SRCS))

# =========================================================
# Rules
# =========================================================
all: $(BINDIR) $(BINDIR)/$(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)/src \
	         $(BINDIR)/config \
	         $(BINDIR)/hardware/led \
	         $(BINDIR)/hardware/button \
	         $(BINDIR)/middle/mqtt \
	         $(BINDIR)/middle/ota \
	         $(BINDIR)/third_party/jsmn

$(BINDIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "========================================="
	@echo "[OK] Biên dịch thành công: $@"
	@echo "========================================="

$(BINDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BINDIR)
	@echo "[OK] Cleaned"

install: $(BINDIR)/$(TARGET)
	cp $(BINDIR)/$(TARGET) $(ROOTFS)/usr/bin/
	@echo "[OK] Installed to rootfs"

.PHONY: all clean install
