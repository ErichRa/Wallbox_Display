#ifndef TOUCH_H
#define TOUCH_H

/*---------------------------------------------------------------
 * GT911 touch interface
 * Control the PCA9557 reset lines and read touch points over I2C.
 *--------------------------------------------------------------*/

#include <Wire.h>

constexpr int TOUCH_SCL = 20;
constexpr int TOUCH_SDA = 19;
constexpr int TOUCH_INT = -1;
constexpr int TOUCH_RST = -1;
constexpr int TOUCH_MAP_X1 = 800;
constexpr int TOUCH_MAP_X2 = 0;
constexpr int TOUCH_MAP_Y1 = 480;
constexpr int TOUCH_MAP_Y2 = 0;
constexpr uint8_t PCA9557_ADDRESS = 0x18;
constexpr uint8_t PCA9557_OUTPUT = 0x01;
constexpr uint8_t PCA9557_CONFIG = 0x03;
constexpr uint8_t GT911_ADDRESS = 0x5D;
constexpr uint16_t GT911_POINT_INFO = 0x814E;
constexpr uint16_t GT911_FIRST_POINT = 0x814F;

// Latest touch position in display coordinates.
int touch_last_x = 0;
int touch_last_y = 0;

// Cached contact state used to rate-limit GT911 polling.
bool touch_is_pressed = false;

static void pca9557_write(uint8_t reg, uint8_t value)
{
    Wire1.beginTransmission(PCA9557_ADDRESS);
    Wire1.write(reg);
    Wire1.write(value);
    Wire1.endTransmission();
}

static uint8_t pca9557_read(uint8_t reg)
{
    Wire1.beginTransmission(PCA9557_ADDRESS);
    Wire1.write(reg);
    Wire1.endTransmission();
    Wire1.requestFrom(PCA9557_ADDRESS, static_cast<uint8_t>(1));
    return Wire1.read();
}

static void pca9557_set_mode(uint8_t pin, bool input)
{
    uint8_t config = pca9557_read(PCA9557_CONFIG);
    config = input ? config | (1U << pin) : config & ~(1U << pin);
    pca9557_write(PCA9557_CONFIG, config);
}

static void pca9557_set_state(uint8_t pin, bool high)
{
    uint8_t output = pca9557_read(PCA9557_OUTPUT);
    output = high ? output | (1U << pin) : output & ~(1U << pin);
    pca9557_write(PCA9557_OUTPUT, output);
}

void touch_init()
{
    pca9557_write(PCA9557_CONFIG, 0xFF);
    pca9557_set_mode(0, false);
    pca9557_set_mode(1, false);
    pca9557_set_state(0, false);
    pca9557_set_state(1, false);
    delay(20);
    pca9557_set_state(0, true);
    delay(100);
    pca9557_set_mode(1, true);
}

static bool gt911_read(uint16_t reg, uint8_t *data, size_t size)
{
    Wire1.beginTransmission(GT911_ADDRESS);
    Wire1.write(static_cast<uint8_t>(reg >> 8));
    Wire1.write(static_cast<uint8_t>(reg));
    if(Wire1.endTransmission(false) != 0) return false;
    if(Wire1.requestFrom(GT911_ADDRESS, size) != size) return false;

    for(size_t index = 0; index < size; ++index) data[index] = Wire1.read();
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
    if(!gt911_read(GT911_POINT_INFO, &status, 1)) return touch_is_pressed;
    if((status & 0x80) == 0) return touch_is_pressed;

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
