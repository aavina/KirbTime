#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;
static TextLayer *s_battery_layer;
static char s_buffer[] = "00:00";
static InverterLayer *s_inv_layer;
static GBitmap *s_kirbwalk0_bitmap, *s_kirbwalk1_bitmap, *s_kirbwalk2_bitmap;
static BitmapLayer *s_kirbwalk0_layer, *s_kirbwalk1_layer, *s_kirbwalk2_layer;
BitmapLayer **active_kirb;
unsigned int active_frame = 0;

static AppTimer *timer;
const int delta = 125;
const int MAX_X = 144;
const int MAX_Y = 168;
const int kirbsize = 16;
int dx = 1;


void timer_callback(void *data) {
  GRect next;

  // Current position
  GRect current = layer_get_frame(bitmap_layer_get_layer(*active_kirb));
  
  next = GRect(-16, current.origin.y, kirbsize, kirbsize);
  
  // Check to see if we have hit the edges
  if(current.origin.x > MAX_X - kirbsize) {
    current.origin.x = 0;
  }

  // Switch layer to next
  if(active_frame == 0) {
    layer_set_frame(bitmap_layer_get_layer(*active_kirb), next);
    active_kirb = &s_kirbwalk1_layer;
    active_frame = 1;
    dx = 1;
  }
  else if(active_frame == 1) {
    if(dx == 1) {
      layer_set_frame(bitmap_layer_get_layer(*active_kirb), next);
      active_kirb = &s_kirbwalk2_layer;
      active_frame = 2;
    }
    else {
      layer_set_frame(bitmap_layer_get_layer(*active_kirb), next);
      active_kirb = &s_kirbwalk0_layer;
      active_frame = 0;
    }
  }
  else {
    layer_set_frame(bitmap_layer_get_layer(*active_kirb), next);
    active_kirb = &s_kirbwalk1_layer;
    active_frame = 1;
    dx = -1;
  }
  next = GRect(current.origin.x + 1, current.origin.y, kirbsize, kirbsize);
  layer_set_frame(bitmap_layer_get_layer(*active_kirb), next);
  
  timer = app_timer_register(delta, (AppTimerCallback) timer_callback, NULL);
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Update watchface display
  strftime(s_buffer, sizeof("00:00"), "%H:%M", tick_time);
  
  // Change the TextLayer to show next time
  text_layer_set_text(s_text_layer, s_buffer);
}

static void battery_handler(BatteryChargeState charge_state) {
  static char s_battery_buffer[32];
  
  if(charge_state.is_charging) {
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "charging");
  } else {
    snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d%% charged", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, s_battery_buffer);
}

void window_load(Window *window) {
  s_text_layer = text_layer_create(GRect(0, 83, 132, 168));
  text_layer_set_background_color(s_text_layer, GColorClear);
  text_layer_set_text(s_text_layer, GColorBlack);
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_text_layer));
  
  // Create battery layer
  s_battery_layer = text_layer_create(GRect(0, 0, 144, 20));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(s_window), text_layer_get_layer(s_battery_layer));
  
  // Add inverter layer
  s_inv_layer = inverter_layer_create(GRect(0, 80, 144, 62));
  layer_add_child(window_get_root_layer(s_window), inverter_layer_get_layer(s_inv_layer));
  
  // Mult kirb layers
  s_kirbwalk0_bitmap = gbitmap_create_with_resource(RESOURCE_ID_KIRB_WALK_0);
  s_kirbwalk0_layer = bitmap_layer_create(GRect(-16, 60, 16, 16));
  bitmap_layer_set_bitmap(s_kirbwalk0_layer, s_kirbwalk0_bitmap);
  
  s_kirbwalk1_bitmap = gbitmap_create_with_resource(RESOURCE_ID_KIRB_WALK_1);
  s_kirbwalk1_layer = bitmap_layer_create(GRect(63, 60, 16, 16));
  bitmap_layer_set_bitmap(s_kirbwalk1_layer, s_kirbwalk1_bitmap);
  
  s_kirbwalk2_bitmap = gbitmap_create_with_resource(RESOURCE_ID_KIRB_WALK_2);
  s_kirbwalk2_layer = bitmap_layer_create(GRect(-16, 60, 16, 16));
  bitmap_layer_set_bitmap(s_kirbwalk2_layer, s_kirbwalk2_bitmap);
  active_kirb = &s_kirbwalk1_layer;
  active_frame = 1;
  dx = 1;
  
  layer_add_child(window_get_root_layer(s_window), bitmap_layer_get_layer(s_kirbwalk0_layer));
  layer_add_child(window_get_root_layer(s_window), bitmap_layer_get_layer(s_kirbwalk1_layer));
  layer_add_child(window_get_root_layer(s_window), bitmap_layer_get_layer(s_kirbwalk2_layer));
  
  struct tm *t;
  time_t temp;
  temp = time(NULL);
  t = localtime(&temp);
  
  // Manually call the tick handler when window is done loading
  tick_handler(t, MINUTE_UNIT);
  
  // Manually call battery handler
  battery_handler(battery_state_service_peek());
  
  // Start kirb moving
  timer = app_timer_register(delta, (AppTimerCallback) timer_callback, NULL);
}

void window_unload(Window *window) {
  text_layer_destroy(s_battery_layer);
  text_layer_destroy(s_text_layer);
  
  inverter_layer_destroy(s_inv_layer);
  
  gbitmap_destroy(s_kirbwalk0_bitmap);
  gbitmap_destroy(s_kirbwalk1_bitmap);
  gbitmap_destroy(s_kirbwalk2_bitmap);
  bitmap_layer_destroy(s_kirbwalk0_layer);
  bitmap_layer_destroy(s_kirbwalk1_layer);
  bitmap_layer_destroy(s_kirbwalk2_layer);
  
  app_timer_cancel(timer);
}

void init() {
  // Initialize the app elements here
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  window_stack_push(s_window, true);
  
  // Subscribe to timer service
  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) tick_handler);
  
  // Subscribe to battery handler
  battery_state_service_subscribe(battery_handler);
}

void deinit() {
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}