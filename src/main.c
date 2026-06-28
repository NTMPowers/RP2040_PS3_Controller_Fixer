#include "bsp/board.h"
#include "tusb.h"
#include <string.h>
#include "hardware/pio.h"
#include "hardware/clocks.h"

// ====================================================================
// WS2812 (NeoPixel) Inline PIO Driver for RP2040 Zero (GPIO 16)
// ====================================================================
#define WS2812_PIN 16
#define LED_INTENSITY 5

static const uint16_t ws2812_program_instructions[] = {
    0x6221, //  0: out    x, 1            side 0 [2]
    0x1123, //  1: jmp    !x, 3           side 1 [1]
    0x1400, //  2: jmp    0               side 1 [4]
    0xa442, //  3: nop                    side 0 [4]
};
static const struct pio_program ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length = 4,
    .origin = -1,
};

static inline pio_sm_config ws2812_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset, offset + 3);
    sm_config_set_sideset(&c, 1, false, false);
    return c;
}

static void ws2812_init(void) {
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, WS2812_PIN);
    pio_gpio_init(pio, WS2812_PIN);
    pio_sm_set_consecutive_pindirs(pio, 0, WS2812_PIN, 1, true);
    sm_config_set_out_shift(&c, false, true, 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    float div = clock_get_hz(clk_sys) / (800000.0f * 10.0f);
    sm_config_set_clkdiv(&c, div);
    pio_sm_init(pio, 0, offset, &c);
    pio_sm_set_enabled(pio, 0, true);
}

static void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | b;
    pio_sm_put_blocking(pio0, 0, color << 8);
}

// ====================================================================
// DS3 Input Report buffer & Feature Reports
// ====================================================================
static uint8_t hid_report[49] = {0};
static uint8_t masterBdaddr[6] = {0x00, 0x03, 0x50, 0x81, 0xd8, 0x01};
static uint8_t byte_6_ef = 0x00;
static uint8_t reply_f5  = 0;

static const uint8_t report_01[] = {
    0x00, 0x01, 0x04, 0x00, 0x07, 0x0c, 0x01, 0x02,0x18, 0x18, 0x18, 0x18, 0x09, 0x0a, 0x10, 0x11,0x12, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};
static const uint8_t report_f2[] = {
    0xf2, 0xff, 0xff, 0x00,0x01, 0x02, 0x03, 0x04, 0x05, 0x06,0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};
static const uint8_t report_f5[] = {
    0x01, 0x00,0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,0xff, 0xf7, 0x00, 0x03, 0x50, 0x81, 0xd8, 0x01,0x8a, 0x13, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,0x02, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x04,0x04, 0x04, 0x04, 0x00, 0x00, 0x04, 0x00, 0x01,0x02, 0x07, 0x00, 0x17, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};
static const uint8_t report_ef[] = {
    0x00, 0xef, 0x04, 0x00, 0x07, 0x03, 0x01, 0xb0,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x02, 0x6b, 0x02, 0x68, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};
static const uint8_t report_f8[] = {
    0x00, 0x01, 0x00, 0x00, 0x07, 0x03, 0x01, 0xb0,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x02, 0x6b, 0x02, 0x68, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};
static const uint8_t report_f7[] = {
    0x01, 0x04, 0xc4, 0x02, 0xd6, 0x01, 0xee, 0xff,0x14, 0x13, 0x01, 0x02, 0xc4, 0x01, 0xd6, 0x00,0x00, 0x02, 0x02, 0x02, 0x00, 0x03, 0x00, 0x00,0x02, 0x00, 0x00, 0x02, 0x62, 0x01, 0x02, 0x01,0x5e, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};

// ====================================================================
// Button & Combo Definitions
// ====================================================================
#define BTN_SELECT 0x01
#define BTN_L3     0x02
#define BTN_L1     0x04
#define BTN_R1     0x08

#define BTN_LEFT   0x80 
#define BTN_CROSS  0x40 

static bool boot_btn_prev = false;
static bool manual_combo_active = false;
static uint32_t manual_combo_timer = 0;

// ====================================================================
// STARTUP COMBO (PS3HEN)
// ====================================================================
#ifdef ENABLE_COMBO
#define COMBO_START_DELAY_SEC  30.0f

typedef struct {
    uint8_t btn2, btn3, btn4;
    uint8_t left_x, left_y, right_x, right_y;
    uint32_t duration;
} combo_step_t;

static const combo_step_t COMBO_STEPS[] = {
    { BTN_LEFT, 0x00, 0, 0x80, 0x80, 0x80, 0x80, 100 },
    { 0x00,     0x00, 0, 0x80, 0x80, 0x80, 0x80,  50 },
    { BTN_LEFT, 0x00, 0, 0x80, 0x80, 0x80, 0x80, 100 },
    { 0x00,     0x00, 0, 0x80, 0x80, 0x80, 0x80,  50 },
    { 0x00, BTN_CROSS, 0, 0x80, 0x80, 0x80, 0x80, 100 },
    { 0x00,     0x00, 0, 0x80, 0x80, 0x80, 0x80,   0 },
};

typedef enum { COMBO_STATE_DELAY, COMBO_STATE_RUNNING, COMBO_STATE_FINISHED } combo_state_t;
static combo_state_t combo_state = COMBO_STATE_DELAY;
static uint16_t combo_step = 0;
static uint32_t combo_timer = 0;

static void process_startup_combo(void) {
    uint32_t now = board_millis();
    if (combo_state == COMBO_STATE_DELAY) {
        if (now >= (uint32_t)(COMBO_START_DELAY_SEC * 1000.0f)) {
            combo_state = COMBO_STATE_RUNNING;
            combo_step = 0;
            combo_timer = now;
        }
        return;
    }
    if (combo_state == COMBO_STATE_FINISHED) return;
	
    const combo_step_t* s = &COMBO_STEPS[combo_step];
    if (s->duration == 0) {
        combo_state = COMBO_STATE_FINISHED;
        return;
    }

    if (!manual_combo_active) {
        hid_report[2] = s->btn2;
        hid_report[3] = s->btn3;
        hid_report[4] = s->btn4;
        hid_report[6] = s->left_x; hid_report[7] = s->left_y;
        hid_report[8] = s->right_x; hid_report[9] = s->right_y;
        hid_report[15] = (s->btn2 & BTN_LEFT) ? 0xFF : 0x00;
        hid_report[17] = 0x00;
        hid_report[24] = (s->btn3 & BTN_CROSS) ? 0xFF : 0x00;
    }

    if (now - combo_timer >= s->duration) {
        combo_step++;
        combo_timer = now;
    }
}
#endif 

// ====================================================================
// LED State Machine
// ====================================================================

typedef enum { LED_RED, LED_GREEN, LED_OFF } led_state_t;
static led_state_t led_state = LED_RED;
static uint32_t led_timer = 0;

static void update_led(void) {
    if (board_button_read()) {
        ws2812_set_color(0, 0, LED_INTENSITY);
        return;
    }

#ifdef ENABLE_COMBO
    if (combo_state == COMBO_STATE_RUNNING) {
        ws2812_set_color(0, 0, LED_INTENSITY);
        return;
    }
#endif

    switch (led_state) {
        case LED_RED:
            ws2812_set_color(LED_INTENSITY, 0, 0);
            if (tud_mounted()) {
                led_state = LED_GREEN;
                led_timer = board_millis();
            }
            break;
        case LED_GREEN:
            ws2812_set_color(0, LED_INTENSITY - 2, 0);
            if (board_millis() - led_timer >= 3000)
                led_state = LED_OFF;
            break;
        case LED_OFF:
            ws2812_set_color(0, 0, 0);
            break;
    }
}

int main(void) {
    board_init();
    ws2812_init(); 
    ws2812_set_color(LED_INTENSITY, 0, 0);
	tusb_init();

    hid_report[0] = 0x01; hid_report[1] = 0x00; hid_report[2] = 0x00; hid_report[3] = 0x00;
    hid_report[4] = 0x00; hid_report[5] = 0x00; hid_report[6] = 0x80; hid_report[7] = 0x80;
    hid_report[8] = 0x80; hid_report[9] = 0x80;
    memset(hid_report + 10, 0x00, 39);

    uint32_t report_timer = 0;

    while (1) {
        tud_task(); 
        update_led(); 

        bool boot_btn = board_button_read();
        if (boot_btn && !boot_btn_prev) {
            manual_combo_active = true;
            manual_combo_timer = board_millis();
        }
        boot_btn_prev = boot_btn;

        if (board_millis() - report_timer > 10) {
            report_timer = board_millis();
            
            // Reset report to default EVERY loop to prevent stuck buttons
            hid_report[2] = 0x00; hid_report[3] = 0x00; hid_report[4] = 0x00; hid_report[5] = 0x00;
            hid_report[6] = 0x80; hid_report[7] = 0x80; hid_report[8] = 0x80; hid_report[9] = 0x80;
            memset(hid_report + 14, 0x00, 12); 

#ifdef ENABLE_COMBO
            process_startup_combo();
#endif

            if (manual_combo_active) {
                hid_report[2] = BTN_SELECT | BTN_L3;
                hid_report[3] = BTN_L1 | BTN_R1;
                hid_report[18] = 0xFF; // L1 pressure
                hid_report[19] = 0xFF; // R1 pressure
                
                if (board_millis() - manual_combo_timer >= 500) {
                    manual_combo_active = false;
                }
            }

            if (tud_mounted() && tud_hid_n_ready(0)) {
                tud_hid_n_report(0, 0, hid_report, sizeof(hid_report));
            }
        }
    }
    return 0;
}

// ====================================================================
// TinyUSB Callbacks
// ====================================================================
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    if (report_type == HID_REPORT_TYPE_INPUT) {
        if (report_id == 0x01) { uint16_t len = (reqlen > 48) ? 48 : reqlen; memcpy(buffer, hid_report + 1, len); return len; }
        return 0;
    }
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        const uint8_t* src = NULL; uint16_t src_len = 0; uint8_t first_byte = report_id;
        switch (report_id) {
            case 0x01: src = report_01; src_len = sizeof(report_01); first_byte = 0x00; break;
            case 0xf2: src = report_f2; src_len = sizeof(report_f2); first_byte = 0xf2; break;
            case 0xf5: {
                static uint8_t f5_buf[64]; memcpy(f5_buf, report_f5, 64);
                if (reply_f5 == 0) { reply_f5 = 1; } else { memcpy(f5_buf + 2, masterBdaddr, 6); }
                src = f5_buf; src_len = 64; first_byte = 0x01; break;
            }
            case 0xef: {
                static uint8_t ef_buf[64]; memcpy(ef_buf, report_ef, 64); ef_buf[7] = byte_6_ef;
                src = ef_buf; src_len = 64; first_byte = 0x00; break;
            }
            case 0xf8: {
                static uint8_t f8_buf[64]; memcpy(f8_buf, report_f8, 64); f8_buf[7] = byte_6_ef;
                src = f8_buf; src_len = 64; first_byte = 0x00; break;
            }
            case 0xf7: src = report_f7; src_len = sizeof(report_f7); first_byte = 0x01; break;
            default: return 0;
        }
        buffer[-1] = first_byte;
        uint16_t len = (reqlen > (uint16_t)(src_len - 1)) ? (uint16_t)(src_len - 1) : reqlen;
        memcpy(buffer, src + 1, len);
        return len;
    }
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) buffer;
    (void) bufsize;
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        switch (report_id) {
            case 0xf5: if (bufsize >= 8) { memcpy(masterBdaddr, buffer + 2, 6); } break;
            case 0xef: if (bufsize >= 7) { byte_6_ef = buffer[6]; } break;
            default: break;
        }
    }
}