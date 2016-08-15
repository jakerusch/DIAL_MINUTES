#include <pebble.h>
#define KEY_TEMP 0
#define KEY_ICON 1

static Window *s_main_window;
static Layer *s_dial_layer, *s_hands_layer, *s_temp_circle, *s_date_circle;
static TextLayer *s_temp_layer, *s_date_layer;
static BitmapLayer *s_weather_bitmap_layer;
static GBitmap *s_weather_bitmap;
static GPath *s_minute_arrow, *s_hour_arrow, *s_minute_filler, *s_hour_filler;
static int buf=8;
static GFont s_font;
static char icon_layer_buf[32];

static const GPathInfo MINUTE_HAND_POINTS = {
  5, (GPoint []) {
    {5, 16},
    {-5, 16},
    {-4, -64},
    {0, -70},
    {4, -64}
  }
};

static const GPathInfo MINUTE_HAND_FILLER = {
  4, (GPoint []) {
    {2, -16},
    {-2, -16},
    {-2, -60},
    {2, -60}
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  5, (GPoint []) {
    {5, 16},
    {-5, 16},
    {-4, -48}, 
    {0, -54},
    {4, -48}
  }
};

static const GPathInfo HOUR_HAND_FILLER = {
  4, (GPoint []) {
    {2, -16},
    {-2, -16},
    {-2, -44},
    {2, -44}
  }
};

static void hide_hands() {
  layer_set_hidden(s_hands_layer, true); 
}

static void show_hands() {
  layer_set_hidden(s_hands_layer, false);
}

/////////////////////////////////////////////////
// select click                                //
// hides hands for 2 seconds, then shows again //
/////////////////////////////////////////////////
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  hide_hands();
  app_timer_register(2000, show_hands, NULL);
}

///////////////////
// assign clicks //
///////////////////
static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

/////////////////////////
// draws dial on watch //
/////////////////////////
static void dial_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds); 
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, (bounds.size.w+buf)/2);
  
  // set number of tickmarks
  int tick_marks_number = 60;

  // tick mark lengths
  int tick_length_end = (bounds.size.w+buf)/2; 
  int tick_length_start;
  
  // set colors
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 6);
  
  for(int i=0; i<tick_marks_number; i++) {
    
    if(i%5==0) {
      graphics_context_set_stroke_width(ctx, 4);
      tick_length_start = tick_length_end-8;
    } else {
      graphics_context_set_stroke_width(ctx, 1);
      tick_length_start = tick_length_end-4;
    }

    int angle = TRIG_MAX_ANGLE * i/tick_marks_number;

    GPoint tick_mark_start = {
      .x = (int)(sin_lookup(angle) * (int)tick_length_start / TRIG_MAX_RATIO) + center.x,
      .y = (int)(-cos_lookup(angle) * (int)tick_length_start / TRIG_MAX_RATIO) + center.y,
    };
    
    GPoint tick_mark_end = {
      .x = (int)(sin_lookup(angle) * (int)tick_length_end / TRIG_MAX_RATIO) + center.x,
      .y = (int)(-cos_lookup(angle) * (int)tick_length_end / TRIG_MAX_RATIO) + center.y,
    };      
    
    graphics_draw_line(ctx, tick_mark_end, tick_mark_start);  
  } // end of loop 
}

static void temp_update_proc(Layer *layer, GContext *ctx) {
  GPoint center = GPoint(144/2, 46);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, center, 36/2);
}

/////////////////////////////////
// draw hands and update ticks //
/////////////////////////////////
static void ticks_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds); 
    
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  // set stroke to 1
  graphics_context_set_stroke_width(ctx, 1);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  
  // draw minute hand
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  // draw hour hand
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);
  
  // switch color for fillers
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);  
   
  // draw minute filler
  gpath_rotate_to(s_minute_filler, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_filler);
  gpath_draw_outline(ctx, s_minute_filler);
  
  // draw hour filler
  gpath_rotate_to(s_hour_filler, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_filler);
  gpath_draw_outline(ctx, s_hour_filler);

  // switch colors for center circle
  graphics_context_set_fill_color(ctx, GColorWhite);
  
  // circle overlay
  graphics_fill_circle(ctx, center, 7);
  
  // outline circle overlay
  graphics_draw_circle(ctx, center, 7);
  
  // dot in the middle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 1);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // set background color
  window_set_background_color(s_main_window, GColorBlack);
  
  // register button clicks
  window_set_click_config_provider(window, click_config_provider);
  
  s_font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  //////////////////////////////////
  // create canvas layer for dial //
  //////////////////////////////////
  s_dial_layer = layer_create(bounds);
  layer_set_update_proc(s_dial_layer, dial_update_proc);
  layer_add_child(window_layer, s_dial_layer);  
  
  ////////////////////////
  // create temp circle //
  ////////////////////////
  s_temp_circle = layer_create(bounds);
  layer_set_update_proc(s_temp_circle, temp_update_proc);
  layer_add_child(s_dial_layer, s_temp_circle);
  
  //////////////////////
  // create temp text //
  //////////////////////
  s_temp_layer = text_layer_create(GRect(60, 28, 24, 16));
  text_layer_set_background_color(s_temp_layer, GColorClear);
//   text_layer_set_text_color(s_temp_layer, GColorWhite);
  text_layer_set_text_alignment(s_temp_layer, GTextAlignmentCenter);
  text_layer_set_font(s_temp_layer, s_font);
  text_layer_set_text(s_temp_layer, "100");
  layer_add_child(s_dial_layer, text_layer_get_layer(s_temp_layer));
  
  ////////////////////////////
  // create temp icon layer //
  ////////////////////////////
  
  
//   ///////////////////////////////////
//   // create canvas layer for hands //
//   ///////////////////////////////////
//   s_hands_layer = layer_create(bounds);
//   layer_set_update_proc(s_hands_layer, ticks_update_proc);
//   layer_add_child(window_layer, s_hands_layer);
}

static void update_time() {
  // get a tm strucutre
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // write the current hours into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H" : "%I", tick_time);
  
  // write the current minutes
  static char s_buffer_small[8];
  strftime(s_buffer_small, sizeof(s_buffer_small), "%M", tick_time);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
  update_time();
}

static void main_window_unload(Window *window) {
  layer_destroy(s_hands_layer);
}

//////////////////////////////////////
// display appropriate weather icon //
//////////////////////////////////////
static void load_icons(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  // populate icon variable
    if(strcmp(icon_layer_buf, "clear-day")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_DAY_BLACK_ICON);  
    } else if(strcmp(icon_layer_buf, "clear-night")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLEAR_SKY_NIGHT_BLACK_ICON);
    }else if(strcmp(icon_layer_buf, "rain")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RAIN_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "snow")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SNOW_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "sleet")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SLEET_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "wind")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WIND_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "fog")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FOG_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "cloudy")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CLOUDY_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "partly-cloudy-day")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_DAY_BLACK_ICON);
    } else if(strcmp(icon_layer_buf, "partly-cloudy-night")==0) {
      s_weather_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PARTLY_CLOUDY_NIGHT_BLACK_ICON);
    }
  
  ///////////////////////////
  // populate weather icon //
  ///////////////////////////
  s_weather_bitmap_layer = bitmap_layer_create(GRect(60, 46, 24, 16));
  bitmap_layer_set_compositing_mode(s_weather_bitmap_layer, GCompOpSet);  
  bitmap_layer_set_bitmap(s_weather_bitmap_layer, s_weather_bitmap); 
  layer_add_child(window_layer, bitmap_layer_get_layer(s_weather_bitmap_layer)); 
}

///////////////////
// weather calls //
///////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temp_buf[8];
  static char icon_buf[32];
  static char temp_layer_buf[32];

  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMP);
  Tuple *icon_tuple = dict_find(iterator, KEY_ICON);  

  // If all data is available, use it
  if(temp_tuple && icon_tuple) {
    
    // temp
    snprintf(temp_buf, sizeof(temp_buf), "%d", (int)temp_tuple->value->int32);
    snprintf(temp_layer_buf, sizeof(temp_layer_buf), "%s", temp_buf);
    text_layer_set_text(s_temp_layer, temp_layer_buf);

    // icon
    snprintf(icon_buf, sizeof(icon_buf), "%s", icon_tuple->value->cstring);
    snprintf(icon_layer_buf, sizeof(icon_layer_buf), "%s", icon_buf);    
  }  
  
  load_icons(s_main_window);
  APP_LOG(APP_LOG_LEVEL_INFO, "inbox_received_callback");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

////////////////////
// initialize app //
////////////////////
static void init() {
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  // show window on the watch with animated=true
  window_stack_push(s_main_window, true);
  
  // subscribe to time events
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Make sure the time is displayed from the start
  update_time(); 
  
  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_minute_filler = gpath_create(&MINUTE_HAND_FILLER);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  s_hour_filler = gpath_create(&HOUR_HAND_FILLER);
  
  // move hands to proper locations
  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_minute_filler, center);
  gpath_move_to(s_hour_arrow, center);    
  gpath_move_to(s_hour_filler, center);     
  
  // Register weather callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  
  // Open AppMessage for weather callbacks
  const int inbox_size = 64;
  const int outbox_size = 64;
  app_message_open(inbox_size, outbox_size);  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Clock show_clock_window");  
}

///////////////////////
// de-initialize app //
///////////////////////
static void deinit() {
  window_destroy(s_main_window);
}

/////////////
// run app //
/////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
}