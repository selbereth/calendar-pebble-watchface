#include <pebble.h>

#define MAX_TEXT_LEN 64
#define EVENT_COUNT 3

#define PERSIST_KEY_TITLE_BASE 100
#define PERSIST_KEY_TIME_BASE 200

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_event_title_layers[EVENT_COUNT];
static TextLayer *s_event_time_layers[EVENT_COUNT];
static Layer *s_divider_layer;

static char s_time_buffer[8];
static char s_event_title_buffers[EVENT_COUNT][MAX_TEXT_LEN];
static char s_event_time_buffers[EVENT_COUNT][MAX_TEXT_LEN];

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  strftime(s_time_buffer, sizeof(s_time_buffer),
           clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_time_buffer);
}

static void refresh_event_layers(void) {
  for (int i = 0; i < EVENT_COUNT; i++) {
    text_layer_set_text(s_event_title_layers[i], s_event_title_buffers[i]);
    text_layer_set_text(s_event_time_layers[i], s_event_time_buffers[i]);
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

static const uint32_t s_title_keys[EVENT_COUNT] = {
  MESSAGE_KEY_EVENT_1_TITLE, MESSAGE_KEY_EVENT_2_TITLE, MESSAGE_KEY_EVENT_3_TITLE
};
static const uint32_t s_time_keys[EVENT_COUNT] = {
  MESSAGE_KEY_EVENT_1_TIME, MESSAGE_KEY_EVENT_2_TIME, MESSAGE_KEY_EVENT_3_TIME
};

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  bool any_event = false;

  for (int i = 0; i < EVENT_COUNT; i++) {
    Tuple *title_tuple = dict_find(iterator, s_title_keys[i]);
    Tuple *time_tuple = dict_find(iterator, s_time_keys[i]);

    if (title_tuple) {
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
  int time_height = (int)(bounds.size.h * 0.34);

  GRect time_frame = GRect(margin, PBL_IF_ROUND_ELSE(12, 4),
                            bounds.size.w - (2 * margin), time_height);
  s_time_layer = text_layer_create(time_frame);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  int divider_y = time_frame.origin.y + time_frame.size.h + 2;
  s_divider_layer = layer_create(GRect(margin, divider_y, bounds.size.w - (2 * margin), 1));
  layer_set_update_proc(s_divider_layer, divider_update_proc);
  layer_add_child(window_layer, s_divider_layer);

  int events_top = divider_y + 6;
  int events_height = bounds.size.h - events_top - 4;
  int row_height = events_height / EVENT_COUNT;

  for (int i = 0; i < EVENT_COUNT; i++) {
    GRect row_frame = GRect(margin, events_top + (i * row_height),
                             bounds.size.w - (2 * margin), row_height);

    GRect time_col = GRect(row_frame.origin.x, row_frame.origin.y, 48, row_frame.size.h);
    GRect title_col = GRect(row_frame.origin.x + 50, row_frame.origin.y,
                             row_frame.size.w - 50, row_frame.size.h);

    s_event_time_layers[i] = text_layer_create(time_col);
    text_layer_set_background_color(s_event_time_layers[i], GColorClear);
    text_layer_set_text_color(s_event_time_layers[i], GColorWhite);
    text_layer_set_font(s_event_time_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(s_event_time_layers[i], GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(s_event_time_layers[i]));

    s_event_title_layers[i] = text_layer_create(title_col);
    text_layer_set_background_color(s_event_title_layers[i], GColorClear);
    text_layer_set_text_color(s_event_title_layers[i], GColorWhite);
    text_layer_set_font(s_event_title_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_text_alignment(s_event_title_layers[i], GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(s_event_title_layers[i]));
  }

  refresh_event_layers();
  update_time();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  layer_destroy(s_divider_layer);
  for (int i = 0; i < EVENT_COUNT; i++) {
    text_layer_destroy(s_event_title_layers[i]);
    text_layer_destroy(s_event_time_layers[i]);
  }
}

static void init(void) {
  load_persisted_events();

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
