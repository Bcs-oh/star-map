#ifndef GUI_H
#define GUI_H

#include "splashkit.h"
#include "star_map.h"

struct GUI_State
{
    int panel_x = 960;
    string input_city = "";
    string input_year = "";
    string input_month = "";
    string input_day = "";
    string input_hour = "";
    string input_minute = "";
    string input_second = "";
    string input_target = "";
    string last_city_call_API = ""; // Record the city where cloud usage was last retrieved to prevent frequent API calls.
    bool show_all = false;
    string button_text = "";
    double sliderss = 1.0;
    string bgm_text = "";
};

/** Check observation fields against calendar and clock ranges (years 1-9999). */
bool check_timeboxs(const GUI_State &state);

/** Shift a valid observation by one hour without leaving years 1-9999. */
bool shift_observation_hour(observer_data &observer, int change);

/** Format a UTC offset without losing half-hour or quarter-hour minutes. */
string format_utc_offset(double timezone);

/** Calculate a visible marker radius, with a one-pixel minimum. */
double object_marker_radius(double apparent_magnitude);

/**
 * Starts the graphical user interface for the Star Map.
 * 
 * @param star_map The star map data containing observation details.
 * @param target The selected celestial object to display.
 */
void start_gui(Star_map_data &star_map, celestial_object *target);

#endif
