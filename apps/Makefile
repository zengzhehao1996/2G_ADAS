###########################################################################
# Sample multi-part application Makefile
#
# Copyright (c) 2017 Linaro Limited
# Copyright (c) 2017 Open Source Foundries Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

# This is an example Makefile to demonstrate how to use mcuboot to
# deploy and upgrade images.  The image building should work on any
# supported target, however flashing will likely require changes to
# the flash addresses, depending on the partition layout of the device
# in question.
#
# running
#
#     make BOARD=frdm_k64f all
#
# should generate three "*.bin" files in this directory:
#
#   mcuboot.bin: The bootloader itself
#   signed-hello1.bin: A signed sample.
#   signed-hello2.bin: An upgrade image, signed and marked for
#   upgrade.
#
# "make flash_boot" should flash the bootloader into the flash,
# erasing the rest of the device.  If you examine the device at this
# time, you should see a message about the bootloader not being able
# to find a bootable image.
#
# "make flash_hello1" will then flash the first application into
# "slot0".  This should boot into this app, print a small message, and
# give the zephyr console.
#
# "make flash_hello2" will flash hello2 into the second slot.  The
# reset should upgrade and run the new image.  Resetting again should
# then revert back to the first app, since we did not mark this image
# as good.

# We can add on to the CMake configuration for the bootloader by
# setting this.
BOOTLOADER_CONFIG ?=

BOARD ?= aidong_linde429v13

.PHONY: check boot demo clean_boot clean_demo linde_gpio clean_linde_gpio

# For signing, use the default RSA demo key, to match the default in
# the mcuboot Makefile.
# SIGNING_KEY ?= ../keys/root-rsa-2048.pem
SIGNING_KEY ?= ../keys/syp_sign.pem

# The header size should match that in hello1/prj.conf
# CONFIG_TEXT_SECTION_OFFSET.  This value needs to be a power of two
# that is at least as large as the size of the vector table.  The
# value given here of 0x200 should be sufficient for any supported
# devices, but it can be made smaller, as long as this value matches
# that used to build the app.
BOOT_HEADER_LEN = 0x200

# For upgrades, the signing tool needs to know the device alignment.
# This requirement will be going away soon.
FLASH_ALIGNMENT = 8

IMGTOOL = ../mcuboot/scripts/imgtool.py
ASSEMBLE = ../mcuboot/scripts/assemble.py

SOURCE_DIRECTORY := $(CURDIR)
BUILD_TIME	= $(shell date +%s)
BUILD_DIRECTORY := $(CURDIR)/build/$(BOARD)
BUILD_DIR_BOOT := $(BUILD_DIRECTORY)/mcuboot
BUILD_DIR_LINDE := $(BUILD_DIRECTORY)/linde

SYP_PADDING := 0x40000

help:
	@echo "make <target> BOARD=<board>"
	@echo "<target>: all, boot"
	@echo "<board>: aidong_linde429v13" 

all: boot

clean: clean_boot 
	@echo "Cleaning......"
	@rm -rf build/
	@rm -f mcuboot.bin
	@rm -rf out
	@rm -f signed-linde_v131.bin
	@rm -f mcuboot-v13.bin
	@rm -f mcuboot-v14.bin
	@rm -f core
	@rm -rf linde_v131/build/
	@rm -rf diff
	@rm -rf ../scripts/__pycache__/
	@rm -rf ../scripts/dts/__pycache__/
	@rm -rf ../scripts/kconfig/__pycache__/

boot: check
	@rm -f mcuboot.bin
	(mkdir -p $(BUILD_DIR_BOOT) && \
		cd $(BUILD_DIR_BOOT) && \
		cmake $(BOOTLOADER_CONFIG) \
			-G"Unix Makefiles" \
			-DBOARD=$(BOARD) \
			$(SOURCE_DIRECTORY)/../mcuboot/boot/zephyr && \
		make -j$(nproc))
	cp $(BUILD_DIR_BOOT)/zephyr/zephyr.bin mcuboot.bin

clean_boot: check
	@rm -rf $(BUILD_DIR_BOOT)

# Build and sign "linde_v131".
linde: check
	(mkdir -p $(BUILD_DIR_LINDE) && \
		cd $(BUILD_DIR_LINDE) && \
		cmake -G"Unix Makefiles" \
			-DBOARD=$(BOARD) \
			$(SOURCE_DIRECTORY)/linde && \
		make -j$(nproc))
	$(IMGTOOL) sign \
		--key $(SIGNING_KEY) \
		--header-size $(BOOT_HEADER_LEN) \
		--align $(FLASH_ALIGNMENT) \
		--version 1.2.0+$(BUILD_TIME) \
		--included-header \
		--pad $(SYP_PADDING)\
		$(BUILD_DIR_LINDE)/zephyr/zephyr.bin \
		signed-zephyr.bin

clean_linde: check
	rm -rf $(BUILD_DIR_LINDE)

clean_out: check
	rm -rf out/
