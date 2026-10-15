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

/**
 * Starts the graphical user interface for the Star Map.
 * 
 * @param star_map The star map data containing observation details.
 * @param target The selected celestial object to display.
 */
void start_gui(Star_map_data &star_map, celestial_object *target);

#endif