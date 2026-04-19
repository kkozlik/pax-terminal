# Build settings for iquest shared module.
TARGET := iquest.so
SOURCES := iquest.c ../utils/fb.c ../utils/kbd.c ../utils/gfx.c ../utils/bmp.c ../utils/system.c ../utils/printer.c mp3.c
RES_FILES := $(shell find res -type f 2>/dev/null)
PUSH_DEPS := $(TARGET) $(RES_FILES)
PUSH_STAMP := .push.stamp
APP_DIR := data/app/MAINAPP

DEVICE ?= ACM0
DEST_LIB ?= /data/app/MAINAPP/iquest/$(TARGET)
REPO ?=

DEPS := $(TARGET:.so=.d)

.PHONY: all build install push clean help

all: build

build: $(TARGET)

$(TARGET): $(SOURCES)
	arm-none-eabi-gcc -shared -fPIC -I /usr/arm-linux-gnueabi/include/ -I ../utils \
		-MMD -MP -o $(TARGET) $(SOURCES) -nostartfiles -static

install: $(TARGET)
	@echo "Pushing $(TARGET) -> $(DEST_LIB)"
	python3 ../client.py $(DEVICE) push $(TARGET) $(DEST_LIB)

push: $(PUSH_STAMP)

# Uses make timestamps:
# - prerequisites are $(TARGET) and files in res/
# - $? expands to prerequisites newer than .push.stamp
$(PUSH_STAMP): $(PUSH_DEPS)
	@if [ -z "$(REPO)" ]; then \
		echo "Set REPO, e.g. make push REPO=../other-repo/$(TARGET)"; \
		exit 1; \
	fi
	@if [ ! -d "$(REPO)" ]; then \
		echo "Invalid path to repo $(REPO)"; \
		exit 1; \
	fi
	@echo "Copying changed files -> $(REPO)"
	@for f in $?; do \
		if [ "$$f" = "$(TARGET)" ]; then \
			cp -f "$(TARGET)" "$(REPO)/$(APP_DIR)/iquest/"; \
			cp -f "$(TARGET)" "$(REPO)/$(APP_DIR)/lib/libosal.so"; \
			continue; \
		fi; \
		if [ ! -f "$$f" ]; then \
			continue; \
		fi; \
		case "$$f" in \
			res/*) dest_rel="$(APP_DIR)/iquest/$${f#res/}" ;; \
			*) continue ;; \
		esac; \
		mkdir -p "$(REPO)/$$(dirname "$$dest_rel")"; \
		cp -f "$$f" "$(REPO)/$$dest_rel"; \
	done
	@touch "$(PUSH_STAMP)"

clean:
	@rm -f $(TARGET) $(DEPS) $(PUSH_STAMP)

help:
	@echo "Targets:"
	@echo "  all/build   - build $(TARGET)"
	@echo "  install     - push $(TARGET) (DEVICE=$(DEVICE), DEST_LIB=$(DEST_LIB))"
	@echo "  push        - copy $(TARGET) + changed res/* to REPO=path/to/repo"
	@echo "  clean       - remove $(TARGET)"

-include $(DEPS)
