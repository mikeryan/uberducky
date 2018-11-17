/*
    LPCUSB, an USB device driver for LPC microcontrollers
    Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. The name of the author may not be used to endorse or promote products
       derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "usbapi.h"
#include "usbhw_lpc.h"
#include "ubertooth.h"

#define LE_WORD(x)      ((x)&0xFF),((x)>>8)

#define REPORT_SIZE         8

#define INTR_IN_EP      0x81

static U8   abClassReqData[4];
static int  _iIdleRate = 0;

// Report descriptor from Apple Aluminum Keyboard MB110LL/A
static U8 abReportDesc[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0xFF,        //   Usage Page (Reserved 0xFF)
    0x09, 0x03,        //   Usage (0x03)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
};

static U8 abDescriptors[] = {

    // Device descriptor
    0x12,
    DESC_DEVICE,
    LE_WORD(0x0110),        // bcdUSB
    0x00,                   // bDeviceClass
    0x00,                   // bDeviceSubClass
    0x00,                   // bDeviceProtocol
    MAX_PACKET_SIZE0,       // bMaxPacketSize
    LE_WORD(0x05AC),        // idVendor
    LE_WORD(0x2227),        // idProduct
    LE_WORD(0x0100),        // bcdDevice
    0x01,                   // iManufacturer
    0x02,                   // iProduct
    0x03,                   // iSerialNumber
    0x01,                   // bNumConfigurations

    // configuration
    0x09,
    DESC_CONFIGURATION,
    LE_WORD(0x22),          // wTotalLength
    0x01,                   // bNumInterfaces
    0x01,                   // bConfigurationValue
    0x00,                   // iConfiguration
    0x80,                   // bmAttributes
    0x32,                   // bMaxPower

    // interface
    0x09,
    DESC_INTERFACE,
    0x00,                   // bInterfaceNumber
    0x00,                   // bAlternateSetting
    0x01,                   // bNumEndPoints
    0x03,                   // bInterfaceClass = HID
    0x00,                   // bInterfaceSubClass
    0x00,                   // bInterfaceProtocol
    0x00,                   // iInterface

    // HID descriptor
    0x09,
    DESC_HID_HID,           // bDescriptorType = HID
    LE_WORD(0x0110),        // bcdHID
    0x00,                   // bCountryCode
    0x01,                   // bNumDescriptors = report
    DESC_HID_REPORT,        // bDescriptorType
    LE_WORD(sizeof(abReportDesc)),

    // EP descriptor
    0x07,
    DESC_ENDPOINT,
    INTR_IN_EP,             // bEndpointAddress
    0x03,                   // bmAttributes = INT - interrupt
    LE_WORD(MAX_PACKET_SIZE0),// wMaxPacketSize
    10,                     // bInterval

    // string descriptors
    0x04,
    DESC_STRING,
    LE_WORD(0x0409),

    // manufacturer string
    0x0A,
    DESC_STRING,
    'I', 0, 'C', 0, 'E', 0, '9', 0,

    // product string
    0x14,
    DESC_STRING,
    'U', 0, 'b', 0, 'e', 0, 'r', 0, 'd', 0, 'u', 0, 'c', 0, 'k', 0, 'y', 0,

    // serial number string
    0x42,
    DESC_STRING,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '1', 0,

    // terminator
    0
};

#define USB_SERIAL_OFFSET 88

/*************************************************************************
    HIDHandleStdReq
    ===============
        Standard request handler for HID devices.

    This function tries to service any HID specific requests.

**************************************************************************/
static BOOL HIDHandleStdReq(TSetupPacket *pSetup, int *piLen, U8 **ppbData)
{
    U8  bType, bIndex;

    if ((pSetup->bmRequestType == 0x81) &&          // standard IN request for interface
        (pSetup->bRequest == REQ_GET_DESCRIPTOR)) { // get descriptor

        bType = GET_DESC_TYPE(pSetup->wValue);
        bIndex = GET_DESC_INDEX(pSetup->wValue);
        switch (bType) {

        case DESC_HID_REPORT:
            // report
            *ppbData = abReportDesc;
            *piLen = sizeof(abReportDesc);
            break;

        case DESC_HID_HID:
        case DESC_HID_PHYSICAL:
        default:
            // search descriptor space
            return USBGetDescriptor(pSetup->wValue, pSetup->wIndex, piLen, ppbData);
        }

        return TRUE;
    }
    return FALSE;
}


/*************************************************************************
    HandleClassRequest
    ==================
        HID class request handler

**************************************************************************/
static BOOL HandleClassRequest(TSetupPacket *pSetup, int *piLen, U8 **ppbData)
{
    U8  *pbData = *ppbData;

    switch (pSetup->bRequest) {

    // get_idle
    case HID_GET_IDLE:
        pbData[0] = (_iIdleRate / 4) & 0xFF;
        *piLen = 1;
        break;

    // set_idle:
    case HID_SET_IDLE:
        _iIdleRate = ((pSetup->wValue >> 8) & 0xFF) * 4;
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

static void set_serial_descriptor(U8 *descriptors) {
    U8 buf[17], *desc, nibble;
    int len, i;
    get_device_serial(buf, &len);
    if(buf[0] == 0) { /* IAP success */
        desc = descriptors + USB_SERIAL_OFFSET;
        for(i=0; i<16; i++) {
            nibble  = (buf[i+1]>>4) & 0xF;
            desc[i * 4] = (nibble > 9) ? ('a' + nibble - 10) : ('0' + nibble);
            desc[1+ i * 4] = 0;
            nibble = buf[i+1]&0xF;
            desc[2 + i * 4] = (nibble > 9) ? ('a' + nibble - 10) : ('0' + nibble);
            desc[3 + i * 4] = 0;
        }
    }
}

void usb_init(void) {
    // initialise stack
    USBInit();

    set_serial_descriptor(abDescriptors);

    // register device descriptors
    USBRegisterDescriptors(abDescriptors);

    // register HID standard request handler
    USBRegisterCustomReqHandler(HIDHandleStdReq);

    // register class request handler
    USBRegisterRequestHandler(REQTYPE_TYPE_CLASS, HandleClassRequest, abClassReqData);

    // register endpoint
    USBHwRegisterEPIntHandler(INTR_IN_EP, NULL);

    // connect to bus
    USBHwConnect(TRUE);
}
