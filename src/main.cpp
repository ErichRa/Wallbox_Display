/*---------------------------------------------------------------
 * Elecrow DIS08070H V3.0 - Wallbox dashboard
 * Stable RGB/LVGL pipeline + WiFi/MQTT + GT911 touch
 *--------------------------------------------------------------*/

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <time.h>
#include <PubSubClient.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lvgl.h>
#include "ui.h"
#include "config.h"
#include "crowpanel_i2c_fix.h"

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
        bus_config.freq_write = 18000000;
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
int charge_mode_request = -1;
int set_current_request = -1;

namespace
{
constexpr uint8_t BACKLIGHT_PIN = 2;
constexpr uint32_t BACKLIGHT_PWM_FREQUENCY_HZ = 5000;
constexpr uint8_t BACKLIGHT_PWM_RESOLUTION_BITS = 8;
constexpr uint8_t BACKLIGHT_ACTIVE_PERCENT = 100;
constexpr uint8_t BACKLIGHT_DIMMED_PERCENT = 5;
constexpr uint32_t BACKLIGHT_DIM_DELAY_MS = 2UL * 60UL * 1000UL;
constexpr uint32_t BACKLIGHT_OFF_DELAY_MS = 10UL * 60UL * 1000UL;
constexpr uint32_t SCREEN_WIDTH = 800;
constexpr uint32_t SCREEN_HEIGHT = 480;
constexpr uint32_t CHARGE_MODE_CONFIRM_TIMEOUT_MS = 10000;
constexpr char TIMEZONE_EUROPE_BERLIN[] = "CET-1CEST,M3.5.0/2,M10.5.0/3";

enum class BacklightState : uint8_t
{
    Unknown,
    Active,
    Dimmed,
    Off
};

WiFiClient wifi_client;
PubSubClient mqtt(wifi_client);
WebServer web_server(80);
bool web_server_started = false;
bool ota_started = false;
bool ota_password_warning_printed = false;
uint32_t last_wifi_try = 0;
uint32_t last_mqtt_try = 0;
uint32_t last_clock_check_ms = 0;
int last_led_sent = -1;
bool time_sync_started = false;
bool backlight_pwm_ready = false;
bool suppress_touch_until_release = false;
uint32_t last_user_activity_ms = 0;
BacklightState backlight_state = BacklightState::Unknown;

lv_color_t *screen_frame_buffer = nullptr;

int pending_wallbox_status = -1;
int displayed_wallbox_status = -2;
int pending_charge_mode = -1;
int displayed_charge_mode = -2;
int requested_charge_mode = -1;
uint32_t charge_mode_request_started_ms = 0;
int pending_set_current_a = -1;
int displayed_set_current_a = -2;
float pending_power_watts = 0.0f;
float displayed_power_watts = -1.0f;
bool power_received = false;
float pending_current_amps = 0.0f;
float displayed_current_amps = -1.0f;
bool current_received = false;
char pending_charge_phase[16] = "off";
char displayed_charge_phase[16] = "";
bool charge_phase_received = false;
float pending_session_energy_kwh = 0.0f;
float displayed_session_energy_kwh = -1.0f;
bool session_energy_received = false;
float pending_total_energy_kwh = 0.0f;
float displayed_total_energy_kwh = -1.0f;
bool total_energy_received = false;
uint32_t pending_session_duration_s = 0;
uint32_t displayed_session_duration_s = 0xFFFFFFFFUL;
bool session_duration_received = false;
char pending_session_start[32] = "---";
char displayed_session_start[32] = "";
bool session_start_received = false;
char pending_session_end[32] = "---";
char displayed_session_end[32] = "";
bool session_end_received = false;

lv_obj_t *power_decimal_label = nullptr;
lv_obj_t *power_fraction_label = nullptr;
lv_obj_t *power_unit_label = nullptr;

uint32_t tick_get()
{
    return millis();
}

void set_backlight_percent(uint8_t percent)
{
    if(percent > 100) percent = 100;

    if(backlight_pwm_ready) {
        const uint32_t duty = (static_cast<uint32_t>(percent) * 255U + 50U) / 100U;
        ledcWrite(BACKLIGHT_PIN, duty);
    }
    else {
        digitalWrite(BACKLIGHT_PIN, percent > 0 ? HIGH : LOW);
    }
}

void set_backlight_state(BacklightState state)
{
    if(state == backlight_state) return;
    backlight_state = state;

    switch(state) {
        case BacklightState::Active:
            set_backlight_percent(BACKLIGHT_ACTIVE_PERCENT);
            break;
        case BacklightState::Dimmed:
            set_backlight_percent(BACKLIGHT_DIMMED_PERCENT);
            break;
        case BacklightState::Off:
            set_backlight_percent(0);
            break;
        case BacklightState::Unknown:
            break;
    }
}

void register_user_activity()
{
    last_user_activity_ms = millis();
    set_backlight_state(BacklightState::Active);
}

void backlight_service()
{
    const uint32_t inactive_ms = millis() - last_user_activity_ms;

    if(inactive_ms >= BACKLIGHT_OFF_DELAY_MS) {
        set_backlight_state(BacklightState::Off);
    }
    else if(inactive_ms >= BACKLIGHT_DIM_DELAY_MS) {
        set_backlight_state(BacklightState::Dimmed);
    }
    else {
        set_backlight_state(BacklightState::Active);
    }
}

void display_flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixel_map)
{
    (void)area;
    if(!lcd.bus.presentFrameBuffer(pixel_map)) {
        Serial.println("LovyanGFX VSYNC frame switch timeout");
    }
    lv_display_flush_ready(display);
}

void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->state = LV_INDEV_STATE_RELEASED;

    if(touch_touched()) {
        if(backlight_state == BacklightState::Off) {
            register_user_activity();
            suppress_touch_until_release = true;
        }
        else {
            register_user_activity();
        }

        if(suppress_touch_until_release) return;

        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
    }
    else {
        suppress_touch_until_release = false;
    }
}

static void put_u16_le(uint8_t *buffer, size_t offset, uint16_t value)
{
    buffer[offset] = static_cast<uint8_t>(value & 0xFFU);
    buffer[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFU);
}

static void put_u32_le(uint8_t *buffer, size_t offset, uint32_t value)
{
    buffer[offset] = static_cast<uint8_t>(value & 0xFFU);
    buffer[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFU);
    buffer[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFFU);
    buffer[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFFU);
}

void handle_screenshot()
{
    if(screen_frame_buffer == nullptr) {
        web_server.send(503, "text/plain", "Framebuffer nicht verfuegbar");
        return;
    }

    constexpr uint32_t row_bytes = SCREEN_WIDTH * 3U;
    constexpr uint32_t row_stride = (row_bytes + 3U) & ~3U;
    constexpr uint32_t image_size = row_stride * SCREEN_HEIGHT;
    constexpr uint32_t file_size = 54U + image_size;

    uint8_t bmp_header[54] = {};
    bmp_header[0] = 'B';
    bmp_header[1] = 'M';
    put_u32_le(bmp_header, 2, file_size);
    put_u32_le(bmp_header, 10, 54U);
    put_u32_le(bmp_header, 14, 40U);
    put_u32_le(bmp_header, 18, SCREEN_WIDTH);
    put_u32_le(bmp_header, 22, SCREEN_HEIGHT);
    put_u16_le(bmp_header, 26, 1U);
    put_u16_le(bmp_header, 28, 24U);
    put_u32_le(bmp_header, 34, image_size);
    put_u32_le(bmp_header, 38, 2835U);
    put_u32_le(bmp_header, 42, 2835U);

    WiFiClient client = web_server.client();
    client.printf("HTTP/1.1 200 OK\r\n"
                  "Content-Type: image/bmp\r\n"
                  "Content-Disposition: inline; filename=wallbox-screen.bmp\r\n"
                  "Content-Length: %lu\r\n"
                  "Cache-Control: no-store, no-cache, must-revalidate\r\n"
                  "Pragma: no-cache\r\n"
                  "Connection: close\r\n\r\n",
                  static_cast<unsigned long>(file_size));
    client.write(bmp_header, sizeof(bmp_header));

    static uint8_t line_buffer[row_stride];

    // BMP speichert positive Hoehen von unten nach oben. Der LVGL-Framebuffer
    // liegt zeilenweise von oben nach unten, deshalb werden die Zeilen umgekehrt.
    for(int y = static_cast<int>(SCREEN_HEIGHT) - 1; y >= 0; --y) {
        const uint16_t *source = reinterpret_cast<const uint16_t *>(screen_frame_buffer) +
                                 static_cast<size_t>(y) * SCREEN_WIDTH;

        for(uint32_t x = 0; x < SCREEN_WIDTH; ++x) {
            const uint16_t rgb565 = source[x];
            const uint8_t r5 = static_cast<uint8_t>((rgb565 >> 11) & 0x1FU);
            const uint8_t g6 = static_cast<uint8_t>((rgb565 >> 5) & 0x3FU);
            const uint8_t b5 = static_cast<uint8_t>(rgb565 & 0x1FU);

            const uint8_t r8 = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
            const uint8_t g8 = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
            const uint8_t b8 = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));

            const size_t offset = static_cast<size_t>(x) * 3U;
            line_buffer[offset] = b8;
            line_buffer[offset + 1] = g8;
            line_buffer[offset + 2] = r8;
        }

        for(uint32_t p = row_bytes; p < row_stride; ++p) {
            line_buffer[p] = 0;
        }

        if(client.write(line_buffer, row_stride) != row_stride) {
            Serial.println("Screenshot: Client-Verbindung abgebrochen");
            break;
        }
        yield();
    }

    client.flush();
    client.stop();
    Serial.println("HTTP Screenshot ausgeliefert: /screenshot.bmp");
}

void setup_web_server()
{
    web_server.on("/", HTTP_GET, []() {
        String page;
        page.reserve(700);
        page += F("<!doctype html><html><head><meta charset='utf-8'>");
        page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
        page += F("<title>Wallbox Display</title></head><body style='font-family:sans-serif;background:#111;color:#eee;text-align:center'>");
        page += F("<h2>Wallbox Display</h2><p><a style='color:#8cf' href='/screenshot.bmp'>Screenshot als BMP oeffnen</a></p>");
        page += F("<img src='/screenshot.bmp' style='max-width:100%;height:auto;border:1px solid #555'>");
        page += F("<p>Seite neu laden fuer einen aktuellen Screenshot.</p></body></html>");
        web_server.sendHeader("Cache-Control", "no-store");
        web_server.send(200, "text/html; charset=utf-8", page);
    });

    web_server.on("/screenshot.bmp", HTTP_GET, handle_screenshot);

    web_server.onNotFound([]() {
        web_server.send(404, "text/plain", "Nicht gefunden");
    });
}

void web_service()
{
    if(WiFi.status() != WL_CONNECTED) return;

    if(!web_server_started) {
        web_server.begin();
        web_server_started = true;
        Serial.printf("HTTP Screenshot bereit: http://%s/screenshot.bmp\n",
                      WiFi.localIP().toString().c_str());
    }

    web_server.handleClient();
}

static lv_obj_t *create_power_part(lv_obj_t *parent,
                                   const char *text,
                                   lv_coord_t x,
                                   lv_coord_t y,
                                   lv_coord_t width,
                                   const lv_font_t *font,
                                   lv_color_t color,
                                   lv_text_align_t align)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
    return label;
}

void setup_power_display()
{
    lv_obj_update_layout(ui_TempLabel);

    lv_obj_t *parent = lv_obj_get_parent(ui_TempLabel);
    const lv_coord_t x = lv_obj_get_x(ui_TempLabel);
    const lv_coord_t y = lv_obj_get_y(ui_TempLabel);
    const lv_font_t *font = lv_obj_get_style_text_font(ui_TempLabel, LV_PART_MAIN);
    const lv_color_t color = lv_obj_get_style_text_color(ui_TempLabel, LV_PART_MAIN);

    constexpr lv_coord_t integer_width = 72;
    constexpr lv_coord_t fraction_width = 54;
    constexpr lv_coord_t unit_gap = 6;
    constexpr lv_coord_t unit_width = 62;

    power_decimal_label = lv_label_create(parent);
    lv_label_set_text(power_decimal_label, ",");
    lv_obj_set_style_text_font(power_decimal_label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(power_decimal_label, color, LV_PART_MAIN);
    lv_obj_update_layout(power_decimal_label);

    const lv_coord_t comma_width = lv_obj_get_width(power_decimal_label);
    const lv_coord_t right_edge = x + lv_obj_get_width(ui_TempLabel);
    const lv_coord_t total_width = integer_width + comma_width +
                                   fraction_width + unit_gap + unit_width;
    const lv_coord_t group_x = right_edge - total_width;
    const lv_coord_t comma_x = group_x + integer_width;
    const lv_coord_t fraction_x = comma_x + comma_width;

    lv_obj_set_pos(ui_TempLabel, group_x, y);
    lv_obj_set_width(ui_TempLabel, integer_width);
    lv_label_set_long_mode(ui_TempLabel, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(ui_TempLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_pos(power_decimal_label, comma_x, y);

    power_fraction_label = create_power_part(parent, "00",
                                              fraction_x, y,
                                              fraction_width, font, color,
                                              LV_TEXT_ALIGN_LEFT);

    power_unit_label = create_power_part(parent, "kW",
                                          fraction_x + fraction_width + unit_gap, y,
                                          unit_width, font, color,
                                          LV_TEXT_ALIGN_LEFT);
}

void set_power_label(float watts)
{
    if(watts < 0.0f) watts = 0.0f;

    const uint32_t centi_kw = static_cast<uint32_t>((watts + 5.0f) / 10.0f);
    const uint32_t whole = centi_kw / 100U;
    const uint32_t fraction = centi_kw % 100U;

    char whole_text[8];
    char fraction_text[3];
    snprintf(whole_text, sizeof(whole_text), "%lu", static_cast<unsigned long>(whole));
    snprintf(fraction_text, sizeof(fraction_text), "%02lu", static_cast<unsigned long>(fraction));

    lv_label_set_text(ui_TempLabel, whole_text);
    lv_label_set_text(power_fraction_label, fraction_text);
}

void set_current_phase_label(float amps, const char *phase)
{
    if(amps < 0.0f) amps = 0.0f;

    int deci_amps = static_cast<int>(amps * 10.0f + 0.5f);
    if(deci_amps > 160) deci_amps = 160;
    lv_arc_set_value(ui_CurrentArc, deci_amps);

    char current_text[16];
    snprintf(current_text, sizeof(current_text), "%.1f A", amps);
    lv_label_set_text(ui_CurrentLabel, current_text);
    lv_label_set_text(ui_ChargePhaseLabel, phase);
}

void set_session_energy_label(float kwh)
{
    if(kwh < 0.0f) kwh = 0.0f;

    char text[20];
    snprintf(text, sizeof(text), "%.2f kWh", kwh);
    lv_label_set_text(ui_EnergyTodayLabel, text);
}

void set_total_energy_label(float kwh)
{
    if(kwh < 0.0f) kwh = 0.0f;

    char text[40];
    snprintf(text, sizeof(text), "Energie gesamt: %.2f kWh", kwh);
    lv_label_set_text(ui_TotalEnergyLabel, text);
}

void set_session_duration_label(uint32_t seconds)
{
    const uint32_t hours = seconds / 3600U;
    const uint32_t minutes = (seconds % 3600U) / 60U;

    char text[24];
    snprintf(text, sizeof(text), "%02lu:%02lu h",
             static_cast<unsigned long>(hours),
             static_cast<unsigned long>(minutes));
    lv_label_set_text(ui_ChargeTimeLabel, text);
}

void format_session_datetime(const char *value, char *formatted, size_t formatted_size)
{
    if(strcmp(value, "---") == 0) {
        snprintf(formatted, formatted_size, "---");
        return;
    }

    if(strlen(value) < 16 ||
       value[4] != '-' || value[7] != '-' ||
       value[10] != 'T' || value[13] != ':') {
        snprintf(formatted, formatted_size, "---");
        return;
    }

    snprintf(formatted, formatted_size,
             "%c%c.%c%c.%c%c%c%c %c%c:%c%c",
             value[8], value[9], value[5], value[6],
             value[0], value[1], value[2], value[3],
             value[11], value[12], value[14], value[15]);
}

void set_session_datetime_label(lv_obj_t *label, const char *value)
{
    char formatted[24];
    format_session_datetime(value, formatted, sizeof(formatted));
    lv_label_set_text(label, formatted);
}

bool status_is_charging(int status)
{
    return status == 2 ||
           status == 21 ||
           status == 22 ||
           status == 23;
}

bool status_has_control_fault(int status)
{
    return status >= 8 && status <= 19;
}

void set_control_enabled(lv_obj_t *control, bool enabled)
{
    if(enabled) {
        lv_obj_remove_state(control, LV_STATE_DISABLED);
    }
    else {
        lv_obj_add_state(control, LV_STATE_DISABLED);
    }
}

void update_mode_controls()
{
    const bool mode_feedback_valid = displayed_charge_mode >= 0 &&
                                     displayed_charge_mode <= 2;

    set_control_enabled(ui_ManualModeButton, mode_feedback_valid);
    set_control_enabled(ui_AutoModeButton, mode_feedback_valid);
    set_control_enabled(ui_ScheduledModeButton, mode_feedback_valid);

    const bool set_current_enabled = mode_feedback_valid &&
                                     displayed_charge_mode == 0;
    set_control_enabled(ui_SetCurrentSlider, set_current_enabled);
    lv_obj_set_style_text_color(ui_SetCurrentLabel,
                                set_current_enabled ? lv_color_hex(0xA2A6AA)
                                                    : lv_color_hex(0x64748B),
                                LV_PART_MAIN);
}

void render_charge_mode_buttons()
{
    const lv_color_t inactive = lv_color_hex(0x1E3A5F);
    const lv_color_t active = lv_color_hex(0x238EC4);
    const lv_color_t requested = lv_color_hex(0xD99A00);

    lv_obj_set_style_bg_color(ui_ManualModeButton,
                              requested_charge_mode == 0 ? requested :
                              displayed_charge_mode == 0 ? active : inactive,
                              LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_AutoModeButton,
                              requested_charge_mode == 1 ? requested :
                              displayed_charge_mode == 1 ? active : inactive,
                              LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_ScheduledModeButton,
                              requested_charge_mode == 2 ? requested :
                              displayed_charge_mode == 2 ? active : inactive,
                              LV_PART_MAIN);
}

void update_start_stop_controls()
{
    bool start_enabled = false;
    bool stop_enabled = false;

    const bool feedback_valid = displayed_charge_mode >= 0 &&
                                displayed_charge_mode <= 2 &&
                                displayed_wallbox_status >= 0;

    if(feedback_valid &&
       !status_has_control_fault(displayed_wallbox_status) &&
       displayed_wallbox_status != 24) {
        const bool charging = status_is_charging(displayed_wallbox_status);

        if(displayed_charge_mode == 0) {
            // Manual: Start und Stop wechseln entsprechend dem Ladezustand.
            start_enabled = !charging;
            stop_enabled = charging;
        }
        else {
            // Auto/Scheduled starten selbststaendig; nur eine laufende
            // Ladung darf manuell gestoppt werden.
            start_enabled = false;
            stop_enabled = charging;
        }
    }

    set_control_enabled(ui_OnButton, start_enabled);
    set_control_enabled(ui_OffButton, stop_enabled);
}

void set_charge_mode_buttons(int mode)
{
    if(requested_charge_mode == mode) {
        Serial.printf("Lademodus bestaetigt: %d\n", mode);
        requested_charge_mode = -1;
        charge_mode_request_started_ms = 0;
    }

    render_charge_mode_buttons();
    update_mode_controls();
    update_start_stop_controls();
}

void update_charge_mode_confirmation_timeout()
{
    if(requested_charge_mode < 0) return;
    if(millis() - charge_mode_request_started_ms < CHARGE_MODE_CONFIRM_TIMEOUT_MS) return;

    Serial.printf("Lademodus-Bestaetigung Timeout: %d\n", requested_charge_mode);
    requested_charge_mode = -1;
    charge_mode_request_started_ms = 0;
    render_charge_mode_buttons();
}

void set_current_slider(int amps)
{
    if(amps < 6) amps = 6;
    if(amps > 16) amps = 16;

    if(lv_slider_get_value(ui_SetCurrentSlider) != amps) {
        lv_slider_set_value(ui_SetCurrentSlider, amps, LV_ANIM_OFF);
    }

    char text[16];
    snprintf(text, sizeof(text), "Soll: %d A", amps);
    lv_label_set_text(ui_SetCurrentLabel, text);
}

void set_wallbox_status(int status)
{
    const char *main_text = "UNBEKANNT";
    const char *vehicle_text = "Fahrzeugstatus unbekannt";

    switch(status) {
        case 0:  main_text = "BEREIT"; vehicle_text = "Fahrzeug nicht verbunden"; break;
        case 1:  main_text = "VERBUNDEN"; vehicle_text = "Fahrzeug verbunden"; break;
        case 2:  main_text = "LADEN"; vehicle_text = "Fahrzeug wird geladen"; break;
        case 3:  main_text = "GELADEN"; vehicle_text = "Ladevorgang beendet"; break;
        case 4:  main_text = "WARTE AUF SONNE"; vehicle_text = "Fahrzeug verbunden"; break;
        case 5:  main_text = "RFID ERFORDERLICH"; vehicle_text = "Fahrzeug verbunden"; break;
        case 6:  main_text = "WARTE AUF START"; vehicle_text = "Fahrzeug verbunden"; break;
        case 7:  main_text = "BATTERIE ZU LEER"; vehicle_text = "Laden wartet"; break;
        case 8:  main_text = "ERDUNGSFEHLER"; vehicle_text = "Wallbox-Fehler"; break;
        case 9:  main_text = "KONTAKTFEHLER"; vehicle_text = "Wallbox-Fehler"; break;
        case 10: main_text = "CP-FEHLER"; vehicle_text = "Wallbox-Fehler"; break;
        case 11: main_text = "FEHLERSTROM"; vehicle_text = "Wallbox-Fehler"; break;
        case 12: main_text = "UNTERSPANNUNG"; vehicle_text = "Wallbox-Fehler"; break;
        case 13: main_text = "UEBERSPANNUNG"; vehicle_text = "Wallbox-Fehler"; break;
        case 14: main_text = "UEBERHITZUNG"; vehicle_text = "Wallbox-Fehler"; break;
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: main_text = "RESERVIERT"; vehicle_text = "Wallbox-Status reserviert"; break;
        case 20: main_text = "LADELIMIT"; vehicle_text = "Fahrzeug verbunden"; break;
        case 21: main_text = "STARTE LADUNG"; vehicle_text = "Fahrzeug verbunden"; break;
        case 22: main_text = "WECHSLE AUF 3 PHASEN"; vehicle_text = "Fahrzeug verbunden"; break;
        case 23: main_text = "WECHSLE AUF 1 PHASE"; vehicle_text = "Fahrzeug verbunden"; break;
        case 24: main_text = "BEENDE LADUNG"; vehicle_text = "Fahrzeug verbunden"; break;
    }

    lv_label_set_text(ui_HumiLabel, main_text);
    lv_label_set_text(ui_VehicleLabel, vehicle_text);
    update_start_stop_controls();
}

bool payload_to_buffer(byte *payload, unsigned int length, char *buffer, size_t buffer_size)
{
    if(buffer_size == 0) return false;
    const unsigned int copy_length = (length < buffer_size - 1) ? length : buffer_size - 1;
    memcpy(buffer, payload, copy_length);
    buffer[copy_length] = '\0';
    return copy_length == length;
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    char buffer[40];
    if(!payload_to_buffer(payload, length, buffer, sizeof(buffer))) {
        Serial.printf("MQTT Payload zu lang [%s]\n", topic);
        return;
    }

    if(strcmp(topic, TOPIC_STATUS) == 0) {
        char *end = nullptr;
        const long value = strtol(buffer, &end, 10);
        if(end == buffer || *end != '\0' || value < 0 || value > 24) {
            Serial.printf("MQTT Status ungueltig: %s\n", buffer);
            pending_wallbox_status = -1;
            return;
        }
        Serial.printf("MQTT RX [%s] = %ld\n", topic, value);
        pending_wallbox_status = static_cast<int>(value);
        return;
    }

    if(strcmp(topic, TOPIC_CHARGE_MODE) == 0) {
        if(strcmp(buffer, "manual") == 0) {
            pending_charge_mode = 0;
        }
        else if(strcmp(buffer, "auto") == 0) {
            pending_charge_mode = 1;
        }
        else if(strcmp(buffer, "scheduled") == 0) {
            pending_charge_mode = 2;
        }
        else {
            Serial.printf("MQTT Lademodus ungueltig: %s\n", buffer);
            return;
        }
        Serial.printf("MQTT RX [%s] = %s\n", topic, buffer);
        return;
    }

    if(strcmp(topic, TOPIC_SET_CURRENT) == 0) {
        char *end = nullptr;
        const long value = strtol(buffer, &end, 10);
        if(end == buffer || *end != '\0' || value < 6 || value > 16) {
            Serial.printf("MQTT Soll-Ladestrom ungueltig: %s\n", buffer);
            return;
        }
        pending_set_current_a = static_cast<int>(value);
        Serial.printf("MQTT RX [%s] = %ld A\n", topic, value);
        return;
    }

    if(strcmp(topic, TOPIC_POWER) == 0) {
        char *end = nullptr;
        const float value = strtof(buffer, &end);
        if(end == buffer || *end != '\0' || value < 0.0f) {
            Serial.printf("MQTT Power ungueltig: %s\n", buffer);
            return;
        }
        Serial.printf("MQTT RX [%s] = %.1f W\n", topic, value);
        pending_power_watts = value;
        power_received = true;
        return;
    }

    if(strcmp(topic, TOPIC_CURRENT) == 0) {
        char *end = nullptr;
        const float value = strtof(buffer, &end);
        if(end == buffer || *end != '\0' || value < 0.0f) {
            Serial.printf("MQTT Ladestrom ungueltig: %s\n", buffer);
            return;
        }
        Serial.printf("MQTT RX [%s] = %.1f A\n", topic, value);
        pending_current_amps = value;
        current_received = true;
        return;
    }

    if(strcmp(topic, TOPIC_CHARGE_PHASE) == 0) {
        if(strcmp(buffer, "1-ph") != 0 &&
           strcmp(buffer, "3-ph") != 0 &&
           strcmp(buffer, "off") != 0) {
            Serial.printf("MQTT Ladephase ungueltig: %s\n", buffer);
            return;
        }
        snprintf(pending_charge_phase, sizeof(pending_charge_phase), "%s", buffer);
        charge_phase_received = true;
        Serial.printf("MQTT RX [%s] = %s\n", topic, pending_charge_phase);
        return;
    }

    if(strcmp(topic, TOPIC_SESSION_ENERGY) == 0) {
        char *end = nullptr;
        const float value = strtof(buffer, &end);
        if(end == buffer || *end != '\0' || value < 0.0f) {
            Serial.printf("MQTT Ladeenergie ungueltig: %s\n", buffer);
            return;
        }
        Serial.printf("MQTT RX [%s] = %.3f kWh\n", topic, value);
        pending_session_energy_kwh = value;
        session_energy_received = true;
        return;
    }

    if(strcmp(topic, TOPIC_TOTAL_ENERGY) == 0) {
        char *end = nullptr;
        const float value = strtof(buffer, &end);
        if(end == buffer || *end != '\0' || value < 0.0f) {
            Serial.printf("MQTT Gesamtenergie ungueltig: %s\n", buffer);
            return;
        }
        Serial.printf("MQTT RX [%s] = %.3f kWh\n", topic, value);
        pending_total_energy_kwh = value;
        total_energy_received = true;
        return;
    }

    if(strcmp(topic, TOPIC_SESSION_DURATION) == 0) {
        char *end = nullptr;
        const unsigned long value = strtoul(buffer, &end, 10);
        if(end == buffer || *end != '\0' || buffer[0] == '-') {
            Serial.printf("MQTT Ladedauer ungueltig: %s\n", buffer);
            return;
        }
        pending_session_duration_s = static_cast<uint32_t>(value);
        session_duration_received = true;
        Serial.printf("MQTT RX [%s] = %lu s\n", topic, value);
        return;
    }

    if(strcmp(topic, TOPIC_SESSION_START) == 0) {
        snprintf(pending_session_start, sizeof(pending_session_start), "%s", buffer);
        session_start_received = true;
        Serial.printf("MQTT RX [%s] = %s\n", topic, pending_session_start);
        return;
    }

    if(strcmp(topic, TOPIC_SESSION_END) == 0) {
        snprintf(pending_session_end, sizeof(pending_session_end), "%s", buffer);
        session_end_received = true;
        Serial.printf("MQTT RX [%s] = %s\n", topic, pending_session_end);
        return;
    }
}

void update_display_from_pending_data()
{
    if(pending_wallbox_status != displayed_wallbox_status) {
        displayed_wallbox_status = pending_wallbox_status;
        set_wallbox_status(displayed_wallbox_status);
        Serial.printf("Display Status aktualisiert: %d\n", displayed_wallbox_status);
    }

    if(pending_charge_mode != displayed_charge_mode) {
        displayed_charge_mode = pending_charge_mode;
        set_charge_mode_buttons(displayed_charge_mode);
        Serial.printf("Display Lademodus aktualisiert: %d\n", displayed_charge_mode);
    }

    if(pending_set_current_a != displayed_set_current_a && pending_set_current_a >= 6) {
        displayed_set_current_a = pending_set_current_a;
        set_current_slider(displayed_set_current_a);
        Serial.printf("Display Soll-Ladestrom aktualisiert: %d A\n", displayed_set_current_a);
    }

    float effective_power = power_received ? pending_power_watts : 0.0f;
    if(displayed_wallbox_status == 0 || displayed_wallbox_status == 3) {
        effective_power = 0.0f;
    }

    if(effective_power != displayed_power_watts) {
        displayed_power_watts = effective_power;
        set_power_label(displayed_power_watts);
        Serial.printf("Display Power aktualisiert: %.1f W\n", displayed_power_watts);
    }

    float effective_current = current_received ? pending_current_amps : 0.0f;
    const char *effective_phase = charge_phase_received ? pending_charge_phase : "off";
    if(displayed_wallbox_status == 0 || displayed_wallbox_status == 3) {
        effective_current = 0.0f;
        effective_phase = "off";
    }

    if(effective_current != displayed_current_amps ||
       strcmp(effective_phase, displayed_charge_phase) != 0) {
        displayed_current_amps = effective_current;
        snprintf(displayed_charge_phase, sizeof(displayed_charge_phase), "%s", effective_phase);
        set_current_phase_label(displayed_current_amps, displayed_charge_phase);
        Serial.printf("Display Ladestrom/Phase aktualisiert: %.0f A / %s\n",
                      displayed_current_amps, displayed_charge_phase);
    }

    const float effective_session_energy = session_energy_received ? pending_session_energy_kwh : 0.0f;
    if(effective_session_energy != displayed_session_energy_kwh) {
        displayed_session_energy_kwh = effective_session_energy;
        set_session_energy_label(displayed_session_energy_kwh);
        Serial.printf("Display Ladeenergie aktualisiert: %.3f kWh\n", displayed_session_energy_kwh);
    }

    if(total_energy_received && pending_total_energy_kwh != displayed_total_energy_kwh) {
        displayed_total_energy_kwh = pending_total_energy_kwh;
        set_total_energy_label(displayed_total_energy_kwh);
        Serial.printf("Display Gesamtenergie aktualisiert: %.3f kWh\n",
                      displayed_total_energy_kwh);
    }

    const uint32_t effective_session_duration =
        session_duration_received ? pending_session_duration_s : 0U;
    if(effective_session_duration != displayed_session_duration_s) {
        displayed_session_duration_s = effective_session_duration;
        set_session_duration_label(displayed_session_duration_s);
        Serial.printf("Display Ladedauer aktualisiert: %lu s\n",
                      static_cast<unsigned long>(displayed_session_duration_s));
    }

    const char *effective_start = session_start_received ? pending_session_start : "---";
    if(strcmp(effective_start, displayed_session_start) != 0) {
        snprintf(displayed_session_start, sizeof(displayed_session_start), "%s", effective_start);
        set_session_datetime_label(ui_SessionStartLabel, displayed_session_start);
    }

    const bool session_running = displayed_wallbox_status == 2 ||
                                 (displayed_wallbox_status >= 21 && displayed_wallbox_status <= 24);
    const char *effective_end = session_running ? "---" :
                                (session_end_received ? pending_session_end : "---");
    if(strcmp(effective_end, displayed_session_end) != 0) {
        snprintf(displayed_session_end, sizeof(displayed_session_end), "%s", effective_end);
        set_session_datetime_label(ui_SessionEndLabel, displayed_session_end);
    }
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

void clock_service()
{
    if(WiFi.status() != WL_CONNECTED) return;

    if(!time_sync_started) {
        configTzTime(TIMEZONE_EUROPE_BERLIN, "pool.ntp.org", "time.nist.gov");
        time_sync_started = true;
        Serial.println("NTP-Zeitsynchronisation gestartet");
    }

    const uint32_t now = millis();
    if(now - last_clock_check_ms < 1000) return;
    last_clock_check_ms = now;

    struct tm local_time;
    if(!getLocalTime(&local_time, 0)) return;

    char clock_text[6];
    strftime(clock_text, sizeof(clock_text), "%H:%M", &local_time);
    if(strcmp(lv_label_get_text(ui_ClockLabel), clock_text) != 0) {
        lv_label_set_text(ui_ClockLabel, clock_text);
    }
}

void ota_service()
{
    if(WiFi.status() != WL_CONNECTED) return;

    if(!ota_started) {
        if(strlen(OTA_PASSWORD) == 0) {
            if(!ota_password_warning_printed) {
                Serial.println("OTA deaktiviert: OTA_PASSWORD fehlt in include/secrets.h");
                ota_password_warning_printed = true;
            }
            return;
        }

        ArduinoOTA.setHostname(OTA_HOSTNAME);
        ArduinoOTA.setPassword(OTA_PASSWORD);
        ArduinoOTA.onStart([]() {
            Serial.println("OTA-Update gestartet");
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("OTA-Update abgeschlossen, Neustart folgt");
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("OTA-Fehler [%u]\n", static_cast<unsigned int>(error));
        });
        ArduinoOTA.begin();
        ota_started = true;
        Serial.printf("OTA bereit: %s.local\n", OTA_HOSTNAME);
    }

    ArduinoOTA.handle();
}

void publish_presence()
{
    const String ip = WiFi.localIP().toString();
    const String rssi = String(WiFi.RSSI());

    mqtt.publish(TOPIC_ONLINE, "1", true);
    mqtt.publish(TOPIC_IP, ip.c_str(), true);
    mqtt.publish(TOPIC_RSSI, rssi.c_str(), true);

    char wifi_text[32];
    snprintf(wifi_text, sizeof(wifi_text), "WLAN  %ld dBm", WiFi.RSSI());
    lv_label_set_text(ui_WifiLabel, wifi_text);
    lv_label_set_text(ui_MqttLabel, "MQTT  Verbunden");

    String ip_text = "IP: " + ip;
    lv_label_set_text(ui_IpLabel, ip_text.c_str());

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
            ok = mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD,
                              TOPIC_ONLINE, 0, true, "0");
        }
        else {
            ok = mqtt.connect(MQTT_CLIENT_ID, TOPIC_ONLINE, 0, true, "0");
        }

        if(ok) {
            Serial.println("MQTT verbunden");
            mqtt.subscribe(TOPIC_STATUS, 0);
            mqtt.subscribe(TOPIC_CHARGE_MODE, 0);
            mqtt.subscribe(TOPIC_SET_CURRENT, 0);
            mqtt.subscribe(TOPIC_POWER, 0);
            mqtt.subscribe(TOPIC_CURRENT, 0);
            mqtt.subscribe(TOPIC_CHARGE_PHASE, 0);
            mqtt.subscribe(TOPIC_SESSION_ENERGY, 0);
            mqtt.subscribe(TOPIC_TOTAL_ENERGY, 0);
            mqtt.subscribe(TOPIC_SESSION_DURATION, 0);
            mqtt.subscribe(TOPIC_SESSION_START, 0);
            mqtt.subscribe(TOPIC_SESSION_END, 0);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_STATUS);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_CHARGE_MODE);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_SET_CURRENT);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_POWER);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_CURRENT);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_CHARGE_PHASE);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_SESSION_ENERGY);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_TOTAL_ENERGY);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_SESSION_DURATION);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_SESSION_START);
            Serial.printf("MQTT subscribe: %s\n", TOPIC_SESSION_END);
            publish_presence();
        }
        else {
            Serial.printf("MQTT Fehler rc=%d\n", mqtt.state());
            if(ui_MqttLabel) lv_label_set_text(ui_MqttLabel, "MQTT  Fehler");
        }
    }

    mqtt.loop();
}

void publish_charge_mode_command()
{
    if(charge_mode_request < 0 ||
       charge_mode_request > 2 ||
       !mqtt.connected()) {
        return;
    }

    const char *mode = charge_mode_request == 0 ? "manual" :
                       charge_mode_request == 1 ? "auto" : "scheduled";

    if(mqtt.publish(TOPIC_CMD_CHARGE_MODE, mode)) {
        if(charge_mode_request != displayed_charge_mode) {
            requested_charge_mode = charge_mode_request;
            charge_mode_request_started_ms = millis();
            render_charge_mode_buttons();
        }
        charge_mode_request = -1;
        Serial.printf("MQTT -> Lademodus %s\n", mode);
    }
}

void publish_set_current_command()
{
    if(set_current_request < 6 ||
       set_current_request > 16 ||
       !mqtt.connected()) {
        return;
    }

    char value[4];
    snprintf(value, sizeof(value), "%d", set_current_request);

    if(mqtt.publish(TOPIC_CMD_SET_CURRENT, value, false)) {
        Serial.printf("MQTT -> Soll-Ladestrom %d A\n", set_current_request);
        set_current_request = -1;
    }
}

void publish_button_command()
{
    if(led == last_led_sent) return;
    last_led_sent = led;

    if(!mqtt.connected()) return;

    if(led == 1) {
        mqtt.publish(TOPIC_CMD_START, "1");
        Serial.println("MQTT -> START");
    }
    else {
        mqtt.publish(TOPIC_CMD_STOP, "1");
        Serial.println("MQTT -> STOP");
    }
}
}

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.printf("Wallbox Display v%s\n", WALLBOX_DISPLAY_VERSION);

    crowpanel_release_usb_pads_for_i2c(TOUCH_SDA, TOUCH_SCL);
    delay(10);

    Wire1.begin(TOUCH_SDA, TOUCH_SCL);
    Wire1.setClock(400000);

    if(!lcd.begin()) {
        Serial.println("lcd.begin() failed");
        return;
    }
    delay(200);

    lv_init();
    lv_tick_set_cb(tick_get);
    touch_init();

    screen_frame_buffer = reinterpret_cast<lv_color_t *>(lcd.bus.getFrameBuffer(0));

    Serial.printf("RGB single frame buffer: %p\n", screen_frame_buffer);
    if(screen_frame_buffer == nullptr) {
        Serial.println("RGB frame buffer allocation failed");
        return;
    }

    lv_display_t *display = lv_display_create(lcd.width(), lcd.height());
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, display_flush);
    lv_display_set_buffers(display,
                           screen_frame_buffer,
                           nullptr,
                           SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_DIRECT);

    lv_indev_t *touchpad = lv_indev_create();
    lv_indev_set_type(touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touchpad, touchpad_read);

    pinMode(BACKLIGHT_PIN, OUTPUT);
    backlight_pwm_ready = ledcAttach(BACKLIGHT_PIN,
                                    BACKLIGHT_PWM_FREQUENCY_HZ,
                                    BACKLIGHT_PWM_RESOLUTION_BITS);
    if(!backlight_pwm_ready) {
        Serial.println("Backlight PWM konnte nicht initialisiert werden");
    }
    last_user_activity_ms = millis();
    set_backlight_state(BacklightState::Active);

    ui_init();
    lv_label_set_text(ui_HumiLabel, "MQTT WARTET");
    setup_power_display();
    set_power_label(0.0f);
    set_current_phase_label(0.0f, "off");
    set_session_energy_label(0.0f);
    set_session_duration_label(0U);
    lv_label_set_text(ui_SetCurrentLabel, "Soll: -- A");

    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqtt_callback);
    mqtt.setBufferSize(1024);

    setup_web_server();
    wifi_service();
}

void loop()
{
    wifi_service();
    clock_service();
    ota_service();
    mqtt_service();
    web_service();
    publish_button_command();
    publish_charge_mode_command();
    publish_set_current_command();
    update_display_from_pending_data();
    update_charge_mode_confirmation_timeout();
    backlight_service();

    lv_timer_handler();
    delay(10);
}
