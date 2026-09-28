#include <pebble.h>

#define MAX_TEXT_LEN 64
#define EVENT_COUNT 11
// Gothic 18 needs about this much height; rows that would be shorter are hidden.
#define MIN_ROW_HEIGHT 22

#define PERSIST_KEY_TITLE_BASE 100
#define PERSIST_KEY_TIME_BASE 200
#define PERSIST_KEY_WEATHER 300
#define PERSIST_KEY_SUN 301
#define PERSIST_KEY_RANGE 302

#define INFO_LINE_HEIGHT 16
#define SUN_COL_WIDTH 32
#define RANGE_COL_WIDTH 24
#define TIME_HEIGHT 34

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_event_title_layers[EVENT_COUNT];
static TextLayer *s_event_time_layers[EVENT_COUNT];
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_range_layer;
static TextLayer *s_sun_layer;
static Layer *s_divider_layer;

static char s_time_buffer[8];
static char s_date_buffer[16];
static char s_weather_buffer[MAX_TEXT_LEN];
static char s_sun_buffer[MAX_TEXT_LEN];
static char s_range_buffer[MAX_TEXT_LEN];
static char s_event_title_buffers[EVENT_COUNT][MAX_TEXT_LEN];
static char s_event_time_buffers[EVENT_COUNT][MAX_TEXT_LEN];

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  // Always 12-hour, no AM/PM, no leading zero (the digits font has no space glyph).
  strftime(s_time_buffer, sizeof(s_time_buffer), "%I:%M", tick_time);
  const char *time_text = s_time_buffer[0] == '0' ? s_time_buffer + 1 : s_time_buffer;
  text_layer_set_text(s_time_layer, time_text);

  strftime(s_date_buffer, sizeof(s_date_buffer), "%a %b %e", tick_time);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void refresh_event_layers(void) {
  for (int i = 0; i < EVENT_COUNT; i++) {
    text_layer_set_text(s_event_title_layers[i], s_event_title_buffers[i]);
    text_layer_set_text(s_event_time_layers[i], s_event_time_buffers[i]);
  }
}

static void load_persisted_info(void) {
  s_weather_buffer[0] = '\0';
  s_sun_buffer[0] = '\0';
  s_range_buffer[0] = '\0';
  if (persist_exists(PERSIST_KEY_RANGE)) {
    persist_read_string(PERSIST_KEY_RANGE, s_range_buffer, MAX_TEXT_LEN);
  }
  if (persist_exists(PERSIST_KEY_WEATHER)) {
    persist_read_string(PERSIST_KEY_WEATHER, s_weather_buffer, MAX_TEXT_LEN);
  }
  if (persist_exists(PERSIST_KEY_SUN)) {
    persist_read_string(PERSIST_KEY_SUN, s_sun_buffer, MAX_TEXT_LEN);
  }
}

static void load_persisted_events(void) {
  bool any_event = false;
  for (int i = 0; i < EVENT_COUNT; i++) {
    if (persist_exists(PERSIST_KEY_TITLE_BASE + i)) {
      persist_read_string(PERSIST_KEY_TITLE_BASE + i, s_event_title_buffers[i], MAX_TEXT_LEN);
      persist_read_string(PERSIST_KEY_TIME_BASE + i, s_event_time_buffers[i], MAX_TEXT_LEN);
      if (strlen(s_event_title_buffers[i]) > 0) {
        any_event = true;
      }
    } else {
      s_event_title_buffers[i][0] = '\0';
      s_event_time_buffers[i][0] = '\0';
    }
  }
  if (!any_event) {
    strncpy(s_event_title_buffers[0], "No upcoming events", MAX_TEXT_LEN);
    s_event_time_buffers[0][0] = '\0';
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static uint32_t s_title_keys[EVENT_COUNT];
static uint32_t s_time_keys[EVENT_COUNT];

static void init_message_keys(void) {
  s_title_keys[0] = MESSAGE_KEY_EVENT_1_TITLE;
  s_title_keys[1] = MESSAGE_KEY_EVENT_2_TITLE;
  s_title_keys[2] = MESSAGE_KEY_EVENT_3_TITLE;
  s_title_keys[3] = MESSAGE_KEY_EVENT_4_TITLE;
  s_title_keys[4] = MESSAGE_KEY_EVENT_5_TITLE;
  s_title_keys[5] = MESSAGE_KEY_EVENT_6_TITLE;
  s_title_keys[6] = MESSAGE_KEY_EVENT_7_TITLE;
  s_title_keys[7] = MESSAGE_KEY_EVENT_8_TITLE;
  s_title_keys[8] = MESSAGE_KEY_EVENT_9_TITLE;
  s_title_keys[9] = MESSAGE_KEY_EVENT_10_TITLE;
  s_title_keys[10] = MESSAGE_KEY_EVENT_11_TITLE;

  s_time_keys[0] = MESSAGE_KEY_EVENT_1_TIME;
  s_time_keys[1] = MESSAGE_KEY_EVENT_2_TIME;
  s_time_keys[2] = MESSAGE_KEY_EVENT_3_TIME;
  s_time_keys[3] = MESSAGE_KEY_EVENT_4_TIME;
  s_time_keys[4] = MESSAGE_KEY_EVENT_5_TIME;
  s_time_keys[5] = MESSAGE_KEY_EVENT_6_TIME;
  s_time_keys[6] = MESSAGE_KEY_EVENT_7_TIME;
  s_time_keys[7] = MESSAGE_KEY_EVENT_8_TIME;
  s_time_keys[8] = MESSAGE_KEY_EVENT_9_TIME;
  s_time_keys[9] = MESSAGE_KEY_EVENT_10_TIME;
  s_time_keys[10] = MESSAGE_KEY_EVENT_11_TIME;
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  bool any_event = false;
  bool got_events = false;

  Tuple *weather_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER);
  if (weather_tuple) {
    strncpy(s_weather_buffer, weather_tuple->value->cstring, MAX_TEXT_LEN);
    s_weather_buffer[MAX_TEXT_LEN - 1] = '\0';
    persist_write_string(PERSIST_KEY_WEATHER, s_weather_buffer);
    text_layer_set_text(s_weather_layer, s_weather_buffer);
  }
  Tuple *range_tuple = dict_find(iterator, MESSAGE_KEY_RANGE);
  if (range_tuple) {
    strncpy(s_range_buffer, range_tuple->value->cstring, MAX_TEXT_LEN);
    s_range_buffer[MAX_TEXT_LEN - 1] = '\0';
    persist_write_string(PERSIST_KEY_RANGE, s_range_buffer);
    text_layer_set_text(s_range_layer, s_range_buffer);
  }
  Tuple *sun_tuple = dict_find(iterator, MESSAGE_KEY_SUN);
  if (sun_tuple) {
    strncpy(s_sun_buffer, sun_tuple->value->cstring, MAX_TEXT_LEN);
    s_sun_buffer[MAX_TEXT_LEN - 1] = '\0';
    persist_write_string(PERSIST_KEY_SUN, s_sun_buffer);
    text_layer_set_text(s_sun_layer, s_sun_buffer);
  }

  for (int i = 0; i < EVENT_COUNT; i++) {
    Tuple *title_tuple = dict_find(iterator, s_title_keys[i]);
    Tuple *time_tuple = dict_find(iterator, s_time_keys[i]);

    if (title_tuple) {
      got_events = true;
      strncpy(s_event_title_buffers[i], title_tuple->value->cstring, MAX_TEXT_LEN);
      s_event_title_buffers[i][MAX_TEXT_LEN - 1] = '\0';
      persist_write_string(PERSIST_KEY_TITLE_BASE + i, s_event_title_buffers[i]);
      if (strlen(s_event_title_buffers[i]) > 0) {
        any_event = true;
      }
    }
    if (time_tuple) {
      strncpy(s_event_time_buffers[i], time_tuple->value->cstring, MAX_TEXT_LEN);
      s_event_time_buffers[i][MAX_TEXT_LEN - 1] = '\0';
      persist_write_string(PERSIST_KEY_TIME_BASE + i, s_event_time_buffers[i]);
    }
  }

  if (!got_events) {
    return;
  }

  if (!any_event) {
    strncpy(s_event_title_buffers[0], "No upcoming events", MAX_TEXT_LEN);
    s_event_time_buffers[0][0] = '\0';
  }

  refresh_event_layers();
}

static void divider_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 1), 0, GCornerNone);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorBlack);

  int margin = PBL_IF_ROUND_ELSE(20, 6);
  int time_height = TIME_HEIGHT;
  int content_w = bounds.size.w - (2 * margin);
  int top = PBL_IF_ROUND_ELSE(12, 2);

  int hdr_margin = PBL_IF_ROUND_ELSE(20, 2);
  int hdr_w = bounds.size.w - (2 * hdr_margin);

  // Top line: date on the left, current temperature on the right.
  s_date_layer = text_layer_create(GRect(hdr_margin, top, hdr_w / 2, INFO_LINE_HEIGHT));
  s_weather_layer = text_layer_create(GRect(hdr_margin + hdr_w / 2, top,
                                            hdr_w - hdr_w / 2, INFO_LINE_HEIGHT));
  TextLayer *info_layers[] = { s_date_layer, s_weather_layer };
  for (int i = 0; i < 2; i++) {
    text_layer_set_background_color(info_layers[i], GColorClear);
    text_layer_set_text_color(info_layers[i], GColorWhite);
    text_layer_set_font(info_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(info_layers[i]));
  }
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
  text_layer_set_text(s_weather_layer, s_weather_buffer);

  // Time row: sunrise over sunset | time | high over low.
  int row_y = top + INFO_LINE_HEIGHT;
  int time_x = hdr_margin + SUN_COL_WIDTH;
  int time_w = hdr_w - SUN_COL_WIDTH - RANGE_COL_WIDTH;
  GRect time_frame = GRect(time_x, row_y, time_w, time_height);
  s_sun_layer = text_layer_create(GRect(hdr_margin, row_y, SUN_COL_WIDTH, time_height));
  text_layer_set_font(s_sun_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_sun_layer, GTextAlignmentLeft);
  text_layer_set_text(s_sun_layer, s_sun_buffer);
  s_range_layer = text_layer_create(GRect(time_x + time_w, row_y, RANGE_COL_WIDTH, time_height));
  text_layer_set_font(s_range_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_range_layer, GTextAlignmentRight);
  text_layer_set_text(s_range_layer, s_range_buffer);
  TextLayer *side_layers[] = { s_sun_layer, s_range_layer };
  for (int i = 0; i < 2; i++) {
    text_layer_set_background_color(side_layers[i], GColorClear);
    text_layer_set_text_color(side_layers[i], GColorWhite);
    layer_add_child(window_layer, text_layer_get_layer(side_layers[i]));
  }

  s_time_layer = text_layer_create(time_frame);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  int divider_y = time_frame.origin.y + time_frame.size.h + 2;
  s_divider_layer = layer_create(GRect(margin, divider_y, bounds.size.w - (2 * margin), 1));
  layer_set_update_proc(s_divider_layer, divider_update_proc);
  layer_add_child(window_layer, s_divider_layer);

  int events_top = divider_y + 6;
  int events_height = bounds.size.h - events_top - 4;
  int visible_rows = events_height / MIN_ROW_HEIGHT;
  if (visible_rows > EVENT_COUNT) {
    visible_rows = EVENT_COUNT;
  }
  if (visible_rows < 1) {
    visible_rows = 1;
  }
  int row_height = events_height / visible_rows;

  for (int i = 0; i < EVENT_COUNT; i++) {
    GRect row_frame = GRect(margin, events_top + (i * row_height),
                             bounds.size.w - (2 * margin), row_height);

    GRect time_col = GRect(row_frame.origin.x, row_frame.origin.y, 58, row_frame.size.h);
    GRect title_col = GRect(row_frame.origin.x + 60, row_frame.origin.y,
                             row_frame.size.w - 60, row_frame.size.h);

    s_event_time_layers[i] = text_layer_create(time_col);
    text_layer_set_background_color(s_event_time_layers[i], GColorClear);
    text_layer_set_text_color(s_event_time_layers[i], GColorWhite);
    text_layer_set_font(s_event_time_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_event_time_layers[i], GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(s_event_time_layers[i]));

    s_event_title_layers[i] = text_layer_create(title_col);
    text_layer_set_background_color(s_event_title_layers[i], GColorClear);
    text_layer_set_text_color(s_event_title_layers[i], GColorWhite);
    text_layer_set_font(s_event_title_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_event_title_layers[i], GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(s_event_title_layers[i]));

    if (i >= visible_rows) {
      layer_set_hidden(text_layer_get_layer(s_event_time_layers[i]), true);
      layer_set_hidden(text_layer_get_layer(s_event_title_layers[i]), true);
    }
  }

  refresh_event_layers();
  update_time();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_sun_layer);
  text_layer_destroy(s_range_layer);
  layer_destroy(s_divider_layer);
  for (int i = 0; i < EVENT_COUNT; i++) {
    text_layer_destroy(s_event_title_layers[i]);
    text_layer_destroy(s_event_time_layers[i]);
  }
}

static void init(void) {
  init_message_keys();
  load_persisted_events();
  load_persisted_info();

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
