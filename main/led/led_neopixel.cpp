#include "pico_keys.h"

#ifdef ESP_PLATFORM
#include <M5Unified.h>

#ifdef __cplusplus
extern "C" {
#endif

void led_driver_init_neopixel(void) {
}

void led_driver_color_neopixel(uint8_t color, uint32_t led_brightness, float progress) {
    static const uint8_t colors[][3] = {
        {0, 0, 0},      // off
        {255, 0, 0},    // red
        {0, 255, 0},    // green
        {0, 0, 255},    // blue
        {255, 255, 0},  // yellow
        {255, 0, 255},  // magenta
        {0, 255, 255},  // cyan
        {255, 255, 255} // white
    };

    if (!(phy_data.opts & PHY_OPT_DIMM)) {
        progress = progress >= 0.5 ? 1 : 0;
    }

    uint32_t led_phy_btness = phy_data.led_brightness_present ? phy_data.led_brightness : MAX_BTNESS;
    float brightness = ((float)led_brightness / MAX_BTNESS) * ((float)led_phy_btness / MAX_BTNESS) * progress;

    uint8_t r = (uint8_t)(colors[color][0] * brightness);
    uint8_t g = (uint8_t)(colors[color][1] * brightness);
    uint8_t b = (uint8_t)(colors[color][2] * brightness);

    // M5Unified's universal LED setter
    // This works for the Cardputer's built-in RGB LED automatically
    M5.Power.setLed(M5.Lcd.color565(r, g, b)); 
}

led_driver_t led_driver_neopixel = {
    .init = led_driver_init_neopixel,
    .set_color = led_driver_color_neopixel,
};

#ifdef __cplusplus
}
#endif

#endif