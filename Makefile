# Hey Emacs, this is a -*- makefile -*-

# Target file name (without extension).
TARGET = uberducky

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).c \
	ble.c \
	hid.c \
	script.c \
	usb.c \
	$(LIBS_PATH)/LPC17xx_Startup.c \
	$(LIBS_PATH)/LPC17xx_Interrupts.c \
	$(LIBS_PATH)/ubertooth.c \
	$(LPCUSB_PATH)/usbcontrol.c \
	$(LPCUSB_PATH)/usbinit.c \
	$(LPCUSB_PATH)/usbhw_lpc.c \
	$(LPCUSB_PATH)/usbstdreq.c

include common.mk

script.c: script.txt
	./script_gen.py script.txt script > script.c

clean: begin clean_list clean_binary end
	rm -f script.c
