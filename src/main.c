#include "bsp/board.h"
#include "tusb.h"
#include <string.h>

// ====================================================================
// COMBO CONFIGURATION (only active when built with -DENABLE_COMBO)
// ====================================================================
// Delay in seconds after boot before starting the combo sequence
#define COMBO_START_DELAY_SEC  35.0f

// DS3 Button Bitmasks (used by combo steps below)
#define BTN_LEFT   0x80 // Byte 2, bit 7 (D-pad Left)
#define BTN_DOWN   0x40 // Byte 2, bit 6 (D-pad Down)
#define BTN_CROSS  0x40 // Byte 3, bit 6 (Cross / X button)

#ifdef ENABLE_COMBO
typedef struct {
    uint8_t  btn2;      // D-pad + Select/Start/L3/R3 bitmask
    uint8_t  btn3;      // Face buttons + shoulder buttons bitmask
    uint8_t  btn4;      // PS button bitmask
    uint8_t  left_x;    // Left stick X axis (0x80 = center)
    uint8_t  left_y;    // Left stick Y axis (0x80 = center)
    uint8_t  right_x;   // Right stick X axis (0x80 = center)
    uint8_t  right_y;   // Right stick Y axis (0x80 = center)
    uint32_t duration;  // How long to hold this state in milliseconds (0 = end marker)
} combo_step_t;

// --------------- EDIT YOUR COMBO HERE ---------------
// Each step holds a set of inputs for 'duration' milliseconds.
// Use 0 for duration to mark the end of the sequence.
// Normal presses = a brief press followed by a release step.
static const combo_step_t COMBO_STEPS[] = {
    // Press D-pad Left for 100ms
    { BTN_LEFT, 0x00, 0, 0x80, 0x80, 0x80, 0x80, 100 },
    // Release all for 50ms
    { 0x00,     0x00, 0, 0x80, 0x80, 0x80, 0x80,  50 },
    // Press D-pad Left again for 100ms
    { BTN_LEFT, 0x00, 0, 0x80, 0x80, 0x80, 0x80, 100 },
    // Release all for 50ms
    { 0x00,     0x00, 0, 0x80, 0x80, 0x80, 0x80,  50 },
    // Press X (Cross) for 100ms
    { 0x00, BTN_CROSS, 0, 0x80, 0x80, 0x80, 0x80, 100 },
    // End marker
    { 0x00,     0x00, 0, 0x80, 0x80, 0x80, 0x80,   0 },
};
// ----------------------------------------------------
#endif // ENABLE_COMBO
// ====================================================================

// DS3 Input Report buffer: 49 bytes (byte 0 = Report ID 0x01, bytes 1-48 = state)
static uint8_t hid_report[49] = {0};

// Bluetooth MAC state for PS3 handshake emulation
static uint8_t masterBdaddr[6] = {0x00, 0x03, 0x50, 0x81, 0xd8, 0x01};
static uint8_t byte_6_ef = 0x00;
static uint8_t reply_f5  = 0;

// Official DualShock 3 Feature Report data (captured from real hardware)
static const uint8_t report_01[] = {
    0x00, 0x01, 0x04, 0x00, 0x07, 0x0c, 0x01, 0x02,
    0x18, 0x18, 0x18, 0x18, 0x09, 0x0a, 0x10, 0x11,
    0x12, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t report_f2[] = {
    0xf2, 0xff, 0xff, 0x00,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // device bdaddr
    0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,
    0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t report_f5[] = {
    0x01, 0x00,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, // dummy PS3 bdaddr (overwritten on 2nd request)
    0xff, 0xf7, 0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,
    0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
    0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,
    0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,
    0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t report_ef[] = {
    0x00, 0xef, 0x04, 0x00, 0x07, 0x03, 0x01, 0xb0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x6b, 0x02, 0x68, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t report_f8[] = {
    0x00, 0x01, 0x00, 0x00, 0x07, 0x03, 0x01, 0xb0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x02, 0x6b, 0x02, 0x68, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t report_f7[] = {
    0x01, 0x04, 0xc4, 0x02, 0xd6, 0x01, 0xee, 0xff,
    0x14, 0x13, 0x01, 0x02, 0xc4, 0x01, 0xd6, 0x00,
    0x00, 0x02, 0x02, 0x02, 0x00, 0x03, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x02, 0x62, 0x01, 0x02, 0x01,
    0x5e, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// ====================================================================
// Combo State Machine (compiled in only when ENABLE_COMBO is set)
// ====================================================================
#ifdef ENABLE_COMBO
typedef enum {
    COMBO_STATE_DELAY,    // Waiting for boot delay to expire
    COMBO_STATE_RUNNING,  // Executing combo steps
    COMBO_STATE_FINISHED, // Combo complete, no more input injected
} combo_state_t;

static combo_state_t combo_state  = COMBO_STATE_DELAY;
static uint16_t      combo_step   = 0;
static uint32_t      combo_timer  = 0;

static void reset_hid_inputs(void) {
    hid_report[2] = 0x00;   // Clear all digital buttons
    hid_report[3] = 0x00;
    hid_report[4] = 0x00;
    hid_report[5] = 0x00;
    hid_report[6] = 0x80;   // Left X  center
    hid_report[7] = 0x80;   // Left Y  center
    hid_report[8] = 0x80;   // Right X center
    hid_report[9] = 0x80;   // Right Y center
    memset(hid_report + 14, 0x00, 12); // Clear all analog pressure bytes
}

static void process_combo(void) {
    uint32_t now = board_millis();

    if (combo_state == COMBO_STATE_DELAY) {
        if (now >= (uint32_t)(COMBO_START_DELAY_SEC * 1000.0f)) {
            combo_state = COMBO_STATE_RUNNING;
            combo_step  = 0;
            combo_timer = now;
        }
        return;
    }

    if (combo_state == COMBO_STATE_FINISHED) {
        return;
    }

    // COMBO_STATE_RUNNING
    const combo_step_t* s = &COMBO_STEPS[combo_step];

    if (s->duration == 0) {
        // End marker reached - clear all inputs and stop
        reset_hid_inputs();
        combo_state = COMBO_STATE_FINISHED;
        return;
    }

    // Apply digital button states
    hid_report[2] = s->btn2;
    hid_report[3] = s->btn3;
    hid_report[4] = s->btn4;

    // Apply stick positions
    hid_report[6] = s->left_x;
    hid_report[7] = s->left_y;
    hid_report[8] = s->right_x;
    hid_report[9] = s->right_y;

    // Mirror digital d-pad/button presses to their analog pressure bytes
    // so games that read analog pressure also register the input correctly.
    hid_report[14] = 0x00;  // D-Right pressure
    hid_report[15] = (s->btn2 & BTN_LEFT)  ? 0xFF : 0x00; // D-Left  pressure
    hid_report[16] = 0x00;  // D-Up    pressure
    hid_report[17] = (s->btn2 & BTN_DOWN)  ? 0xFF : 0x00; // D-Down  pressure
    hid_report[24] = (s->btn3 & BTN_CROSS) ? 0xFF : 0x00; // Cross   pressure

    // Advance to next step once current duration has elapsed
    if (now - combo_timer >= s->duration) {
        combo_step++;
        combo_timer = now;
    }
}
#endif // ENABLE_COMBO
// ====================================================================

int main(void) {
    board_init();
    tusb_init();

    // Initialise DS3 report defaults:
    // - Report ID 0x01
    // - All digital buttons released
    // - Analog sticks centered (0x80)
    // - All pressure bytes zero
    hid_report[0] = 0x01;
    hid_report[1] = 0x00;
    hid_report[2] = 0x00;
    hid_report[3] = 0x00;
    hid_report[4] = 0x00;
    hid_report[5] = 0x00;
    hid_report[6] = 0x80;  // Left  X
    hid_report[7] = 0x80;  // Left  Y
    hid_report[8] = 0x80;  // Right X
    hid_report[9] = 0x80;  // Right Y
    memset(hid_report + 10, 0x00, 39);

    uint32_t report_timer = 0;

    while (1) {
        tud_task(); // Drive the TinyUSB stack

        // Send HID input report at ~100 Hz (every 10 ms)
        if (board_millis() - report_timer > 10) {
            report_timer = board_millis();

#ifdef ENABLE_COMBO
            process_combo();
#endif

            if (tud_mounted() && tud_hid_n_ready(0)) {
                // Send full 49-byte DS3 report (report_id=0 so TinyUSB won't re-add it)
                tud_hid_n_report(0, 0, hid_report, sizeof(hid_report));
            }
        }
    }

    return 0;
}

// ====================================================================
// TinyUSB Callbacks
// ====================================================================

// Invoked when PS3 sends GET_REPORT — respond with Feature or Input data
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen) {
    (void) instance;

    if (report_type == HID_REPORT_TYPE_INPUT) {
        if (report_id == 0x01) {
            uint16_t len = (reqlen > 48) ? 48 : reqlen;
            memcpy(buffer, hid_report + 1, len);
            return len;
        }
        return 0;
    }

    if (report_type == HID_REPORT_TYPE_FEATURE) {
        const uint8_t* src       = NULL;
        uint16_t       src_len   = 0;
        uint8_t        first_byte = report_id; // default: TinyUSB already prepended this

        switch (report_id) {
            case 0x01:
                src        = report_01;
                src_len    = sizeof(report_01);
                first_byte = 0x00; // DS3 expects 0x00, not 0x01
                break;
            case 0xf2:
                src        = report_f2;
                src_len    = sizeof(report_f2);
                first_byte = 0xf2;
                break;
            case 0xf5: {
                static uint8_t f5_buf[64];
                memcpy(f5_buf, report_f5, 64);
                if (reply_f5 == 0) {
                    reply_f5 = 1; // First request: advertise dummy address
                } else {
                    memcpy(f5_buf + 2, masterBdaddr, 6); // Subsequent: use stored master addr
                }
                src        = f5_buf;
                src_len    = 64;
                first_byte = 0x01;
                break;
            }
            case 0xef: {
                static uint8_t ef_buf[64];
                memcpy(ef_buf, report_ef, 64);
                ef_buf[7] = byte_6_ef;
                src        = ef_buf;
                src_len    = 64;
                first_byte = 0x00;
                break;
            }
            case 0xf8: {
                static uint8_t f8_buf[64];
                memcpy(f8_buf, report_f8, 64);
                f8_buf[7] = byte_6_ef;
                src        = f8_buf;
                src_len    = 64;
                first_byte = 0x00;
                break;
            }
            case 0xf7:
                src        = report_f7;
                src_len    = sizeof(report_f7);
                first_byte = 0x01;
                break;
            default:
                return 0;
        }

        // TinyUSB has already written report_id to buffer[-1].
        // Overwrite it with the byte Sony's protocol actually expects.
        buffer[-1] = first_byte;

        uint16_t len = (reqlen > (uint16_t)(src_len - 1)) ? (uint16_t)(src_len - 1) : reqlen;
        memcpy(buffer, src + 1, len);
        return len;
    }

    return 0;
}

// Invoked when PS3 sends SET_REPORT — store Bluetooth pairing data
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;

    if (report_type == HID_REPORT_TYPE_FEATURE) {
        switch (report_id) {
            case 0xf5:
                if (bufsize >= 8) {
                    memcpy(masterBdaddr, buffer + 2, 6); // Store PS3's Bluetooth MAC
                }
                break;
            case 0xef:
                if (bufsize >= 7) {
                    byte_6_ef = buffer[6];
                }
                break;
            default:
                break;
        }
    }
}
