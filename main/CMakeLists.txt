# ~/dev/cpp/pico-fido/pico-keys-sdk/main/CMakeLists.txt

# 1. Explicitly list only the compatible source files
set(SOURCES
    "app_main.c"
    "apdu.c"
    "asn1.c"
    "crypto_utils.c"
    "eac.c"
    "rescue.c"
    "version.c"
    "fs/file.c"
    "fs/files.c"
    "fs/flash.c"
    "fs/low_flash.c"
    "fs/otp.c"
    "fs/phy.c"
    "io/cardputer_io.cpp"
    "led/led.c"
    "led/led_neopixel.cpp"
    "led/led_pico.c"
    "rng/hwrng.c"
    "rng/random.c"
    "usb/usb.c"
    "usb/usb_descriptors.c"
    "usb/ccid/ccid.c"
)

# 2. Register the component with ESP-IDF
idf_component_register(
    SRCS "${SOURCES}"
    INCLUDE_DIRS 
        "." 
        "fs" 
        "io" 
        "led" 
        "rng" 
        "usb" 
        "usb/ccid" 
        "../mbedtls/include"
    PRIV_REQUIRES 
        esp_tinyusb 
        esp_driver_gpio 
        nvs_flash 
        esp_rom 
        bootloader_support 
        m5unified
)

# Add USB_ITF_CCID=1 so tusb_config.h enables the Vendor driver
target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    ESP_PLATFORM=1 
    USB_ITF_CCID=1
)