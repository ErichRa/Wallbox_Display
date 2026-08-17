#pragma once

/*---------------------------------------------------------------
 * CrowPanel DIS08070H V3.0 - ESP32-S3 USB/I2C pad workaround
 *
 * The board uses GPIO19/20 for the GT911 touch I2C bus:
 *   GPIO19 = SDA
 *   GPIO20 = SCL
 *
 * On the ESP32-S3 the same pads are also used by the built-in
 * USB Serial/JTAG peripheral. With the current Arduino-ESP32 / IDF
 * stack this can leave the pads claimed by USB and cause GT911
 * transactions to fail with:
 *
 *   ESP_ERR_INVALID_STATE
 *   i2cWriteReadNonStop()
 *
 * Releasing the USB pad function before Wire1.begin() fixes the
 * problem on this CrowPanel V3.0. Keep this call before any I2C
 * initialization for the touch controller.
 *--------------------------------------------------------------*/

#include "driver/gpio.h"
#include "soc/usb_serial_jtag_struct.h"

static inline void crowpanel_release_usb_pads_for_i2c(int sda_pin, int scl_pin)
{
    USB_SERIAL_JTAG.conf0.dp_pullup = 0;
    USB_SERIAL_JTAG.conf0.usb_pad_enable = 0;
    USB_SERIAL_JTAG.conf0.pad_pull_override = 1;

    gpio_reset_pin(static_cast<gpio_num_t>(sda_pin));
    gpio_reset_pin(static_cast<gpio_num_t>(scl_pin));
    gpio_set_pull_mode(static_cast<gpio_num_t>(sda_pin), GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(static_cast<gpio_num_t>(scl_pin), GPIO_PULLUP_ONLY);
}
