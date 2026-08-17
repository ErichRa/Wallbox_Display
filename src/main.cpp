/*---------------------------------------------------------------
 * Elecrow DIS08070H V3.0 - stable PlatformIO70 display pipeline
 * + WiFi / MQTT test for Node-RED
 *--------------------------------------------------------------*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lvgl.h>
#include "ui.h"
#include "config.h"

class LGFX : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_RGB bus;
    lgfx::Panel_RGB panel;

    LGFX()
    {
        auto bus_config = bus.config();
        bus_config.panel = &panel;
        bus_config.pin_d0 = GPIO_NUM_15;
        bus_config.pin_d1 = GPIO_NUM_7;
        bus_config.pin_d2 = GPIO_NUM_6;
        bus_config.pin_d3 = GPIO_NUM_5;
        bus_config.pin_d4 = GPIO_NUM_4;
        bus_config.pin_d5 = GPIO_NUM_9;
        bus_config.pin_d6 = GPIO_NUM_46;
        bus_config.pin_d7 = GPIO_NUM_3;
        bus_config.pin_d8 = GPIO_NUM_8;
        bus_config.pin_d9 = GPIO_NUM_16;
        bus_config.pin_d10 = GPIO_NUM_1;
        bus_config.pin_d11 = GPIO_NUM_14;
        bus_config.pin_d12 = GPIO_NUM_21;
        bus_config.pin_d13 = GPIO_NUM_47;
        bus_config.pin_d14 = GPIO_NUM_48;
        bus_config.pin_d15 = GPIO_NUM_45;
        bus_config.pin_henable = GPIO_NUM_41;
        bus_config.pin_vsync = GPIO_NUM_40;
        bus_config.pin_hsync = GPIO_NUM_39;
        bus_config.pin_pclk = GPIO_NUM_0;
        bus_config.freq_write = 24000000;
        bus_config.hsync_polarity = 0;
        bus_config.hsync_front_porch = 40;
        bus_config.hsync_pulse_width = 48;
        bus_config.hsync_back_porch = 40;
        bus_config.vsync_polarity = 0;
        bus_config.vsync_front_porch = 1;
        bus_config.vsync_pulse_width = 31;
        bus_config.vsync_back_porch = 13;
        bus_config.pclk_active_neg = 1;
        bus_config.de_idle_high = 0;
        bus_config.pclk_idle_high = 0;
        bus.config(bus_config);

        auto panel_config = panel.config();
        panel_config.memory_width = 800;
        panel_config.memory_height = 480;
        panel_config.panel_width = 800;
        panel_config.panel_height = 480;
        panel_config.offset_x = 0;
        panel_config.offset_y = 0;
        panel.config(panel_config);

        panel.setBus(&bus);
        setPanel(&panel);
    }
};

LGFX lcd;
#include "touch.h"
int led = 0;

namespace
{
constexpr uint8_t BACKLIGHT_PIN = 2;
constexpr uint8_t RELAY_PIN = 38;
constexpr uint32_t SCREEN_WIDTH = 800;
constexpr uint32_t SCREEN_HEIGHT = 480;

WiFiClient wifi_client;
PubSubClient mqtt(wifi_client);
uint32_t last_wifi_try = 0;
uint32_t last_mqtt_try = 0;
int last_led_sent = -1;

uint32_t tick_get() { return millis(); }

void display_flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixel_map)
{
    (void)area;
    if(!lcd.bus.presentFrameBuffer(pixel_map)) Serial.println("LovyanGFX VSYNC frame switch timeout");
    lv_display_flush_ready(display);
}

void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->state = LV_INDEV_STATE_RELEASED;
    if(touch_touched()) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
    }
}

void set_power_label(float watts)
{
    char text[24];
    snprintf(text, sizeof(text), "%.2f kW", watts / 1000.0f);
    lv_label_set_text(ui_TempLabel, text);
}

void set_status_label(bool connected, bool charging, float current)
{
    char text[32];
    if(charging) snprintf(text, sizeof(text), "LÄDT %.1f A", current);
    else if(connected) snprintf(text, sizeof(text), "VERBUNDEN");
    else snprintf(text, sizeof(text), "BEREIT");
    lv_label_set_text(ui_HumiLabel, text);
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    if(strcmp(topic, TOPIC_STATUS) != 0) return;
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if(err) {
        Serial.printf("MQTT JSON Fehler: %s\n", err.c_str());
        return;
    }
    const bool connected = doc["connected"] | false;
    const bool charging = doc["charging"] | false;
    const float power = doc["power"] | 0.0f;
    const float current = doc["current"] | 0.0f;
    set_power_label(power);
    set_status_label(connected, charging, current);
    Serial.printf("Status: connected=%d charging=%d power=%.0fW current=%.1fA\n", connected, charging, power, current);
}

void wifi_service()
{
    if(WiFi.status() == WL_CONNECTED) return;
    const uint32_t now = millis();
    if(now - last_wifi_try < 10000) return;
    last_wifi_try = now;
    Serial.printf("WLAN verbinden mit %s ...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void publish_presence()
{
    const String ip = WiFi.localIP().toString();
    const String rssi = String(WiFi.RSSI());
    mqtt.publish(TOPIC_ONLINE, "1", true);
    mqtt.publish(TOPIC_IP, ip.c_str(), true);
    mqtt.publish(TOPIC_RSSI, rssi.c_str(), true);
    Serial.printf("WLAN verbunden: IP=%s RSSI=%s dBm\n", ip.c_str(), rssi.c_str());
}

void mqtt_service()
{
    if(WiFi.status() != WL_CONNECTED) return;
    if(!mqtt.connected()) {
        const uint32_t now = millis();
        if(now - last_mqtt_try < 5000) return;
        last_mqtt_try = now;
        Serial.printf("MQTT verbinden mit %s:%d ...\n", MQTT_HOST, MQTT_PORT);
        bool ok = false;
        if(strlen(MQTT_USER) > 0) {
            ok = mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD, TOPIC_ONLINE, 0, true, "0");
        } else {
            ok = mqtt.connect(MQTT_CLIENT_ID, TOPIC_ONLINE, 0, true, "0");
        }
        if(ok) {
            Serial.println("MQTT verbunden");
            mqtt.subscribe(TOPIC_STATUS);
            publish_presence();
        } else {
            Serial.printf("MQTT Fehler rc=%d\n", mqtt.state());
        }
    }
    mqtt.loop();
}

void publish_button_command()
{
    if(led == last_led_sent) return;
    last_led_sent = led;
    if(!mqtt.connected()) return;
    if(led == 1) {
        mqtt.publish(TOPIC_CMD_START, "1");
        Serial.println("MQTT -> START");
    } else {
        mqtt.publish(TOPIC_CMD_STOP, "1");
        Serial.println("MQTT -> STOP");
    }
}
}

void setup()
{
    Serial.begin(115200);
    delay(300);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // Start I2C once and keep it active. Reinitializing Wire1 after lcd.begin()
    // is intentionally avoided while isolating the GT911 INVALID_STATE issue.
    Wire1.begin(19, 20);
    Wire1.setClock(400000);

    if(!lcd.begin()) {
        Serial.println("lcd.begin() failed");
        return;
    }
    delay(200);

    lv_init();
    lv_tick_set_cb(tick_get);
    touch_init();

    lv_color_t *frame_buffer_0 = reinterpret_cast<lv_color_t *>(lcd.bus.getFrameBuffer(0));
    lv_color_t *frame_buffer_1 = reinterpret_cast<lv_color_t *>(lcd.bus.getFrameBuffer(1));
    Serial.printf("RGB frame buffers: %p, %p\n", frame_buffer_0, frame_buffer_1);
    if(frame_buffer_0 == nullptr || frame_buffer_1 == nullptr) {
        Serial.println("RGB double frame buffer allocation failed");
        return;
    }

    lv_display_t *display = lv_display_create(lcd.width(), lcd.height());
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, display_flush);
    lv_display_set_buffers(display, frame_buffer_0, frame_buffer_1,
                           SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_FULL);

    lv_indev_t *touchpad = lv_indev_create();
    lv_indev_set_type(touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touchpad, touchpad_read);

    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, HIGH);
    ui_init();
    lv_label_set_text(ui_TempLabel, "--.-- kW");
    lv_label_set_text(ui_HumiLabel, "MQTT WARTET");

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqtt_callback);
    mqtt.setBufferSize(1024);
    wifi_service();
}

void loop()
{
    wifi_service();
    mqtt_service();
    publish_button_command();
    digitalWrite(RELAY_PIN, led == 1 ? HIGH : LOW);
    lv_timer_handler();
    delay(10);
}
