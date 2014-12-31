#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_timer_layer;
static GFont *s_timer_font;

static int s_seconds_ellapsed = 0;

void update_timer(int seconds, int minutes) {
	
	static char s_timer_buffer[32];
	
	// Update the text layer
	snprintf(s_timer_buffer, sizeof(s_timer_buffer), "%d:%02d", minutes, seconds);
	
	text_layer_set_text(s_timer_layer, s_timer_buffer);
}

void generate_temporal_feeback(int seconds, int minutes) {
	
	// Short vibe 15 seconds before 2 and 8 minute marks
	if (seconds == 45) {
		switch (minutes) {
			case 1 :
			case 7 :
				vibes_short_pulse();
				break;
		}
	}
	
	// Long vibe on the 2 and 8 minute marks
	if (seconds == 0) {
		switch (minutes) {
			case 2 :
			case 8 :
				vibes_long_pulse();
				break;
		}
	}
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	
	int seconds = s_seconds_ellapsed % 60;
	int minutes = (s_seconds_ellapsed % 3600) / 60;
	
	update_timer(seconds, minutes);	
	
	generate_temporal_feeback(seconds, minutes);
	
	s_seconds_ellapsed++;
}

void handle_init(void) {
  s_main_window = window_create();

	// Get size of window layer
	Layer *window_layer = window_get_root_layer(s_main_window);
	GRect window_bounds = layer_get_bounds(window_layer);
	
	// Configure timer text layer
	s_timer_layer = text_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
	text_layer_set_text_alignment(s_timer_layer, GTextAlignmentCenter);
	text_layer_set_text(s_timer_layer, "0:00");	
	layer_add_child(window_layer, text_layer_get_layer(s_timer_layer));
	
	// Configure fonts
	s_timer_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));
	text_layer_set_font(s_timer_layer, s_timer_font);
		
  window_stack_push(s_main_window, true);
	
	// Register with TickTimerService
	tick_timer_service_subscribe(SECOND_UNIT , tick_handler);
	
	vibes_short_pulse();
}

void handle_deinit(void) {
  text_layer_destroy(s_timer_layer);
	
	fonts_unload_custom_font(s_timer_font);
	
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
