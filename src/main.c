#include <pebble.h>

const short MAX_RUN_MINUTES = 10;
const short MAX_COMMENTS = 30;

int s_seconds_ellapsed = 0;

bool is_paused = true;

// TODO: Replace magic number with MAX_COMMENTS
short comments[ 30 ]; 
short comments_total = 0;

struct time {
	short minutes;
	short seconds;
};

/* Main Window */ 
static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_timer_layer;
static GFont *s_timer_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

/* Summary Window*/
// TODO: Organize across multiple source files and sort functions
static Window *s_summary_window;
static TextLayer *s_summary_text;


/* -- Method declarations -- */
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);

void handle_scheduled_vibrations(struct time t);

bool is_Paused();
void set_Paused(bool pause);

void update_timer_display(struct time t);

void update_progress_bars_display(Layer *layer, GContext *ctx);

void click_config_provider(Window *window);
void select_single_click_handler(ClickRecognizerRef recognizer, void *context);
void up_single_click_handler(ClickRecognizerRef recognizer, void *context);
void select_long_click_handler(ClickRecognizerRef recognizer, void *context);

void add_comment();

void display_summary();

void init_main_window();
void destroy_main_window();

void init_summary_window();
void destroy_summary_window();


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

bool is_Paused() {
	return is_paused;
}

void set_Paused(bool pause) {
	
	if (pause) {
		// Stop ticking
		tick_timer_service_unsubscribe();
	}
	else {
		// Start ticking
		tick_timer_service_subscribe(SECOND_UNIT , tick_handler);
		vibes_short_pulse();
	}
	
	is_paused = pause;
	
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
		set_Paused(true);
		
		// TODO: Show summary screen
		//  Total comments; Speaker time, etc.
	}
	
	update_timer_display(t);	
	
	handle_scheduled_vibrations(t);
	
	s_seconds_ellapsed++;
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {  
  // Toggle paused state
	if (is_Paused()) {
		set_Paused(false);
	}
	else {
		set_Paused(true);
	}	
}

void add_comment() {
	if (comments_total < MAX_COMMENTS) {
		comments[comments_total] = s_seconds_ellapsed;
		comments_total++;
	}
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	add_comment();
}

void display_summary() {
	set_Paused(true);
	
	/* -- Calculate lengths and averages -- */
	static char message_buffer[256] = "";
	
	// Avg seconds of comment overhead: mic handling time + hand choosing time :)
	// TODO: Make this user-configurable
	const int comment_overhead = 5;
	
	int talk_length = s_seconds_ellapsed;
	
	int first_comment_start = 0;
	int comment_length_avg = 0;
	int comment_length_midmean = 0;
	int longest = 0;
	int shortest = 0;
	
	if (comments_total > 0) {
		first_comment_start = comments[0];

		talk_length = first_comment_start - comment_overhead;
	
		comment_length_avg = (  s_seconds_ellapsed
													- first_comment_start
													- (comment_overhead * comments_total))
												/ comments_total;
		
		longest = s_seconds_ellapsed - comments[comments_total - 1];
		shortest = longest;
		
		// Get longest and shortest comments
		for ( int i = comments_total - 1, i_len = 0; i > 0; i-- ) {
      i_len = comments[i] - comments[i - 1];
			
			if (i_len > longest) {
				longest = i_len;
			}
			
			if (i_len < shortest) {
				shortest = i_len;
			}	
  	}

		comment_length_midmean = comment_length_avg;
			
		if (comments_total > 2) {
			comment_length_midmean = (  s_seconds_ellapsed
																- first_comment_start
																- longest
																- shortest
																- (comment_overhead * 2))
															/ (comments_total - 2);
		}
	}
	
	// TODO: Format times as MM:SS
	snprintf(message_buffer, sizeof(message_buffer), 
					 "*** Summary ***\nTalk: %i\nComments: %i\nLongest: %i\nShortest: %i\nAverage: %i\nMidmean: %i\nTotal: %i", 
					 talk_length,
					 comments_total,
					 longest,
					 shortest,
					 comment_length_avg,
					 comment_length_midmean,
					 s_seconds_ellapsed);
	
	text_layer_set_text(s_summary_text, message_buffer);
	
	window_stack_push(s_summary_window, true);
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	display_summary();	
}

void click_config_provider(Window *window) {
 // Single select click:
  window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);	
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  
  // Long click config:
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
}

void init_main_window() {
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

	window_stack_push(s_main_window, true);
}

void init_summary_window() {
	s_summary_window = window_create();
	
	Layer *window_layer = window_get_root_layer(s_summary_window);
	
	GRect window_bounds = layer_get_bounds(window_layer);
	
	s_summary_text = text_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	text_layer_set_text(s_summary_text, "Highlights Summary");
	
	layer_add_child(window_layer, text_layer_get_layer(s_summary_text));	
}

void handle_init(void) {
  
	// Init comments array
	for ( int i = 0; i < MAX_COMMENTS; i++ ) {
      comments[ i ] = 0;
  }
	
	init_main_window();
	
	init_summary_window();
	
	// Start out paused
	set_Paused(true);	
}

void destroy_main_window() {
	text_layer_destroy(s_timer_layer);
	
	fonts_unload_custom_font(s_timer_font);
	
	layer_destroy(s_canvas_layer);
	
	gbitmap_destroy(s_background_bitmap);
	bitmap_layer_destroy(s_background_layer);
	
  window_destroy(s_main_window);
}

void destroy_summary_window() {
	text_layer_destroy(s_summary_text);
	
	window_destroy(s_summary_window);
}

void handle_deinit(void) {
	destroy_summary_window();
	
  destroy_main_window();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
