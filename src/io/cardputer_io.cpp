#include <M5Unified.h>
#include "cardputer_io.h"

static bool button_read(void) {
    int boot_state = gpio_get_level((gpio_num_t)0);
    return boot_state == 0;
}


extern "C"
{

    void init_cardputer_hw()
    {
        auto cfg = M5.config();
        M5.begin(cfg);

        M5.Display.drawString("Pico Key Cardputer", M5.Display.width() / 2,
                              M5.Display.height() / 2);

    }

    bool wait_for_keypress()
    {
        M5.Display.drawString("Press G0.. ", M5.Display.width() / 2,
                              M5.Display.height() / 2 + 20);
        while (true)
        {
            if (button_read())
            {
                M5.Display.drawString("Confirmed!", M5.Display.width() / 2,
                                      M5.Display.height() / 2 + 20);
                return false;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}