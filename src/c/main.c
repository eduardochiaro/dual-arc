#include <pebble.h>

// Forward declarations
static void update_display();
static void update_time();
static void update_steps();

// Main window and layers
static Window *s_main_window;
static Layer *s_canvas_layer;

// Text layers
static TextLayer *s_hour_layer;
static TextLayer *s_minute_layer;
static TextLayer *s_date_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_steps_layer;

// Data
static int s_battery_level = 0;
static int s_current_steps = 0;
static int s_max_steps = 10000;  // Default max, will be updated

// Buffers
static char s_hour_buffer[4];
static char s_minute_buffer[4];
static char s_ampm_buffer[4];
static char s_date_buffer[16];
static char s_battery_buffer[8];
static char s_steps_buffer[8];

// Settings
static GColor s_background_color;
static GColor s_foreground_color;
static GColor s_secondary_color;
static bool s_use_24h = true;

// Colors for the arcs and bars
static GColor s_color_teal;
static GColor s_color_orange;
static GColor s_color_pink;
static GColor s_color_yellow;
static GColor s_color_background_yellow;
static GColor s_color_background_pink;

// Load settings
static void load_settings() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "load settings called");
  s_use_24h = persist_exists(MESSAGE_KEY_USE_24_HOUR) ? persist_read_bool(MESSAGE_KEY_USE_24_HOUR) : true;
  s_background_color = persist_exists(MESSAGE_KEY_BACKGROUND_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_BACKGROUND_COLOR) } : GColorBlack;
  s_foreground_color = persist_exists(MESSAGE_KEY_FOREGROUND_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_FOREGROUND_COLOR) } : GColorWhite;
  s_secondary_color = persist_exists(MESSAGE_KEY_SECONDARY_COLOR) ? (GColor){ .argb = (uint8_t)persist_read_int(MESSAGE_KEY_SECONDARY_COLOR) } : GColorLightGray;
  s_max_steps = persist_exists(MESSAGE_KEY_STEP_GOAL) ? persist_read_int(MESSAGE_KEY_STEP_GOAL) : 10000;
}

// Save settings
static void save_settings() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "save settings called");
  persist_write_bool(MESSAGE_KEY_USE_24_HOUR, s_use_24h);
  persist_write_int(MESSAGE_KEY_BACKGROUND_COLOR, s_background_color.argb);
  persist_write_int(MESSAGE_KEY_FOREGROUND_COLOR, s_foreground_color.argb);
  persist_write_int(MESSAGE_KEY_SECONDARY_COLOR, s_secondary_color.argb);
  persist_write_int(MESSAGE_KEY_STEP_GOAL, s_max_steps);
}

// Inbox received callback
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "inbox received callback called");
  Tuple *use_24h_tuple = dict_find(iterator, MESSAGE_KEY_USE_24_HOUR);
  
  if (use_24h_tuple) {
    s_use_24h = use_24h_tuple->value->int32 == 1;
  }

  Tuple *bgcolor_tuple = dict_find(iterator, MESSAGE_KEY_BACKGROUND_COLOR);
  if (bgcolor_tuple) {
    s_background_color = GColorFromHEX(bgcolor_tuple->value->int32);
  }

  Tuple *hourscolor_tuple = dict_find(iterator, MESSAGE_KEY_FOREGROUND_COLOR);
  if (hourscolor_tuple) {
    s_foreground_color = GColorFromHEX(hourscolor_tuple->value->int32);
  }

  Tuple *minutescolor_tuple = dict_find(iterator, MESSAGE_KEY_SECONDARY_COLOR);
  if (minutescolor_tuple) {
    s_secondary_color = GColorFromHEX(minutescolor_tuple->value->int32);
  }

  save_settings();

  update_display();
  
}

static void update_display() {
  layer_mark_dirty(s_canvas_layer);
  window_set_background_color(s_main_window, s_background_color);
}

// Update battery level
static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%02d%%", s_battery_level);
  if (s_battery_layer) {
    text_layer_set_text(s_battery_layer, s_battery_buffer);
  }
  layer_mark_dirty(s_canvas_layer);
}

// Update step count
static void update_steps() {
  s_current_steps = (int)health_service_sum_today(HealthMetricStepCount);
  
  // Update max steps if current exceeds it
  if (s_current_steps > s_max_steps) {
    s_max_steps = s_current_steps;
  }
  
  snprintf(s_steps_buffer, sizeof(s_steps_buffer), "%05d", s_current_steps);
  if (s_steps_layer) {
    text_layer_set_text(s_steps_layer, s_steps_buffer);
  }
  layer_mark_dirty(s_canvas_layer);
}

// Health event handler
static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate || event == HealthEventMovementUpdate) {
    update_steps();
  }
}

// Update time
static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Hour
  strftime(s_hour_buffer, sizeof(s_hour_buffer), s_use_24h ? "%H" : "%I", tick_time);
  
  // Remove leading zero for 12-hour format
  if (!s_use_24h && s_hour_buffer[0] == '0') {
    memmove(s_hour_buffer, s_hour_buffer + 1, strlen(s_hour_buffer));
  }
  
  // Minute
  strftime(s_minute_buffer, sizeof(s_minute_buffer), "%M", tick_time);
  
  // AM/PM
  if (!s_use_24h) {
    strftime(s_ampm_buffer, sizeof(s_ampm_buffer), "%p", tick_time);
  }
  
  // Date: day number and weekday
  strftime(s_date_buffer, sizeof(s_date_buffer), "%d %a", tick_time);
  
  // Convert weekday to uppercase
  for (int i = 3; i < 6 && s_date_buffer[i]; i++) {
    if (s_date_buffer[i] >= 'a' && s_date_buffer[i] <= 'z') {
      s_date_buffer[i] -= 32;
    }
  }
  
  if (s_hour_layer) {
    text_layer_set_text(s_hour_layer, s_hour_buffer);
  }
  if (s_minute_layer) {
    text_layer_set_text(s_minute_layer, s_minute_buffer);
  }
  if (s_date_layer) {
    text_layer_set_text(s_date_layer, s_date_buffer);
  }
  
  layer_mark_dirty(s_canvas_layer);
}

// Tick handler
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// Draw the canvas (arcs and bars)
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  int radius = (bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h) / 2 - 8;
  
  // Draw outer arc decorations (tick marks around edge)
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, 1);
  
  for (int i = -15; i <= 15; i++) {
    int32_t angle = TRIG_MAX_ANGLE * i / 60;
    int inner_r = radius - 3;
    int outer_r = radius;
    
    // Longer tick at every 5 minutes
    if (i % 5 == 0) {
      inner_r = radius - 6;
    }
    
    GPoint inner = {
      .x = center.x + (inner_r * sin_lookup(angle) / TRIG_MAX_RATIO),
      .y = center.y - (inner_r * cos_lookup(angle) / TRIG_MAX_RATIO)
    };
    GPoint outer = {
      .x = center.x + (outer_r * sin_lookup(angle) / TRIG_MAX_RATIO),
      .y = center.y - (outer_r * cos_lookup(angle) / TRIG_MAX_RATIO)
    };
    
    graphics_draw_line(ctx, inner, outer);
  }
  
  // Quarter arc progress indicators at bottom
  int arc_radius = radius - 4;
  int arc_width = PBL_IF_ROUND_ELSE(14, 8);
  if (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery) {
    arc_width = 14;
  }
  
  // Arc angles for bottom quarter arcs
  // Left arc: from about 7 o'clock to 6 o'clock (battery)
  // Right arc: from about 6 o'clock to 5 o'clock (steps)
  
  int32_t left_arc_start = DEG_TO_TRIGANGLE(200);   // Start at ~7 o'clock
  int32_t left_arc_end = DEG_TO_TRIGANGLE(250);     // End at ~8 o'clock  
  int32_t right_arc_start = DEG_TO_TRIGANGLE(110);  // Start at ~5 o'clock
  int32_t right_arc_end = DEG_TO_TRIGANGLE(160);    // End at ~4 o'clock
  
  GRect arc_rect = GRect(center.x - arc_radius, center.y - arc_radius, 
                         arc_radius * 2, arc_radius * 2);
  
  // Left arc background (battery)
  graphics_context_set_stroke_color(ctx, s_color_background_yellow);
  graphics_context_set_stroke_width(ctx, arc_width);
  graphics_draw_arc(ctx, arc_rect, GOvalScaleModeFitCircle, left_arc_start, left_arc_end);
  
  // Left arc fill (battery) - yellow/orange gradient
  if (s_battery_level > 0) {
    int32_t arc_range = left_arc_end - left_arc_start;
    int32_t fill_end = left_arc_start + (arc_range * s_battery_level / 100);
    graphics_context_set_stroke_color(ctx, s_color_yellow);
    graphics_context_set_stroke_width(ctx, arc_width - 2);
    graphics_draw_arc(ctx, arc_rect, GOvalScaleModeFitCircle, left_arc_start, fill_end);
  }
  
  // Right arc background (steps)
  graphics_context_set_stroke_color(ctx, s_color_background_pink);
  graphics_context_set_stroke_width(ctx, arc_width);
  graphics_draw_arc(ctx, arc_rect, GOvalScaleModeFitCircle, right_arc_start, right_arc_end);
  
  // Right arc fill (steps) - pink
  int steps_percent = (s_current_steps * 100) / s_max_steps;
  if (steps_percent > 100) steps_percent = 100;
  if (steps_percent > 0) {
    int32_t arc_range = right_arc_end - right_arc_start;
    int32_t fill_start = right_arc_end - (arc_range * steps_percent / 100);
    graphics_context_set_stroke_color(ctx, s_color_pink);
    graphics_context_set_stroke_width(ctx, arc_width - 2);
    graphics_draw_arc(ctx, arc_rect, GOvalScaleModeFitCircle, fill_start, right_arc_end);
  }
  
  // Draw small dots at center bottom
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  for (int i = 0; i < 3; i++) {
    graphics_fill_circle(ctx, GPoint(center.x - 4 + i * 4, center.y + 52), 1);
  }
}

// Create text layer helper
static TextLayer* create_text_layer(GRect frame, GFont font, GTextAlignment alignment) {
  TextLayer *layer = text_layer_create(frame);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, s_foreground_color);
  text_layer_set_font(layer, font);
  text_layer_set_text_alignment(layer, alignment);
  return layer;
}

// Main window load
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "loaded main window");
  
  // Initialize colors
  s_color_teal = GColorTiffanyBlue;
  s_color_orange = GColorOrange;
  s_color_pink = GColorMelon;
  s_color_yellow = GColorYellow;
  s_color_background_yellow = GColorArmyGreen;
  s_color_background_pink = GColorBulgarianRose;

  // Set window background
  window_set_background_color(window, s_background_color);
  
  // Create canvas layer for custom drawing
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  // Create text layers
  // Date (above time)
  s_date_layer = create_text_layer(GRect(0, center.y - 48, bounds.size.w, 24),
                                    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                                    GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  GFont time_font = fonts_get_system_font(PBL_IF_ROUND_ELSE(FONT_KEY_LECO_42_NUMBERS, FONT_KEY_LECO_36_BOLD_NUMBERS));
  if (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery) {
    time_font = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  }
  
  // Hour (center left, white)
  s_hour_layer = create_text_layer(GRect(0, center.y - 30, center.x - 2, 50),
                                    time_font,
                                    GTextAlignmentRight);
  text_layer_set_text_color(s_hour_layer, s_foreground_color);
  layer_add_child(window_layer, text_layer_get_layer(s_hour_layer));
  
  // Minute (center right, light gray)
  s_minute_layer = create_text_layer(GRect(center.x + 2, center.y - 30, center.x, 50),
                                      time_font,
                                      GTextAlignmentLeft);
  text_layer_set_text_color(s_minute_layer, s_secondary_color);
  layer_add_child(window_layer, text_layer_get_layer(s_minute_layer));
  
  // Battery text (bottom left)
  s_battery_layer = create_text_layer(GRect(center.x - 50, PBL_IF_ROUND_ELSE(center.y + 22, center.y + 10), 50, 20),
                                       fonts_get_system_font(FONT_KEY_GOTHIC_14),
                                       GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  
  // Steps text (bottom right)  
  s_steps_layer = create_text_layer(GRect(center.x + 5, PBL_IF_ROUND_ELSE(center.y + 22, center.y + 10), 50, 20),
                                     fonts_get_system_font(FONT_KEY_GOTHIC_14),
                                     GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));
  
  // Initialize data
  battery_callback(battery_state_service_peek());
  update_time();
  update_steps();
}

// Main window unload
static void main_window_unload(Window *window) {
  text_layer_destroy(s_hour_layer);
  text_layer_destroy(s_minute_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_steps_layer);
  layer_destroy(s_canvas_layer);
}

// Init
static void init(void) {
  // Load settings
  load_settings();
  
  // Create main window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  
  // Register services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  
  // Subscribe to health service
  #if defined(PBL_HEALTH)
  if (health_service_events_subscribe(health_handler, NULL)) {
    // Health service available
  }
  #endif

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(128, 64);
}

// Deinit
static void deinit() {
  window_destroy(s_main_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  #if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
  #endif
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
