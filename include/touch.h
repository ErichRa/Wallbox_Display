#ifndef TOUCH_H
#define TOUCH_H

/*---------------------------------------------------------------
 * GT911 touch interface for Elecrow DIS08070H V3.0.
 *
 * GPIO19 = SDA
 * GPIO20 = SCL
 * GPIO38 = GT911 INT
 *
 * The PCA9557 is deliberately not touched here. The GT911 is
 * allowed to start with the board hardware reset/pull-up circuit.
 *--------------------------------------------------------------*/

#include <Wire.h>

constexpr int TOUCH_SCL = 20;
constexpr int TOUCH_SDA = 19;
constexpr int TOUCH_INT = 38;
constexpr uint8_t GT911_ADDRESS = 0x5D;
constexpr uint16_t GT911_POINT_INFO = 0x814E;
constexpr uint16_t GT911_FIRST_POINT = 0x814F;

int touch_last_x = 0;
int touch_last_y = 0;
bool touch_is_pressed = false;

void touch_init()
{
    // GPIO38 belongs to the GT911 interrupt line. Never drive it as a relay.
    pinMode(TOUCH_INT, INPUT);
    delay(100);
}

static bool gt911_read(uint16_t reg, uint8_t *data, size_t size)
{
    Wire1.beginTransmission(GT911_ADDRESS);
    Wire1.write(static_cast<uint8_t>(reg >> 8));
    Wire1.write(static_cast<uint8_t>(reg));
    if(Wire1.endTransmission(false) != 0) return false;
    if(Wire1.requestFrom(GT911_ADDRESS, size) != size) return false;

    for(size_t index = 0; index < size; ++index) {
        data[index] = Wire1.read();
    }
    return true;
}

static void gt911_clear_status()
{
    Wire1.beginTransmission(GT911_ADDRESS);
    Wire1.write(static_cast<uint8_t>(GT911_POINT_INFO >> 8));
    Wire1.write(static_cast<uint8_t>(GT911_POINT_INFO));
    Wire1.write(0);
    Wire1.endTransmission();
}

bool touch_touched()
{
    static uint32_t last_sample = 0;
    const uint32_t now = millis();
    if(now - last_sample < 30) return touch_is_pressed;
    last_sample = now;

    uint8_t status = 0;
    if(!gt911_read(GT911_POINT_INFO, &status, 1)) {
        touch_is_pressed = false;
        return false;
    }

    if((status & 0x80) == 0) {
        touch_is_pressed = false;
        return false;
    }

    const uint8_t point_count = status & 0x0F;
    touch_is_pressed = point_count > 0 && point_count <= 5;

    if(touch_is_pressed) {
        uint8_t point[7];
        if(gt911_read(GT911_FIRST_POINT, point, sizeof(point))) {
            const uint16_t raw_x = point[1] | (static_cast<uint16_t>(point[2]) << 8);
            const uint16_t raw_y = point[3] | (static_cast<uint16_t>(point[4]) << 8);
            touch_last_x = constrain(raw_x, 0, lcd.width() - 1);
            touch_last_y = constrain(raw_y, 0, lcd.height() - 1);
        }
    }

    gt911_clear_status();
    return touch_is_pressed;
}

#endif
