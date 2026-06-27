#include "tusb.h"

// Sony DS3 Identifiers
#define USB_VID 0x054C
#define USB_PID 0x0268

// Device Descriptor
const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) { return (uint8_t const *)&desc_device; }

// HID Report Descriptor: Official Sony DualShock 3 descriptor
uint8_t const desc_hid_report[] = {
    0x05, 0x01,         // Usage Page (Generic Desktop)
    0x09, 0x04,         // Usage (Joystick)
    0xa1, 0x01,         // Collection (Application)
    0xa1, 0x02,         //   Collection (Logical)
    0x85, 0x01,         //     Report ID (1)
    0x75, 0x08,         //     Report Size (8)
    0x95, 0x01,         //     Report Count (1)
    0x15, 0x00,         //     Logical Minimum (0)
    0x26, 0xff, 0x00,   //     Logical Maximum (255)
    0x81, 0x03,         //     Input (Constant, Variable, Absolute)
    0x75, 0x01,         //     Report Size (1)
    0x95, 0x13,         //     Report Count (19)
    0x15, 0x00,         //     Logical Minimum (0)
    0x25, 0x01,         //     Logical Maximum (1)
    0x35, 0x00,         //     Physical Minimum (0)
    0x45, 0x01,         //     Physical Maximum (1)
    0x05, 0x09,         //     Usage Page (Button)
    0x19, 0x01,         //     Usage Minimum (Button 1)
    0x29, 0x13,         //     Usage Maximum (Button 19)
    0x81, 0x02,         //     Input (Data, Variable, Absolute)
    0x75, 0x01,         //     Report Size (1)
    0x95, 0x0d,         //     Report Count (13)
    0x06, 0x00, 0xff,   //     Usage Page (Vendor Defined)
    0x81, 0x03,         //     Input (Constant, Variable, Absolute)
    0x15, 0x00,         //     Logical Minimum (0)
    0x26, 0xff, 0x00,   //     Logical Maximum (255)
    0x05, 0x01,         //     Usage Page (Generic Desktop)
    0x09, 0x01,         //     Usage (Pointer)
    0xa1, 0x00,         //     Collection (Physical)
    0x75, 0x08,         //       Report Size (8)
    0x95, 0x04,         //       Report Count (4)
    0x35, 0x00,         //       Physical Minimum (0)
    0x46, 0xff, 0x00,   //       Physical Maximum (255)
    0x09, 0x30,         //       Usage (X)
    0x09, 0x31,         //       Usage (Y)
    0x09, 0x32,         //       Usage (Z)
    0x09, 0x35,         //       Usage (Rx)
    0x81, 0x02,         //       Input (Data, Variable, Absolute)
    0xc0,               //     End Collection
    0x05, 0x01,         //     Usage Page (Generic Desktop)
    0x75, 0x08,         //     Report Size (8)
    0x95, 0x27,         //     Report Count (39)
    0x09, 0x01,         //     Usage (Pointer)
    0x81, 0x02,         //     Input (Data, Variable, Absolute)
    0x75, 0x08,         //     Report Size (8)
    0x95, 0x30,         //     Report Count (48)
    0x09, 0x01,         //     Usage (Pointer)
    0x91, 0x02,         //     Output (Data, Variable, Absolute)
    0x75, 0x08,         //     Report Size (8)
    0x95, 0x30,         //     Report Count (48)
    0x09, 0x01,         //     Usage (Pointer)
    0xb1, 0x02,         //     Feature (Data, Variable, Absolute)
    0xc0,               //   End Collection
    0xa1, 0x02,         //   Collection (Logical)
    0x85, 0x02,         //     Report ID (2)
    0x75, 0x08,         //     Report Size (8)
    0x95, 0x30,         //     Report Count (48)
    0x09, 0x01,         //     Usage (Pointer)
    0xb1, 0x02,         //     Feature (Data, Variable, Absolute)
    0xc0,               //   End Collection
    0xa1, 0x02,         //   Collection (Logical)
    0x85, 0xee,         //     Report ID (238)
    0x75, 0x08,         //     Report Size (8)
    0x95, 0x30,         //     Report Count (48)
    0x09, 0x01,         //     Usage (Pointer)
    0xb1, 0x02,         //     Feature (Data, Variable, Absolute)
    0xc0,               //   End Collection
    0xa1, 0x02,         //   Collection (Logical)
    0x85, 0xef,         //     Report ID (239)
    0x75, 0x08,         //     Report Size (8)
    0x95, 0x30,         //     Report Count (48)
    0x09, 0x01,         //     Usage (Pointer)
    0xb1, 0x02,         //     Feature (Data, Variable, Absolute)
    0xc0,               //   End Collection
    0xc0                // End Collection
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return desc_hid_report;
}

// Configuration Descriptor
enum { ITF_NUM_HID, ITF_NUM_TOTAL };
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

#define EPNUM_HID_OUT 0x02
#define EPNUM_HID_IN  0x81

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID_OUT, EPNUM_HID_IN, 64, 10)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

// String Descriptors
const char* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: Language (English)
    "Sony",                        // 1: Manufacturer
    "PLAYSTATION(R)3 Controller",  // 2: Product (Official name)
};

static uint16_t _desc_str[32];
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;
    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for(uint8_t i=0; i<chr_count; i++) {
            _desc_str[1+i] = str[i];
        }
    }
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);
    return _desc_str;
}