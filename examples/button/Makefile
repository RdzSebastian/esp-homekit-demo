PROGRAM = button

EXTRA_COMPONENTS = \
	extras/http-parser \
	$(abspath ../../components/common/wolfssl) \
	$(abspath ../../components/common/cJSON) \
	$(abspath ../../components/common/homekit)

FLASH_SIZE ?= 32

BUTTON_PIN ?= 14

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS -DBUTTON_PIN=$(BUTTON_PIN)

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
