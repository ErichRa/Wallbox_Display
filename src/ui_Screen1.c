#include "ui.h"

#define COLOR_BG      lv_color_hex(0x07090B)
#define COLOR_PANEL   lv_color_hex(0x101820)
#define COLOR_PANEL_2 lv_color_hex(0x16202C)
#define COLOR_TEXT    lv_color_hex(0xF5F3F2)
#define COLOR_MUTED   lv_color_hex(0xA2A6AA)
#define COLOR_GREEN   lv_color_hex(0x79D856)
#define COLOR_BLUE    lv_color_hex(0x3A91E8)
#define COLOR_YELLOW  lv_color_hex(0xFFC138)

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int x, int y,
                            const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

static lv_obj_t *make_panel(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(panel, COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 14, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    return panel;
}

static void style_button(lv_obj_t *button, lv_color_t color)
{
    lv_obj_set_style_bg_color(button, color, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_bg_color(button, lv_color_darken(color, LV_OPA_20), LV_STATE_PRESSED);
}

void ui_Screen1_screen_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_remove_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen1, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(ui_Screen1, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(ui_Screen1, 0, 0);

    /* Header */
    make_label(ui_Screen1, "WALLBOX", 28, 18, &lv_font_montserrat_32, COLOR_TEXT);
    make_label(ui_Screen1, "Smart Charging", 30, 52, &lv_font_montserrat_16, COLOR_MUTED);

    ui_WifiLabel = make_label(ui_Screen1, "WLAN  -- dBm", 430, 18,
                              &lv_font_montserrat_16, COLOR_GREEN);
    ui_MqttLabel = make_label(ui_Screen1, "MQTT  --", 575, 18,
                              &lv_font_montserrat_16, COLOR_GREEN);
    ui_IpLabel = make_label(ui_Screen1, "IP: ---.---.---.---", 575, 47,
                            &lv_font_montserrat_14, COLOR_MUTED);

    lv_obj_t *header_line = lv_obj_create(ui_Screen1);
    lv_obj_set_pos(header_line, 24, 76);
    lv_obj_set_size(header_line, 752, 1);
    lv_obj_set_style_bg_color(header_line, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_opa(header_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header_line, 0, 0);

    /* Main card */
    lv_obj_t *main_panel = make_panel(ui_Screen1, 24, 90, 752, 230);

    ui_SocArc = lv_arc_create(main_panel);
    lv_obj_set_pos(ui_SocArc, 20, 23);
    lv_obj_set_size(ui_SocArc, 185, 185);
    lv_arc_set_range(ui_SocArc, 0, 100);
    lv_arc_set_value(ui_SocArc, 0);
    lv_arc_set_bg_angles(ui_SocArc, 135, 45);
    lv_obj_remove_flag(ui_SocArc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui_SocArc, lv_color_hex(0x294034), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui_SocArc, 14, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui_SocArc, COLOR_GREEN, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ui_SocArc, 14, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ui_SocArc, LV_OPA_TRANSP, LV_PART_KNOB);

    make_label(main_panel, "EV", 92, 66, &lv_font_montserrat_24, COLOR_GREEN);
    ui_SocLabel = make_label(main_panel, "-- %", 76, 105,
                             &lv_font_montserrat_28, COLOR_TEXT);
    make_label(main_panel, "SOC", 94, 142, &lv_font_montserrat_14, COLOR_MUTED);

    ui_TempLabel = make_label(main_panel, "--.-- kW", 250, 27,
                              &lv_font_montserrat_40, COLOR_TEXT);
    lv_obj_set_width(ui_TempLabel, 210);
    lv_label_set_long_mode(ui_TempLabel, LV_LABEL_LONG_CLIP);
    ui_HumiLabel = make_label(main_panel, "BEREIT", 250, 87,
                              &lv_font_montserrat_28, COLOR_GREEN);
    ui_VehicleLabel = make_label(main_panel, "Fahrzeug nicht verbunden", 250, 136,
                                 &lv_font_montserrat_18, COLOR_MUTED);

    ui_ManualModeButton = lv_button_create(main_panel);
    lv_obj_set_pos(ui_ManualModeButton, 250, 174);
    lv_obj_set_size(ui_ManualModeButton, 88, 40);
    style_button(ui_ManualModeButton, lv_color_hex(0x334155));
    lv_obj_t *manual_label = make_label(ui_ManualModeButton, "MANUAL", 0, 0,
                                        &lv_font_montserrat_14, COLOR_TEXT);
    lv_obj_center(manual_label);

    ui_AutoModeButton = lv_button_create(main_panel);
    lv_obj_set_pos(ui_AutoModeButton, 344, 174);
    lv_obj_set_size(ui_AutoModeButton, 88, 40);
    style_button(ui_AutoModeButton, lv_color_hex(0x334155));
    lv_obj_t *auto_label = make_label(ui_AutoModeButton, "AUTO", 0, 0,
                                      &lv_font_montserrat_14, COLOR_TEXT);
    lv_obj_center(auto_label);

    ui_ScheduledModeButton = lv_button_create(main_panel);
    lv_obj_set_pos(ui_ScheduledModeButton, 438, 174);
    lv_obj_set_size(ui_ScheduledModeButton, 88, 40);
    style_button(ui_ScheduledModeButton, lv_color_hex(0x334155));
    lv_obj_t *scheduled_label = make_label(ui_ScheduledModeButton, "SCHEDULED", 0, 0,
                                           &lv_font_montserrat_14, COLOR_TEXT);
    lv_obj_center(scheduled_label);

    lv_obj_add_event_cb(ui_ManualModeButton, ui_event_ManualModeButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_AutoModeButton, ui_event_AutoModeButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ScheduledModeButton, ui_event_ScheduledModeButton, LV_EVENT_ALL, NULL);

    lv_obj_t *divider = lv_obj_create(main_panel);
    lv_obj_set_pos(divider, 545, 20);
    lv_obj_set_size(divider, 1, 190);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x334155), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);

    make_label(main_panel, "Ladestrom / Phase", 570, 24, &lv_font_montserrat_14, COLOR_BLUE);
    ui_VoltageLabel = make_label(main_panel, "0 A  --", 570, 45,
                                  &lv_font_montserrat_24, COLOR_TEXT);

    make_label(main_panel, "Ladeenergie", 570, 88, &lv_font_montserrat_14, COLOR_YELLOW);
    ui_EnergyTodayLabel = make_label(main_panel, "--.-- kWh", 570, 109,
                                     &lv_font_montserrat_20, COLOR_TEXT);

    make_label(main_panel, "Ladezeit", 570, 151, &lv_font_montserrat_14, COLOR_MUTED);
    ui_ChargeTimeLabel = make_label(main_panel, "--.-- h", 570, 172,
                                    &lv_font_montserrat_20, COLOR_TEXT);

    /* Start / Stop */
    ui_OnButton = lv_button_create(ui_Screen1);
    lv_obj_set_pos(ui_OnButton, 24, 334);
    lv_obj_set_size(ui_OnButton, 368, 92);
    style_button(ui_OnButton, lv_color_hex(0x2E8B3C));
    make_label(ui_OnButton, "START", 118, 16, &lv_font_montserrat_32, COLOR_TEXT);
    make_label(ui_OnButton, "Ladevorgang starten", 101, 58,
               &lv_font_montserrat_14, lv_color_hex(0xDFF7E4));

    ui_OffButton = lv_button_create(ui_Screen1);
    lv_obj_set_pos(ui_OffButton, 408, 334);
    lv_obj_set_size(ui_OffButton, 368, 92);
    style_button(ui_OffButton, lv_color_hex(0xB91C1C));
    make_label(ui_OffButton, "STOP", 126, 16, &lv_font_montserrat_32, COLOR_TEXT);
    make_label(ui_OffButton, "Ladevorgang stoppen", 104, 58,
               &lv_font_montserrat_14, lv_color_hex(0xFCE4E4));

    lv_obj_add_event_cb(ui_OnButton, ui_event_OnButton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_OffButton, ui_event_OffButton, LV_EVENT_ALL, NULL);

    /* Footer */
    lv_obj_t *footer = make_panel(ui_Screen1, 24, 438, 752, 34);
    lv_obj_set_style_bg_color(footer, COLOR_PANEL_2, 0);

    ui_TotalEnergyLabel = make_label(footer, "Energie gesamt: --.-- kWh", 14, 8,
                                     &lv_font_montserrat_14, COLOR_TEXT);
    ui_WallboxOnlineLabel = make_label(footer, "Wallbox --", 610, 8,
                                       &lv_font_montserrat_14, COLOR_MUTED);
}
