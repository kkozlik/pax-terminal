# MbedTLS location used for HTTPS builds.
MBEDTLS_DIR := ../external/mbedtls
MBEDTLS_BUILD := $(MBEDTLS_DIR)/build
MBEDTLS_LIBS := \
	$(MBEDTLS_BUILD)/library/libmbedtls.a \
	$(MBEDTLS_BUILD)/library/libmbedx509.a \
	$(MBEDTLS_BUILD)/library/libmbedcrypto.a

TARGET := wifi.so
DEVICE ?= ACM0
DEPS := $(TARGET:.so=.d)
# Read config in the same order as the C includes: base config, then optional local override.
CONFIG_FILES := wifi_config.h wifi_config.local.h
# Extract USE_HTTPS if defined in either config; fall back to 0 to avoid accidental TLS linking.
USE_HTTPS := $(shell awk '/^[[:space:]]*#define[[:space:]]+USE_HTTPS[[:space:]]+/ {val=$$3} END {if (val=="") print 0; else print val}' $(CONFIG_FILES) 2>/dev/null)

# Base sources and includes (no HTTPS).
SOURCES := wifi.c
CPPFLAGS := -I /usr/arm-linux-gnueabi/include/

ifeq ($(USE_HTTPS),1)
# HTTPS path: add TLS sources, include dirs, and link MbedTLS.
SOURCES += https.c mbedtls_entropy.c
CPPFLAGS += \
	-I $(MBEDTLS_DIR)/include/ \
	-I $(MBEDTLS_DIR)/tf-psa-crypto/include/ \
	-I $(MBEDTLS_DIR)/tf-psa-crypto/drivers/builtin/include/
LINK_LIBS := $(MBEDTLS_LIBS)
else
# Non-HTTPS path: do not compile or link any MbedTLS objects.
LINK_LIBS :=
endif

.PHONY: all build install clean help

all: build

build: $(TARGET)

$(TARGET): $(SOURCES) $(LINK_LIBS) $(CONFIG_FILES)
	arm-none-eabi-gcc -shared -fPIC $(CPPFLAGS) \
		-MMD -MP -o $(TARGET) $(SOURCES) $(LINK_LIBS) -nostartfiles -static

install: $(TARGET)
	@echo Pushing $(TARGET)
	python3 ../client.py $(DEVICE) push $(TARGET) /data/app/MAINAPP/lib/libosal.so
	@echo Pushing Loader
	python3 ../client.py $(DEVICE) push L /data/app/MAINAPP/Loader
	@echo "Now reboot and it will download itself ... if you have Wi-Fi configured"

clean:
	@rm -f $(TARGET) $(DEPS)

help:
	@echo "Targets:"
	@echo "  all/build   - build $(TARGET)"
	@echo "  install     - push $(TARGET) and Loader (DEVICE=$(DEVICE))"
	@echo "  clean       - remove $(TARGET)"
	@echo "Config:"
	@echo "  USE_HTTPS   - $(USE_HTTPS) (from $(CONFIG_FILES))"

-include $(DEPS)
