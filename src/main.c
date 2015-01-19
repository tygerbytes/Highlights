#include <pebble.h>

const short MAX_RUN_MINUTES = 10;
const short MAX_COMMENTS = 30;

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_timer_layer;
static GFont *s_timer_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

int s_seconds_ellapsed = 0;

bool isPaused = true;

// TODO: Replace magic number with MAX_COMMENTS
short comments[ 30 ]; 
short comments_total = 0;

struct time {
	short minutes;
	short seconds;
};


/* -- Method declarations -- */

static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

void handle_scheduled_vibrations(struct time t);

void set_paused(bool pause);

void update_timer_display(struct time t);

void update_progress_bars_display(Layer *layer, GContext *ctx);

void click_config_provider(Window *window);

void select_single_click_handler(ClickRecognizerRef recognizer, void *context);

void up_single_click_handler(ClickRecognizerRef recognizer, void *context);

void add_comment();


/* -- Method implementations -- */

void handle_scheduled_vibrations(struct time t) {
	
	// Short vibe 15 seconds before 2 and 8 minute marks
	if (t.seconds == 45) {
		switch (t.minutes) {
			case 1 :
			case 7 :
				vibes_short_pulse();
				break;
		}
	}
	
	// Long vibe on the 2 and 8 minute marks
	if (t.seconds == 0) {
		switch (t.minutes) {
			case 2 :
			case 8 :
				vibes_long_pulse();
				break;
		}
	}
}

bool is_paused() {
	return isPaused;
}

void set_paused(bool pause) {
	
	if (pause) {
		// Stop ticking
		tick_timer_service_unsubscribe();
	}
	else {
		// Start ticking
		tick_timer_service_subscribe(SECOND_UNIT , tick_handler);	
	}
	
	isPaused = pause;
	
	// TODO: Toggle paused/play icon
}

void update_timer_display(struct time t) {
	
	static char s_timer_buffer[32];
	
	// Update the text layer
	snprintf(s_timer_buffer, sizeof(s_timer_buffer), "%d:%02d", t.minutes, t.seconds);
	
	text_layer_set_text(s_timer_layer, s_timer_buffer);
}

void update_progress_bars_display(Layer *layer, GContext *ctx) {
	
	const int two_minutes = 120;
	const int eight_minutes = 480;
	const int full_bar = 120;
	const int bar_height = 25;
	
	int talk_progress = s_seconds_ellapsed;
	int comments_progress = 0;
	
	if (s_seconds_ellapsed > two_minutes) {
		talk_progress = full_bar;
		comments_progress = (s_seconds_ellapsed - two_minutes) / 3;
		
		if (s_seconds_ellapsed > eight_minutes) {
			comments_progress = full_bar;
		}
	}
		
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(12, 69, talk_progress, bar_height), 0, GCornerNone);
	graphics_fill_rect(ctx, GRect(12, 117, comments_progress, bar_height), 0, GCornerNone);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	
	struct time t;
	
	t.seconds = s_seconds_ellapsed % 60;
	t.minutes = (s_seconds_ellapsed % 3600) / 60;
	
	if (t.minutes >= MAX_RUN_MINUTES) {
		// TODO: Make this optional and configurable in settings.
		set_paused(true);
		
		// TODO: Show summary screen
		//  Total comments; Speaker time, etc.
	}
	
	update_timer_display(t);	
	
	handle_scheduled_vibrations(t);
	
	s_seconds_ellapsed++;
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {  
  // Toggle paused state
	if (is_paused()) {
		set_paused(false);
	}
	else {
		set_paused(true);
	}	
}

void add_comment() {
	if (comments_total == 0) {
		// First comment!
		
	}
	
	if (comments_total < MAX_COMMENTS) {
		comments[comments_total] = s_seconds_ellapsed;
	}
	
	comments_total++;
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	add_comment();
}

void click_config_provider(Window *window) {
 // single select click:
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
	
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  
  // long click config:
  //window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, select_long_click_release_handler);
}

void handle_init(void) {
  s_main_window = window_create();

	// Get size of main window layer
	Layer *window_layer = window_get_root_layer(s_main_window);
	GRect window_bounds = layer_get_bounds(window_layer);
	
	// Set background
	s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_highlights_background);
	s_background_layer = bitmap_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
	layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
	
	// Configure timer text layer
	s_timer_layer = text_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	text_layer_set_background_color(s_timer_layer, GColorClear);
	text_layer_set_text_alignment(s_timer_layer, GTextAlignmentCenter);
	text_layer_set_text(s_timer_layer, "0:00");	
	layer_add_child(window_layer, text_layer_get_layer(s_timer_layer));
	
	// Configure canvas layer
	s_canvas_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	text_layer_set_background_color(s_timer_layer, GColorClear);
	layer_add_child(window_layer, s_canvas_layer);
	layer_set_update_proc(s_canvas_layer, update_progress_bars_display);
	
	// Configure fonts
	s_timer_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));
	text_layer_set_font(s_timer_layer, s_timer_font);
	
	// Configure buttons
	window_set_click_config_provider(s_main_window, (ClickConfigProvider) click_config_provider);

	// Init comments array
	for ( int i = 0; i < MAX_COMMENTS; i++ ) {
      comments[ i ] = 0;
  }
	
  window_stack_push(s_main_window, true);
	
	// Start out paused
	set_paused(true);
	
	vibes_short_pulse();
}

void handle_deinit(void) {
  text_layer_destroy(s_timer_layer);
	
	fonts_unload_custom_font(s_timer_font);
	
	layer_destroy(s_canvas_layer);
	
	gbitmap_destroy(s_background_bitmap);
	bitmap_layer_destroy(s_background_layer);
	
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
