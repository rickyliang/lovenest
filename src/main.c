#include <pebble.h>
#include "selection_layer.h"
#include "alarm_window.h"

#define NUM_MENU_ICONS 3
#define NUM_FIRST_MENU_ITEMS 3
#define APPMSG_INBOUND_SIZE 256
#define APPMSG_OUTBOUND_SIZE 256

enum {
  MESSAGE_TYPE = 99,
  SLEEP_MESSAGE = 105,
  WAKE_MESSAGE = 106,
  KEY_ASLEEP = 69,
  KEY_WAKE_HOUR = 80,
  KEY_WAKE_MINUTE = 81,
};

Window *main_window;
Window *wake_window;
static Layer *wake_selection_layer;
static MenuLayer *s_menu_layer;



/** WAKE WINDOW **/

#define HOUR_MIN_VALUE 1
#define HOUR_MAX_VALUE 12
#define MINUTE_MAX_VALUE 59

static void wake_window_init();
static void wake_window_deinit();

int hour;
int minute;
int ampm;
char s_hour[3];
char s_minute[3];
char s_ampm[3];

static void send_wake() {
  APP_LOG(APP_LOG_LEVEL_INFO, "send_wake START.");
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_INFO, "ERROR: iter is null.");
    return;
  }
  if (dict_write_uint8(iter, MESSAGE_TYPE, WAKE_MESSAGE) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "ERROR: dict write failed.");
    return;
  }
  
  // Adjust times to military time
  if (strcmp(s_ampm, "AM") == 0) {
    if (hour == HOUR_MAX_VALUE) strcpy(s_hour, "00");
  } else {
    if (hour < HOUR_MAX_VALUE) snprintf(s_hour, sizeof(s_hour), "%02d", hour + 12);
  }
  
  // Write hour and minute
  if (dict_write_cstring(iter, KEY_WAKE_HOUR, s_hour) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "ERROR: dict write failed.");
    return;
  }
  if (dict_write_cstring(iter, KEY_WAKE_MINUTE, s_minute) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "ERROR: dict write failed.");
    return;
  }
  app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_INFO, "send_wake OK.");
  
  // Finish window
  wake_window_deinit();
}

static char* selection_handle_get_text(int index, void *context) {
  if (index == 0) {
    snprintf(s_hour, sizeof(s_hour), "%02d", hour);
    return s_hour;
  }
  else if (index == 1) {
    snprintf(s_minute, sizeof(s_minute), "%02d", minute);
    return s_minute;
  }
  else {
    if (ampm == 1) {
      strcpy(s_ampm, "PM");
    } else {
      strcpy(s_ampm, "AM");
    }
    return s_ampm;
  }
}

static void selection_handle_complete(void *context) {
  send_wake();
}

static void selection_handle_inc(int index, uint8_t clicks, void *context) {
  if (index == 0) {
    hour++;
    if (hour > HOUR_MAX_VALUE) hour = HOUR_MIN_VALUE;
  } else if (index == 1) {
    minute++;
    if (minute > MINUTE_MAX_VALUE) minute = 0;
  } else {
    ampm ^= 1;
  }
}

static void selection_handle_dec(int index, uint8_t clicks, void *context) {
  if (index == 0) {
    hour--;
    if (hour < HOUR_MIN_VALUE) hour = HOUR_MAX_VALUE;
  } else if (index == 1) {
    minute--;
    if (minute < 0) minute = MINUTE_MAX_VALUE;
  } else {
    ampm ^= 1;
  }
}

static void wake_window_load(Window *window) {
  // Get the root layer
  Layer *window_layer = window_get_root_layer(window);
  
  // Get the bounds of the root layer for sizing the text layer
  GRect bounds = layer_get_bounds(window_layer);
  
  // Initialize time
  hour = 12;   // 00 hours
  minute = 0;  // 00 minutes
  ampm = 0;    // AM
  
  // Create and add selection layer
  const GEdgeInsets selection_insets = GEdgeInsets(
    (bounds.size.h - PIN_WINDOW_SIZE.h) / 2, 
    (bounds.size.w - PIN_WINDOW_SIZE.w) / 2);
  wake_selection_layer = selection_layer_create(grect_inset(bounds, selection_insets), PIN_WINDOW_NUM_CELLS);
  for (int i = 0; i < PIN_WINDOW_NUM_CELLS; i++) {
    selection_layer_set_cell_width(wake_selection_layer, i, 40);
  }
  selection_layer_set_cell_padding(wake_selection_layer, 4);
  selection_layer_set_active_bg_color(wake_selection_layer, GColorRed);
  selection_layer_set_inactive_bg_color(wake_selection_layer, GColorDarkGray);
  selection_layer_set_click_config_onto_window(wake_selection_layer, window);
  selection_layer_set_callbacks(wake_selection_layer, window_layer, (SelectionLayerCallbacks) {
    .get_cell_text = selection_handle_get_text,
    .complete = selection_handle_complete,
    .increment = selection_handle_inc,
    .decrement = selection_handle_dec,
  });
  APP_LOG(APP_LOG_LEVEL_INFO, "layer_add_child BEFORE.");
  layer_add_child(window_layer, wake_selection_layer);
  APP_LOG(APP_LOG_LEVEL_INFO, "layer_add_child AFTER.");
}

static void wake_window_unload(Window *window) {
  selection_layer_destroy(wake_selection_layer);
}

static void wake_window_init() {
  wake_window = window_create();
  window_set_window_handlers(wake_window, (WindowHandlers) {
    .load = wake_window_load,
    .unload = wake_window_unload
  });
  window_stack_push(wake_window, true);
}

static void wake_window_deinit() {
  window_stack_remove(wake_window, true);
  window_destroy(wake_window);
}



/** MAIN WINDOW **/

bool auto_sleep_on;
char s_onoff[4];

void send_sleep(bool asleep) {
  APP_LOG(APP_LOG_LEVEL_INFO, "send_sleep START.");
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_INFO, "ERROR: iter is null.");
    return;
  }
  if (dict_write_uint8(iter, MESSAGE_TYPE, SLEEP_MESSAGE) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "ERROR: dict write failed.");
    return;
  }
  if (dict_write_uint8(iter, KEY_ASLEEP, asleep) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "ERROR: dict write failed.");
    return;
  }
  app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_INFO, "send_sleep OK.");
}
 
void auto_sleep_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  HealthActivityMask activities = health_service_peek_current_activities();
  if(activities & HealthActivitySleep) {
    APP_LOG(APP_LOG_LEVEL_INFO, "The user is sleeping.");
    send_sleep(true);
  } else if(activities & HealthActivityRestfulSleep) {
    APP_LOG(APP_LOG_LEVEL_INFO, "The user is sleeping peacefully.");
    send_sleep(true);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "The user is not currently sleeping.");
    send_sleep(true);
  }
}

static void auto_sleep_click() {
  if (auto_sleep_on) {
    auto_sleep_on = false;
    strcpy(s_onoff, "OFF");
    tick_timer_service_unsubscribe();
    APP_LOG(APP_LOG_LEVEL_INFO, "Auto sleep detection turned off.");
  } else {
    auto_sleep_on = true;
    strcpy(s_onoff, "ON");
    tick_timer_service_subscribe(MINUTE_UNIT, auto_sleep_tick_handler);
    APP_LOG(APP_LOG_LEVEL_INFO, "Auto sleep detection turned on.");
  }
  layer_mark_dirty(window_get_root_layer(main_window));
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_FIRST_MENU_ITEMS;
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  switch (cell_index->row) {
    case 0:
      menu_cell_basic_draw(ctx, cell_layer, "Auto Sleep", s_onoff, NULL);
      break;
    case 1:
      menu_cell_basic_draw(ctx, cell_layer, "Set Wake Time", NULL, NULL);
      break;
    case 2: 
      menu_cell_basic_draw(ctx, cell_layer, "Set Wake Temp", NULL, NULL);
      break;
    case 3:
      menu_cell_basic_draw(ctx, cell_layer, "Auto Body Temp", "OFF", NULL);
      break;
    case 4:
      menu_cell_basic_draw(ctx, cell_layer, "Nest Control", NULL, NULL);
      break;
  }
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  switch (cell_index->row) {
    case 0:
      auto_sleep_click();
      break;
    case 1:
      wake_window_init();
      break;
    case 2:
//       send_sleep(true);
      break;
  }
}

static void main_window_load(Window *window) {
  // Now we prepare to initialize the menu layer
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  // Initialize menu layer variables
  auto_sleep_on = true;
  strcpy(s_onoff, "ON");
  tick_timer_service_subscribe(MINUTE_UNIT, auto_sleep_tick_handler);

  // Create the menu layer
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  // Add layer to window
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void main_window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(s_menu_layer);
}

static void appmsg_in_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In dropped: %d", reason);
}

void init() {
  main_window = window_create();
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  window_stack_push(main_window, true);
  app_message_open(APPMSG_INBOUND_SIZE, APPMSG_OUTBOUND_SIZE);
  app_message_register_inbox_dropped(appmsg_in_dropped);
}

void deinit() {
  window_destroy(main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
  return 0;
}