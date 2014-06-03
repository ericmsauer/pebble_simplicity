#include "pebble.h"

static Window *window;

/*
 *DISPLAY ATTRIBUTES
 */
//DISPLAY VARIABLES
static bool trailing_zeros = false;

//Background
static GBitmap *background_image;
static BitmapLayer *background_layer;

//Blutetooth Status
static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

//Battery Status
#define TOTAL_BATTERY_DIGITS 4
static GBitmap *battery_image;
static BitmapLayer *battery_layer;
static GBitmap *battery_digits_images[TOTAL_BATTERY_DIGITS];
static BitmapLayer *battery_digits_layers[TOTAL_BATTERY_DIGITS];

//Display AM/PM
static GBitmap *time_format_image;
static BitmapLayer *time_format_layer;

//Date
#define TOTAL_DATE_DIGITS 6
static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

//Time
#define TOTAL_TIME_DIGITS 6
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

//Image Hash Table
const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
	RESOURCE_ID_IMAGE_NUM_0,
	RESOURCE_ID_IMAGE_NUM_1,
	RESOURCE_ID_IMAGE_NUM_2,
	RESOURCE_ID_IMAGE_NUM_3,
	RESOURCE_ID_IMAGE_NUM_4,
	RESOURCE_ID_IMAGE_NUM_5,
	RESOURCE_ID_IMAGE_NUM_6,
	RESOURCE_ID_IMAGE_NUM_7,
	RESOURCE_ID_IMAGE_NUM_8,
	RESOURCE_ID_IMAGE_NUM_9
};

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
	RESOURCE_ID_IMAGE_DAY_NAME_SUN,
	RESOURCE_ID_IMAGE_DAY_NAME_MON,
	RESOURCE_ID_IMAGE_DAY_NAME_TUE,
	RESOURCE_ID_IMAGE_DAY_NAME_WED,
	RESOURCE_ID_IMAGE_DAY_NAME_THU,
	RESOURCE_ID_IMAGE_DAY_NAME_FRI,
	RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

const int DATENUM_IMAGE_RESOURCE_IDS[] = {
	RESOURCE_ID_IMAGE_DATENUM_0,
	RESOURCE_ID_IMAGE_DATENUM_1,
	RESOURCE_ID_IMAGE_DATENUM_2,
	RESOURCE_ID_IMAGE_DATENUM_3,
	RESOURCE_ID_IMAGE_DATENUM_4,
	RESOURCE_ID_IMAGE_DATENUM_5,
	RESOURCE_ID_IMAGE_DATENUM_6,
	RESOURCE_ID_IMAGE_DATENUM_7,
	RESOURCE_ID_IMAGE_DATENUM_8,
	RESOURCE_ID_IMAGE_DATENUM_9
};

/*
 *METHODS
 */
//Used to set maintain which image is showing. Switch old images with new ones
static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin){
	//Temp variable for old image
	GBitmap *old_image = *bmp_image;

	//Replace old image with new image
	*bmp_image = gbitmap_create_with_resource(resource_id);
	GRect frame = (GRect) {
		.origin = origin,
		.size = (*bmp_image)->bounds.size
	};
	bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
	layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

	//Get rid of the old image if it exits
	if (old_image != NULL)
		gbitmap_destroy(old_image);
}

//Return the hour needed to display. Helps with 12 or 24 hour display
static unsigned short get_display_hour(unsigned short hour) {
	if (clock_is_24h_style()) {
		return hour;
	}
	unsigned short display_hour = hour % 12;

	// Converts "0" to "12"
	return display_hour ? display_hour : 12;
}

//Update the elements of the display (Change the images)
static void update_display(struct tm *current_time) {
	//Change the date name
	set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[current_time->tm_wday], GPoint(5, 140));

	//Change the date number
	set_container_image(&date_digits_images[0], date_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mon/10], GPoint(50, 140));
	set_container_image(&date_digits_images[1], date_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mon%10], GPoint(63, 140));
	set_container_image(&date_digits_images[2], date_digits_layers[2], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday/10], GPoint(81, 140));
	set_container_image(&date_digits_images[3], date_digits_layers[3], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday%10], GPoint(94, 140));
	set_container_image(&date_digits_images[4], date_digits_layers[4], DATENUM_IMAGE_RESOURCE_IDS[1], GPoint(112, 140));
	set_container_image(&date_digits_images[5], date_digits_layers[5], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_year%10], GPoint(125, 140));

	unsigned short display_hour = get_display_hour(current_time->tm_hour);

	//Display Hour
	set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(10, 45));
	set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(40, 45));

	//Display Minutes
	set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min/10], GPoint(77, 45));
	set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min%10], GPoint(105, 45));
	
	//Display AM/PM and handle extra 0
	if (!clock_is_24h_style()) {
		if (current_time->tm_hour >= 12) {
			set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_PM_MODE, GPoint(120, 30));
		}
		else{
			set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_AM_MODE, GPoint(120, 30));
		}
		if(display_hour/10 == 0 && !trailing_zeros){
			layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
		}
		else{
			layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
		}
		if(current_time->tm_mon/10 == 0 && !trailing_zeros)
			layer_set_hidden(bitmap_layer_get_layer(date_digits_layers[0]), true);
		else
			layer_set_hidden(bitmap_layer_get_layer(date_digits_layers[0]), false);
	}
}

static void handle_battery(BatteryChargeState charge_state) {
	if(charge_state.is_charging){
		layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[0]), true);
		layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[1]), true);
		layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[2]), true);
		layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[3]), true);
		layer_set_hidden(bitmap_layer_get_layer(battery_layer), false);
	}
	else{
		layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[1]), false);
		layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[2]), false);
		layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[3]), false);
		layer_set_hidden(bitmap_layer_get_layer(battery_layer), true);
		//Display Battery Level
		if(charge_state.charge_percent == 100){
			layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[0]), false);
			set_container_image(&battery_digits_images[0], battery_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[1], GPoint(2, 2));
			set_container_image(&battery_digits_images[1], battery_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[0], GPoint(15, 2));
			set_container_image(&battery_digits_images[2], battery_digits_layers[2], DATENUM_IMAGE_RESOURCE_IDS[0], GPoint(28, 2));	
			set_container_image(&battery_digits_images[3], battery_digits_layers[3], RESOURCE_ID_IMAGE_PERCENT, GPoint(41, 2));
		}
		else
		{
			layer_set_hidden(bitmap_layer_get_layer(battery_digits_layers[0]), true);
			set_container_image(&battery_digits_images[1], battery_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[charge_state.charge_percent/10], GPoint(2, 2));
			set_container_image(&battery_digits_images[2], battery_digits_layers[2], DATENUM_IMAGE_RESOURCE_IDS[charge_state.charge_percent%10], GPoint(15, 2));	
			set_container_image(&battery_digits_images[3], battery_digits_layers[3], RESOURCE_ID_IMAGE_PERCENT, GPoint(28, 2));
		}
	}
}

static void handle_second_tick(struct tm* current_time, TimeUnits units_changed) {
	//Display Seconds
	set_container_image(&time_digits_images[4], time_digits_layers[4], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_sec/10], GPoint(107, 90));
	set_container_image(&time_digits_images[5], time_digits_layers[5], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_sec%10], GPoint(120, 90));

	//Update status of the battery
	handle_battery(battery_state_service_peek());

	//If a minute has passed update rest of items
	if(current_time->tm_sec == 0)
		update_display(current_time);
}

static void handle_bluetooth(bool connected) {
	if(connected)
		set_container_image(&bluetooth_image, bluetooth_layer, RESOURCE_ID_IMAGE_BLUETOOTH_CONNECTED, GPoint(125, 2));
	else	
		set_container_image(&bluetooth_image, bluetooth_layer, RESOURCE_ID_IMAGE_BLUETOOTH_DISCONNECTED, GPoint(125, 2));
}

static void init(void) {
	//Create space for the images
	memset(&time_digits_layers, 0, sizeof(time_digits_layers));
	memset(&time_digits_images, 0, sizeof(time_digits_images));
	memset(&date_digits_layers, 0, sizeof(date_digits_layers));
	memset(&date_digits_images, 0, sizeof(date_digits_images));

	//Create main window
	window = window_create();
	if (window == NULL) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
		return;
	}
	window_stack_push(window, true /* Animated */);
	Layer *window_layer = window_get_root_layer(window);

	//Set up background image
	background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
	background_layer = bitmap_layer_create(layer_get_frame(window_layer));
	bitmap_layer_set_bitmap(background_layer, background_image);
	layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));

	//Set up bluetooth status
	bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_CONNECTED);
	GRect frame1 = (GRect) {
		.origin = { .x = 125, .y = 2 },
		.size = bluetooth_image->bounds.size
	};
	bluetooth_layer = bitmap_layer_create(frame1);
	bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
	layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));

	//Set up battery status
	battery_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_RECHARGING);
	GRect dummy_frame = { {0, 0}, {0, 0} };
	GRect frame2 = (GRect) {
		.origin = { .x = 2, .y = 2 },
		.size = battery_image->bounds.size
	};
	battery_layer = bitmap_layer_create(frame2);
	bitmap_layer_set_bitmap(battery_layer, battery_image);
	layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));	
	for (int i = 0; i < TOTAL_BATTERY_DIGITS; ++i) {
		battery_digits_layers[i] = bitmap_layer_create(dummy_frame);
		layer_add_child(window_layer, bitmap_layer_get_layer(battery_digits_layers[i]));
	}

	//Set up Clock
	if (!clock_is_24h_style()) {
		time_format_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_24_HOUR_MODE);
		GRect frame = (GRect) {
			.origin = { .x = 0, .y = 0 },
			.size = time_format_image->bounds.size
		};
		time_format_layer = bitmap_layer_create(frame);
		bitmap_layer_set_bitmap(time_format_layer, time_format_image);
		layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));
	}

	day_name_layer = bitmap_layer_create(dummy_frame);
	layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));
	for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
		time_digits_layers[i] = bitmap_layer_create(dummy_frame);
		layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
	}
	for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
		date_digits_layers[i] = bitmap_layer_create(dummy_frame);
		layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
	}

	// Avoids a blank screen on watch start.
	time_t now = time(NULL);
	struct tm *current_time = localtime(&now);
	handle_second_tick(current_time, SECOND_UNIT);

	update_display(current_time);
	
	tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
	battery_state_service_subscribe(&handle_battery);
	bluetooth_connection_service_subscribe(&handle_bluetooth);
}

static void deinit(void) {
	layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
	bitmap_layer_destroy(background_layer);
	gbitmap_destroy(background_image);

	layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
	bitmap_layer_destroy(bluetooth_layer);
	gbitmap_destroy(bluetooth_image);

	layer_remove_from_parent(bitmap_layer_get_layer(battery_layer));
	bitmap_layer_destroy(battery_layer);
	gbitmap_destroy(battery_image);
	
	layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
	bitmap_layer_destroy(time_format_layer);
	gbitmap_destroy(time_format_image);

	layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
	bitmap_layer_destroy(day_name_layer);
	gbitmap_destroy(day_name_image);

	for (int i = 0; i < TOTAL_BATTERY_DIGITS; i++) {
		layer_remove_from_parent(bitmap_layer_get_layer(battery_digits_layers[i]));
		gbitmap_destroy(battery_digits_images[i]);
		bitmap_layer_destroy(battery_digits_layers[i]);
	}

	for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
		layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
		gbitmap_destroy(date_digits_images[i]);
		bitmap_layer_destroy(date_digits_layers[i]);
	}

	for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
		layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
		gbitmap_destroy(time_digits_images[i]);
		bitmap_layer_destroy(time_digits_layers[i]);
	}
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
