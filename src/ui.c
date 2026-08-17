#include "ui.h"

lv_obj_t *ui_Screen1;
lv_obj_t *ui_TempLabel;
lv_obj_t *ui_HumiLabel;
lv_obj_t *ui_SocArc;
lv_obj_t *ui_SocLabel;
lv_obj_t *ui_VoltageLabel;
lv_obj_t *ui_EnergyTodayLabel;
lv_obj_t *ui_ChargeTimeLabel;
lv_obj_t *ui_TemperatureLabel;
lv_obj_t *ui_VehicleLabel;
lv_obj_t *ui_WifiLabel;
lv_obj_t *ui_MqttLabel;
lv_obj_t *ui_IpLabel;
lv_obj_t *ui_OnButton;
lv_obj_t *ui_OffButton;

void ui_event_OnButton(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_RELEASED) TurnOn(e);
}

void ui_event_OffButton(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_RELEASED) TurnOff(e);
}

void ui_init(void)
{
    lv_display_t *display = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(display,
                                              lv_palette_main(LV_PALETTE_BLUE),
                                              lv_palette_main(LV_PALETTE_RED),
                                              true,
                                              LV_FONT_DEFAULT);
    lv_display_set_theme(display, theme);
    ui_Screen1_screen_init();
    lv_screen_load(ui_Screen1);
}
