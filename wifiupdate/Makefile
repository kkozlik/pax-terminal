MBEDTLS_DIR := ../external/mbedtls
MBEDTLS_BUILD := $(MBEDTLS_DIR)/build
MBEDTLS_LIBS := \
	$(MBEDTLS_BUILD)/library/libmbedtls.a \
	$(MBEDTLS_BUILD)/library/libmbedx509.a \
	$(MBEDTLS_BUILD)/library/libmbedcrypto.a

TARGET := wifi.so
DEVICE ?= ACM0
DEPS := $(TARGET:.so=.d)

.PHONY: all build install clean help

all: build

build: $(TARGET)

$(TARGET): wifi.c mbedtls_entropy.c $(MBEDTLS_LIBS)
	arm-none-eabi-gcc -shared -fPIC -I /usr/arm-linux-gnueabi/include/ \
		-I $(MBEDTLS_DIR)/include/ \
		-I $(MBEDTLS_DIR)/tf-psa-crypto/include/ \
		-I $(MBEDTLS_DIR)/tf-psa-crypto/drivers/builtin/include/ \
		-MMD -MP -DUSE_HTTPS -o $(TARGET) wifi.c mbedtls_entropy.c $(MBEDTLS_LIBS) -nostartfiles -static

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

-include $(DEPS)
