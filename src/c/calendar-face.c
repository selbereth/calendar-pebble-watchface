#include <pebble.h>
#include <ctype.h>

#define MAX_TEXT_LEN 64
#define EVENT_COUNT 11
// Gothic 18 needs about this much height; rows that would be shorter are hidden.
#define MIN_ROW_HEIGHT 22

#define PERSIST_KEY_TITLE_BASE 100
#define PERSIST_KEY_TIME_BASE 200
#define PERSIST_KEY_WEATHER 300
#define PERSIST_KEY_SUN 301
#define PERSIST_KEY_RANGE 302
#define PERSIST_KEY_SETTINGS 310

#define INFO_LINE_HEIGHT 16
#define DATE_SLACK 1
#define MIN_SUN_COL_WIDTH 36  // fits "06:58"
#define COMPACT_TIME_EXTRA 4  // the clock font is a little taller than two info lines
#define TIME_SLACK 2
#define RIGHT_COL_WIDTH 24
#define TIME_HEIGHT 48  // three info lines tall
#define NARROW_TIME_HEIGHT 46  // Bitham 42 needs about this much when it has its own row

// User settings, sent from the phone's settings page and persisted on the watch.
enum { TIME_12H = 0, TIME_24H = 1, TIME_AUTO = 2 };
enum { DATE_WORDS = 0, DATE_NUMBERS = 1 };
enum { SEP_SLASH = 0, SEP_DOT = 1, SEP_DASH = 2 };
enum { EVENTS_SAME = 0, EVENTS_12H = 1, EVENTS_24H = 2 };  // event times: follow the clock or force

typedef struct {
  uint8_t time_format;   // TIME_*
  uint8_t date_style;    // DATE_*
  uint8_t day_first;     // 0 = month first ("Nov 9", "11/9"), 1 = day first ("9 Nov", "9/11")
  uint8_t date_sep;      // SEP_*, numeric dates only
  uint8_t show_weekday;  // 0/1
  uint8_t invert;        // 0 = white on black, 1 = black on white
  uint8_t event_format;  // EVENTS_* 
} Settings;

static Settings s_settings = { TIME_12H, DATE_WORDS, 0, SEP_SLASH, 1, 0, EVENTS_SAME };
static GColor s_fg, s_bg;

static bool use_24h(void) {
  return s_settings.time_format == TIME_24H ||
         (s_settings.time_format == TIME_AUTO && clock_is_24h_style());
}

static bool event_24h(void) {
  return s_settings.event_format == EVENTS_SAME ? use_24h() : s_settings.event_format == EVENTS_24H;
}

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
static char s_date_buffer[32];
static char s_event_time_display[EVENT_COUNT][12];
static char s_weather_buffer[MAX_TEXT_LEN];
static char s_sun_buffer[MAX_TEXT_LEN];
static char s_range_buffer[MAX_TEXT_LEN];
static char s_event_title_buffers[EVENT_COUNT][MAX_TEXT_LEN];
static char s_event_time_buffers[EVENT_COUNT][MAX_TEXT_LEN];

static int text_width(const char *text, GFont font) {
  return graphics_text_layout_get_content_size(text, font, GRect(0, 0, 400, TIME_HEIGHT),
      GTextOverflowModeFill, GTextAlignmentLeft).w;
}

// From this 12-hour hour on, the date drops its weekday to give the clock more room.
#define LONG_HOUR_START 10

// Width of the widest real time in this font, for hours first_hour..last_hour. 12-hour times
// have no leading zero (the digits font has no space glyph); 24-hour times keep it.
static int widest_time_width(GFont font, int first_hour, int last_hour) {
  int widest = 0;
  char text[8];
  const char *fmt = use_24h() ? "%02d:%02d" : "%d:%02d";
  for (int h = first_hour; h <= last_hour; h++) {
    for (int m = 0; m < 60; m++) {
      snprintf(text, sizeof(text), fmt, h, m);
      int w = text_width(text, font);
      if (w > widest) {
        widest = w;
      }
    }
  }
  return widest;
}

// Joins the date parts in the user's order: "Wed Nov 9", "Wed 9 Nov", "Wed 11/9", "Wed 9.11.".
static void compose_date(char *out, size_t n, const char *weekday, const char *month,
                         const char *day) {
  const bool numbers = s_settings.date_style == DATE_NUMBERS;
  const char *first = s_settings.day_first ? day : month;
  const char *second = s_settings.day_first ? month : day;
  const char *sep = " ";
  const char *tail = "";
  if (numbers) {
    sep = s_settings.date_sep == SEP_DOT ? "." : s_settings.date_sep == SEP_DASH ? "-" : "/";
    tail = s_settings.date_sep == SEP_DOT ? "." : "";
  }
  snprintf(out, n, "%s%s%s%s%s%s", weekday ? weekday : "", weekday ? " " : "", first, sep, second,
           tail);
}

// Name of the month in t (or its number when the date is numeric), written to out.
static void month_text(char *out, size_t n, const struct tm *t) {
  if (s_settings.date_style == DATE_NUMBERS) {
    snprintf(out, n, "%d", t->tm_mon + 1);
  } else {
    strftime(out, n, "%b", t);
  }
}

// Width of the widest date the face can show: the widest weekday, month and day, joined in the
// current format. Names come from strftime so a non-English locale is measured too.
static int widest_date_width(GFont font, bool with_weekday) {
  char name[16], widest_day[16] = "", widest_month[16] = "", widest_weekday[16] = "";
  int widest_day_w = 0, widest_month_w = 0, widest_weekday_w = 0;
  struct tm t = { 0 };
  for (int i = 0; with_weekday && i < 7; i++) {
    t.tm_wday = i;
    strftime(name, sizeof(name), "%a", &t);
    int w = text_width(name, font);
    if (w > widest_weekday_w) {
      widest_weekday_w = w;
      strcpy(widest_weekday, name);
    }
  }
  for (int i = 0; i < 12; i++) {
    t.tm_mon = i;
    month_text(name, sizeof(name), &t);
    int w = text_width(name, font);
    if (w > widest_month_w) {
      widest_month_w = w;
      strcpy(widest_month, name);
    }
  }
  for (int d = 1; d <= 31; d++) {
    snprintf(name, sizeof(name), "%d", d);
    int w = text_width(name, font);
    if (w > widest_day_w) {
      widest_day_w = w;
      strcpy(widest_day, name);
    }
  }
  char date[48];
  compose_date(date, sizeof(date), with_weekday ? widest_weekday : NULL, widest_month, widest_day);
  return text_width(date, font);
}

// Event times arrive from the phone as "09:30", "+09:30" (a later day) or text such as "All day".
// Re-write the clock ones for the 12-hour setting ("9:30a"); everything else is shown as is.
static void format_event_time(char *out, size_t n, const char *in) {
  const char *p = in;
  bool later = *p == '+';
  if (later) {
    p++;
  }
  if (!event_24h() && strlen(p) == 5 && p[2] == ':' && isdigit((int)p[0]) && isdigit((int)p[1]) &&
      isdigit((int)p[3]) && isdigit((int)p[4])) {
    int h = (p[0] - '0') * 10 + (p[1] - '0');
    snprintf(out, n, "%s%d:%c%c%c", later ? "+" : "", h % 12 == 0 ? 12 : h % 12, p[3], p[4],
             h < 12 ? 'a' : 'p');
  } else {
    snprintf(out, n, "%s", in);
  }
}

// Width of the widest event time the column can hold, in the 12- or 24-hour form.
static int widest_event_time_width(GFont font) {
  int widest = 0;
  char in[8], out[12];
  for (int h = 0; h < 24; h++) {
    for (int m = 0; m < 60; m += 11) {  // digit widths repeat; this still covers every digit
      snprintf(in, sizeof(in), "+%02d:%02d", h, m);
      format_event_time(out, sizeof(out), in);
      int w = text_width(out, font);
      if (w > widest) {
        widest = w;
      }
    }
  }
  return widest;
}

// Compact header on every square screen: sun times top-left, clock two info lines tall at the
// right, and the date and weather sharing the third line. Round screens stack instead.
#define COMPACT_HEADER PBL_IF_ROUND_ELSE(0, 1)
static bool s_compact;

// Normal layout: "now" and "high/low" sit in their own column. Compact layout: one line,
// "58° 64/47".
static void refresh_weather_text(void) {
  if (!s_compact) {
    text_layer_set_text(s_weather_layer, s_weather_buffer);
    text_layer_set_text(s_range_layer, s_range_buffer);
    return;
  }
  static char line[MAX_TEXT_LEN];
  int n = snprintf(line, sizeof(line), "%s ", s_weather_buffer);
  for (const char *c = s_range_buffer; *c && n < (int)sizeof(line) - 1; c++) {
    if (*c == '\n') {
      line[n++] = '/';
    } else if ((unsigned char)*c != 0xC2 && (unsigned char)*c != 0xB0) {  // drop the degree sign
      line[n++] = *c;
    }
  }
  line[n] = '\0';
  text_layer_set_text(s_weather_layer, line);
}

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);

  int hour12 = tick_time->tm_hour % 12 == 0 ? 12 : tick_time->tm_hour % 12;
  if (use_24h()) {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M", tick_time);
    text_layer_set_text(s_time_layer, s_time_buffer);
  } else {
    // 12-hour, no AM/PM, no leading zero (the digits font has no space glyph).
    strftime(s_time_buffer, sizeof(s_time_buffer), "%I:%M", tick_time);
    text_layer_set_text(s_time_layer,
                        s_time_buffer[0] == '0' ? s_time_buffer + 1 : s_time_buffer);
  }

  // Long clock hours (10 and later) drop the weekday to give the clock more room.
  int hour = use_24h() ? tick_time->tm_hour : hour12;
  bool weekday = s_settings.show_weekday && (s_compact || hour < LONG_HOUR_START);
  char weekday_text[16], month[16], day[16];
  strftime(weekday_text, sizeof(weekday_text), "%a", tick_time);
  month_text(month, sizeof(month), tick_time);
  snprintf(day, sizeof(day), "%d", tick_time->tm_mday);
  compose_date(s_date_buffer, sizeof(s_date_buffer), weekday ? weekday_text : NULL, month, day);
  text_layer_set_text(s_date_layer, s_date_buffer);
}

static void refresh_event_layers(void) {
  for (int i = 0; i < EVENT_COUNT; i++) {
    text_layer_set_text(s_event_title_layers[i], s_event_title_buffers[i]);
    format_event_time(s_event_time_display[i], sizeof(s_event_time_display[i]),
                      s_event_time_buffers[i]);
    text_layer_set_text(s_event_time_layers[i], s_event_time_display[i]);
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

static void main_window_load(Window *window);
static void main_window_unload(Window *window);

// Applies any settings in the message. Layout depends on them (clock and date widths, colours),
// so a change rebuilds the whole window.
static void apply_settings(DictionaryIterator *iterator) {
  Settings next = s_settings;
  Tuple *t;
  if ((t = dict_find(iterator, MESSAGE_KEY_TIME_FORMAT))) next.time_format = t->value->int32;
  if ((t = dict_find(iterator, MESSAGE_KEY_DATE_STYLE))) next.date_style = t->value->int32;
  if ((t = dict_find(iterator, MESSAGE_KEY_DATE_DAY_FIRST))) next.day_first = t->value->int32 != 0;
  if ((t = dict_find(iterator, MESSAGE_KEY_DATE_SEPARATOR))) next.date_sep = t->value->int32;
  if ((t = dict_find(iterator, MESSAGE_KEY_SHOW_WEEKDAY))) next.show_weekday = t->value->int32 != 0;
  if ((t = dict_find(iterator, MESSAGE_KEY_INVERT))) next.invert = t->value->int32 != 0;
  if ((t = dict_find(iterator, MESSAGE_KEY_EVENT_TIME_FORMAT))) next.event_format = t->value->int32;
  if (next.event_format > EVENTS_24H) next.event_format = EVENTS_SAME;
  if (next.time_format > TIME_AUTO) next.time_format = TIME_12H;
  if (next.date_style > DATE_NUMBERS) next.date_style = DATE_WORDS;
  if (next.date_sep > SEP_DASH) next.date_sep = SEP_SLASH;
  if (memcmp(&next, &s_settings, sizeof(next)) == 0) {
    return;
  }
  s_settings = next;
  persist_write_data(PERSIST_KEY_SETTINGS, &s_settings, sizeof(s_settings));
  main_window_unload(s_main_window);
  main_window_load(s_main_window);
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  bool any_event = false;
  bool got_events = false;

  apply_settings(iterator);

  Tuple *weather_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER);
  if (weather_tuple) {
    strncpy(s_weather_buffer, weather_tuple->value->cstring, MAX_TEXT_LEN);
    s_weather_buffer[MAX_TEXT_LEN - 1] = '\0';
    persist_write_string(PERSIST_KEY_WEATHER, s_weather_buffer);
    refresh_weather_text();
  }
  Tuple *range_tuple = dict_find(iterator, MESSAGE_KEY_RANGE);
  if (range_tuple) {
    strncpy(s_range_buffer, range_tuple->value->cstring, MAX_TEXT_LEN);
    s_range_buffer[MAX_TEXT_LEN - 1] = '\0';
    persist_write_string(PERSIST_KEY_RANGE, s_range_buffer);
    refresh_weather_text();
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
  graphics_context_set_fill_color(ctx, s_fg);
  graphics_fill_rect(ctx, GRect(0, 0, bounds.size.w, 1), 0, GCornerNone);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_fg = s_settings.invert ? GColorBlack : GColorWhite;
  s_bg = s_settings.invert ? GColorWhite : GColorBlack;
  window_set_background_color(window, s_bg);

  int margin = PBL_IF_ROUND_ELSE(bounds.size.w / 9, 6);  // round: inset scales with the bezel
  int top = PBL_IF_ROUND_ELSE(12, 2);

  int hdr_margin = PBL_IF_ROUND_ELSE(bounds.size.w / 9, 2);
  int hdr_w = bounds.size.w - (2 * hdr_margin);

  // Same layout on every square screen: date and sun on the left, clock in the middle,
  // weather on the right. Only the clock font changes: take the biggest one that fits at the
  // widest time (checked over all 720) beside the widest date, with a pixel or two of slack.
  // Hours 1-9 show the full date; hours 10-12 have the longest times, so the date loses its
  // weekday and the clock gets that room. Round screens have no room beside the clock, so
  // they stack (below).
  GFont date_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  int short_date_w = widest_date_width(date_font, false) + DATE_SLACK;
  int date_w = s_settings.show_weekday ? widest_date_width(date_font, true) + DATE_SLACK
                                       : short_date_w;
  // 12-hour clocks run 1..12; 24-hour clocks 0..23 with a leading zero. Either way the "long"
  // hours start at 10, where the clock is widest.
  const int first_hour = use_24h() ? 0 : 1;
  const int last_hour = use_24h() ? 23 : 12;
  const char *time_font_keys[] = { FONT_KEY_BITHAM_42_BOLD, FONT_KEY_BITHAM_34_MEDIUM_NUMBERS,
                                   FONT_KEY_LECO_32_BOLD_NUMBERS, FONT_KEY_GOTHIC_28_BOLD,
                                   FONT_KEY_GOTHIC_24_BOLD, FONT_KEY_GOTHIC_18_BOLD };
  const int font_count = sizeof(time_font_keys) / sizeof(time_font_keys[0]);
  int room = hdr_w - RIGHT_COL_WIDTH;
  GFont time_font = fonts_get_system_font(time_font_keys[0]);
  int time_h = TIME_HEIGHT;
  int time_w = hdr_w;
  int left_w = 0;
  int time_y = top;
  bool inline_time = false;
  bool compact = false;
  if (COMPACT_HEADER) {
    // Biggest clock that stays two info lines tall and leaves room for the sun times.
    GFont f = fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
    int w = widest_time_width(f, first_hour, last_hour) + TIME_SLACK;
    if (hdr_w - w < MIN_SUN_COL_WIDTH) {
      f = fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS);
      w = widest_time_width(f, first_hour, last_hour) + TIME_SLACK;
    }
    time_font = f;
    time_w = w;
    time_h = (2 * INFO_LINE_HEIGHT) + COMPACT_TIME_EXTRA;
    time_y = top - (COMPACT_TIME_EXTRA / 2);
    left_w = hdr_w - w;
    inline_time = true;
    compact = true;
  }
  s_compact = compact;
  for (int i = 0; i < font_count && !compact && !PBL_IF_ROUND_ELSE(true, false); i++) {
    GFont f = fonts_get_system_font(time_font_keys[i]);
    int short_hours_w = widest_time_width(f, first_hour, LONG_HOUR_START - 1) + TIME_SLACK;
    int long_hours_w = widest_time_width(f, LONG_HOUR_START, last_hour) + TIME_SLACK;
    time_font = f;
    inline_time = true;
    // The clock box starts where the short date ends; hours 1-9 are narrow enough to stay
    // clear of the full date, hours 10-12 of the short one.
    left_w = short_date_w;
    time_w = room - short_date_w;
    if (short_hours_w + date_w <= room && long_hours_w + short_date_w <= room) {
      break;  // else keep shrinking; the smallest font is used if none fit
    }
  }
  if (inline_time && !compact) {
    // A smaller font is shorter than the three-line header: centre the clock vertically.
    int text_h = graphics_text_layout_get_content_size("0:00", time_font,
        GRect(0, 0, hdr_w, TIME_HEIGHT), GTextOverflowModeFill, GTextAlignmentRight).h;
    time_y = top + (text_h < TIME_HEIGHT ? (TIME_HEIGHT - text_h) / 2 : 0);
  }
  int right_w = RIGHT_COL_WIDTH;
  int time_x = hdr_margin + left_w;
  int info_top = top;
  GTextAlignment time_align = GTextAlignmentRight;
  if (!inline_time) {
    time_h = NARROW_TIME_HEIGHT;
    time_x = hdr_margin;
    time_align = GTextAlignmentCenter;
    info_top = top + time_h;
    left_w = hdr_w / 2;
    right_w = hdr_w - left_w;
  }
  int right_x = hdr_margin + hdr_w - right_w;
  if (inline_time) {
    right_x = time_x + time_w;
  }

  // Header is three info lines tall, in three columns:
  //   date / sunrise / sunset  |  time (spans all three lines)  |  now / high / low
  s_date_layer = text_layer_create(GRect(hdr_margin, info_top, inline_time ? date_w : left_w,
                                         INFO_LINE_HEIGHT));
  s_sun_layer = text_layer_create(GRect(hdr_margin, info_top + INFO_LINE_HEIGHT, left_w,
                                        2 * INFO_LINE_HEIGHT));
  s_weather_layer = text_layer_create(GRect(right_x, info_top, right_w, INFO_LINE_HEIGHT));
  s_range_layer = text_layer_create(GRect(right_x, info_top + INFO_LINE_HEIGHT, right_w,
                                          2 * INFO_LINE_HEIGHT));
  TextLayer *info_layers[] = { s_date_layer, s_sun_layer, s_weather_layer, s_range_layer };
  for (int i = 0; i < 4; i++) {
    text_layer_set_background_color(info_layers[i], GColorClear);
    text_layer_set_text_color(info_layers[i], s_fg);
    layer_add_child(window_layer, text_layer_get_layer(info_layers[i]));
  }
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_font(s_sun_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_font(s_range_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_sun_layer, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
  text_layer_set_text_alignment(s_range_layer, GTextAlignmentRight);
  text_layer_set_text(s_sun_layer, s_sun_buffer);
  if (compact) {
    int row3 = top + (2 * INFO_LINE_HEIGHT);
    layer_set_frame(text_layer_get_layer(s_sun_layer),
                    GRect(hdr_margin, top, left_w, 2 * INFO_LINE_HEIGHT));
    layer_set_frame(text_layer_get_layer(s_date_layer),
                    GRect(hdr_margin, row3, date_w, INFO_LINE_HEIGHT));
    layer_set_frame(text_layer_get_layer(s_weather_layer),
                    GRect(hdr_margin + date_w, row3, hdr_w - date_w, INFO_LINE_HEIGHT));
    layer_set_hidden(text_layer_get_layer(s_range_layer), true);
  }
  refresh_weather_text();

  GRect time_frame = GRect(time_x, time_y, time_w, time_h);
  s_time_layer = text_layer_create(time_frame);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, s_fg);
  text_layer_set_font(s_time_layer, time_font);
  text_layer_set_text_alignment(s_time_layer, time_align);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  int header_bottom = info_top + 3 * INFO_LINE_HEIGHT;
  int divider_y = header_bottom + 2;
  s_divider_layer = layer_create(GRect(margin, divider_y, bounds.size.w - (2 * margin), 1));
  layer_set_update_proc(s_divider_layer, divider_update_proc);
  layer_add_child(window_layer, s_divider_layer);

  int events_top = divider_y + 6;
  int events_height = bounds.size.h - events_top - PBL_IF_ROUND_ELSE(bounds.size.h / 9, 4);
  int visible_rows = events_height / MIN_ROW_HEIGHT;
  if (visible_rows > EVENT_COUNT) {
    visible_rows = EVENT_COUNT;
  }
  if (visible_rows < 1) {
    visible_rows = 1;
  }
  int row_height = events_height / visible_rows;

  // 58px fits the 24-hour "+09:30"; the 12-hour "+12:30p" can need more.
  int event_time_w = widest_event_time_width(fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)) + 2;
  if (event_time_w < 58) {
    event_time_w = 58;
  }

  for (int i = 0; i < EVENT_COUNT; i++) {
    GRect row_frame = GRect(margin, events_top + (i * row_height),
                             bounds.size.w - (2 * margin), row_height);

    GRect time_col = GRect(row_frame.origin.x, row_frame.origin.y, event_time_w, row_frame.size.h);
    GRect title_col = GRect(row_frame.origin.x + event_time_w + 2, row_frame.origin.y,
                             row_frame.size.w - event_time_w - 2, row_frame.size.h);

    s_event_time_layers[i] = text_layer_create(time_col);
    text_layer_set_background_color(s_event_time_layers[i], GColorClear);
    text_layer_set_text_color(s_event_time_layers[i], s_fg);
    text_layer_set_font(s_event_time_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_event_time_layers[i], GTextAlignmentLeft);
    layer_add_child(window_layer, text_layer_get_layer(s_event_time_layers[i]));

    s_event_title_layers[i] = text_layer_create(title_col);
    text_layer_set_background_color(s_event_title_layers[i], GColorClear);
    text_layer_set_text_color(s_event_title_layers[i], s_fg);
    text_layer_set_font(s_event_title_layers[i], fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
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

static void load_persisted_settings(void) {
  Settings saved;
  if (persist_read_data(PERSIST_KEY_SETTINGS, &saved, sizeof(saved)) == (int)sizeof(saved)) {
    s_settings = saved;
  }
}

static void init(void) {
  init_message_keys();
  load_persisted_settings();
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
