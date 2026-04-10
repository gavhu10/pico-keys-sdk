#include <M5Unified.h>
#include "cardputer_io.h"

extern "C" { 

void init_cardputer_hw() {
    auto cfg = M5.config();
    M5.begin();

    M5.Display.begin();
    M5.Display.setRotation(1);
    M5.Display.setTextColor(BLUE);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(1);

    M5.Display.fillRect(10, M5.Display.height() / 2 - 15,
                        M5.Display.width() - 20, 30, BLUE);
    M5.Display.drawString("BLE KEYBOARD", M5.Display.width() / 2,
                          M5.Display.height() / 2);
}

} 