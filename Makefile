# Hey Emacs, this is a -*- makefile -*-

# Target file name (without extension).
TARGET = uberducky

# List C source files here. (C dependencies are automatically generated.)
SRC = $(TARGET).c \
	hid.c \
	usb.c \
	$(LIBS_PATH)/LPC17xx_Startup.c \
	$(LIBS_PATH)/LPC17xx_Interrupts.c \
	$(LIBS_PATH)/ubertooth.c \
	$(LPCUSB_PATH)/usbcontrol.c \
	$(LPCUSB_PATH)/usbinit.c \
	$(LPCUSB_PATH)/usbhw_lpc.c \
	$(LPCUSB_PATH)/usbstdreq.c

include common.mk
