#pragma once

#include <pebble.h>

// Function declarations
static void init(void);
static void deinit(void);
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void update_time(void);
static void battery_callback(BatteryChargeState state);
static void health_handler(HealthEventType event, void *context);
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void load_settings();
static void save_settings();