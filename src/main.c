/*
 * This file is part of the Pico Keys SDK distribution (https://github.com/polhenarejos/pico-keys-sdk).
 * Copyright (c) 2022 Pol Henarejos.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include "pico_keys.h"

#if !defined(ENABLE_EMULATION)
#include "tusb.h"
#endif
#if defined(ENABLE_EMULATION)
#include "emulation.h"
#elif defined(ESP_PLATFORM)
#include "driver/gpio.h"
#include "rom/gpio.h"
#include "tinyusb.h"
#include "esp_efuse.h"
#define BOOT_PIN GPIO_NUM_0
#endif

#include "random.h"
#include "hwrng.h"
#include "apdu.h"
#include "usb.h"
#include "io/cardputer_io.h"
#include "mbedtls/sha256.h"

extern void init_otp_files(void);

app_t apps[16];
uint8_t num_apps = 0;

app_t *current_app = NULL;

const uint8_t *ccid_atr = NULL;

bool app_exists(const uint8_t *aid, size_t aid_len) {
    for (int a = 0; a < num_apps; a++) {
        if (aid_len >= apps[a].aid[0] && !memcmp(apps[a].aid + 1, aid, apps[a].aid[0])) {
            return true;
        }
    }
    return false;
}

int register_app(int (*select_aid)(app_t *, uint8_t), const uint8_t *aid) {
    if (app_exists(aid + 1, aid[0])) {
        return 1;
    }
    if (num_apps < sizeof(apps) / sizeof(app_t)) {
        apps[num_apps].select_aid = select_aid;
        apps[num_apps].aid = aid;
        num_apps++;
        return 1;
    }
    return 0;
}

int select_app(const uint8_t *aid, size_t aid_len) {
    if (current_app && current_app->aid && (current_app->aid + 1 == aid || (aid_len >= current_app->aid[0] && !memcmp(current_app->aid + 1, aid, current_app->aid[0])))) {
        current_app->select_aid(current_app, 0);
        return PICOKEY_OK;
    }
    for (int a = 0; a < num_apps; a++) {
        if (aid_len >= apps[a].aid[0] && !memcmp(apps[a].aid + 1, aid, apps[a].aid[0])) {
            if (current_app) {
                if (current_app->aid && aid_len >= current_app->aid[0] && !memcmp(current_app->aid + 1, aid, current_app->aid[0])) {
                    current_app->select_aid(current_app, 1);
                    return PICOKEY_OK;
                }
                if (current_app->unload) {
                    current_app->unload();
                }
            }
            current_app = &apps[a];
            if (current_app->select_aid(current_app, 1) == PICOKEY_OK) {
                return PICOKEY_OK;
            }
        }
    }
    return PICOKEY_ERR_FILE_NOT_FOUND;
}

int (*button_pressed_cb)(uint8_t) = NULL;

static void execute_tasks(void);

static bool req_button_pending = false;

bool is_req_button_pending(void) {
    return req_button_pending;
}

bool cancel_button = false;

#ifdef _MSC_VER
#include <windows.h>
struct timezone
{
    __int32  tz_minuteswest; /* minutes W of Greenwich */
    bool  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    (void)tzp;
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}

#endif
#if !defined(ENABLE_EMULATION)
static bool picok_board_button_read(void) {
    int boot_state = gpio_get_level(BOOT_PIN);
    return boot_state == 0;
}
bool button_pressed_state = false;
uint32_t button_pressed_time = 0;
uint8_t button_press = 0;
bool wait_button(void) {
    return wait_for_keypress();
}

__attribute__((weak)) int picokey_init(void) {
    return 0;
}

#endif

bool set_rtc = false;

bool has_set_rtc(void) {
    return set_rtc;
}

void set_rtc_time(time_t t) {
    struct timeval tv = {.tv_sec = t, .tv_usec = 0};
    settimeofday(&tv, NULL);
    set_rtc = true;
}

time_t get_rtc_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

struct apdu apdu;

static void init_rtc(void) {
}

static void execute_tasks(void)
{
    usb_task();
    led_blinking_task();
}

static void core0_loop(void *arg) {
    (void)arg;
    while (1) {
        execute_tasks();
        hwrng_task();
        do_flash();
#ifndef ENABLE_EMULATION
        if (button_pressed_cb && board_millis() > 1000 && !is_busy()) { // wait 1 second to boot up
            bool current_button_state = picok_board_button_read();
            if (current_button_state != button_pressed_state) {
                if (current_button_state == false) { // unpressed
                    if (button_pressed_time == 0 || button_pressed_time + 1000 > board_millis()) {
                        button_press++;
                    }
                    button_pressed_time = board_millis();
                }
                button_pressed_state = current_button_state;
            }
            if (button_pressed_time > 0 && button_press > 0 && button_pressed_time + 1000 < board_millis() && button_pressed_state == false) {
                if (button_pressed_cb != NULL) {
                    (*button_pressed_cb)(button_press);
                }
                button_pressed_time = button_press = 0;
            }
        }
#endif
    vTaskDelay(pdMS_TO_TICKS(10));
    }
}

char pico_serial_str[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
uint8_t pico_serial_hash[32];
pico_unique_board_id_t pico_serial;
#define pico_get_unique_board_id(a) do { uint32_t value; esp_efuse_read_block(EFUSE_BLK1, &value, 0, 32); memcpy((uint8_t *)(a), &value, sizeof(uint32_t)); esp_efuse_read_block(EFUSE_BLK1, &value, 32, 32); memcpy((uint8_t *)(a)+4, &value, sizeof(uint32_t)); } while(0)
extern tinyusb_config_t tusb_cfg;
extern const uint8_t desc_config[];
TaskHandle_t hcore0 = NULL, hcore1 = NULL;
int app_main(void) {
    pico_get_unique_board_id(&pico_serial);
    memset(pico_serial_str, 0, sizeof(pico_serial_str));
    for (size_t i = 0; i < sizeof(pico_serial); i++) {
        snprintf(&pico_serial_str[2 * i], 3, "%02X", pico_serial.id[i]);
    }
    mbedtls_sha256(pico_serial.id, sizeof(pico_serial.id), pico_serial_hash, false);

#ifndef ENABLE_EMULATION


#else
    emul_init("127.0.0.1", 35963);
#endif

    random_init();

    init_otp_files();

    low_flash_init();

    scan_flash();

    init_rtc();

#ifndef ENABLE_EMULATION
    phy_init();
#endif

    led_init();

    usb_init();

    init_cardputer_hw();

#ifndef ENABLE_EMULATION
    gpio_pad_select_gpio(BOOT_PIN);
    gpio_set_direction(BOOT_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_dis(BOOT_PIN);

    tusb_cfg.string_descriptor[3] = pico_serial_str;
    if (phy_data.usb_product_present) {
        tusb_cfg.string_descriptor[2] = phy_data.usb_product;
    }
    static char tmps[4][32];
    for (int i = 4; i < tusb_cfg.string_descriptor_count; i++) {
        strlcpy(tmps[i-4], tusb_cfg.string_descriptor[2], sizeof(tmps[0]));
        strlcat(tmps[i-4], " ", sizeof(tmps[0]));
        strlcat(tmps[i-4], tusb_cfg.string_descriptor[i], sizeof(tmps[0]));
        tusb_cfg.string_descriptor[i] = tmps[i-4];
    }
    tusb_cfg.configuration_descriptor = desc_config;

    tinyusb_driver_install(&tusb_cfg);
#endif

#ifndef ENABLE_EMULATION
    picokey_init();
#endif

    xTaskCreatePinnedToCore(core0_loop, "core0", 4096*ITF_TOTAL*2, NULL, CONFIG_TINYUSB_TASK_PRIORITY - 1, &hcore0, ESP32_CORE0);


    return 0;
}
