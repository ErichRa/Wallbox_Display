#ifndef WALLBOX_UI_H
#define WALLBOX_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_events.h"

void ui_Screen1_screen_init(void);
void ui_init(void);

extern lv_obj_t *ui_Screen1;
extern lv_obj_t *ui_TempLabel;          // Ladeleistung, z. B. 1.63 kW
extern lv_obj_t *ui_HumiLabel;          // Ladezustand, z. B. LÄDT · 7.1 A
extern lv_obj_t *ui_CurrentArc;
extern lv_obj_t *ui_CurrentLabel;
extern lv_obj_t *ui_ChargePhaseLabel;
extern lv_obj_t *ui_EnergyTodayLabel;
extern lv_obj_t *ui_ChargeTimeLabel;
extern lv_obj_t *ui_SessionStartLabel;
extern lv_obj_t *ui_SessionEndLabel;
extern lv_obj_t *ui_TotalEnergyLabel;
extern lv_obj_t *ui_WallboxOnlineLabel;
extern lv_obj_t *ui_VehicleLabel;
extern lv_obj_t *ui_WifiLabel;
extern lv_obj_t *ui_MqttLabel;
extern lv_obj_t *ui_IpLabel;
extern lv_obj_t *ui_OnButton;
extern lv_obj_t *ui_OffButton;
extern lv_obj_t *ui_ManualModeButton;
extern lv_obj_t *ui_AutoModeButton;
extern lv_obj_t *ui_ScheduledModeButton;
extern lv_obj_t *ui_SetCurrentSlider;
extern lv_obj_t *ui_SetCurrentLabel;

void ui_event_OnButton(lv_event_t *e);
void ui_event_OffButton(lv_event_t *e);
void ui_event_ManualModeButton(lv_event_t *e);
void ui_event_AutoModeButton(lv_event_t *e);
void ui_event_ScheduledModeButton(lv_event_t *e);
void ui_event_SetCurrentSlider(lv_event_t *e);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
