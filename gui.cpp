#include "gui.h"
#include <fstream>

/**
 * Determine whether it is a leap year based on the month and year, and the number of days in each month.
 */
int process_date(int month, int year)
{
    if (month == 2)
    {
        // Determine whether it is a leap year.
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        {
            return 29;
        }
        return 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11)
    {
        return 30;
    }
    return 31;
}

/**
 * Draw a uniform format for the module background frame and title.
 */
void draw_module_background(double x, double y, double width, double height, const string &title)
{
    // Draw the background and border of the module.
    fill_rectangle(COLOR_WHITE, x, y, width, height);
    draw_rectangle(COLOR_BLACK, x, y, width, height);

    // Write the title of each module.
    draw_text(title, COLOR_BLACK, "my_font", 18, x + 10, y + 10);
}

/**
 * Check whether all six text boxes in Module 2 are filled with valid numbers.
 */
bool check_timeboxs(const GUI_State &state)
{
    if (is_integer(state.input_year) && is_integer(state.input_month) && is_integer(state.input_day) &&
        is_integer(state.input_hour) && is_integer(state.input_minute) && is_integer(state.input_second))
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Determine if it is one of the five celestial objects that are always displayed.
bool is_permanent(celestial_object *object)
{
    if (object->name == "Sun" || object->name == "Moon" || object->name == "Jupiter" || object->name == "Saturn" || object->name == "Mars")
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Determine whether it is the target celestial object selected by the user.
bool is_target(celestial_object *object, celestial_object *target)
{
    if (object == target)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void start_gui(Star_map_data &star_map, celestial_object *target)
{
    GUI_State state;
    open_window("Star Map", 1280, 720);
    load_font("my_font", "LiberationSans-Regular.ttf");

    const bool bgm_available = std::ifstream("resources/sounds/background.mp3").good();
    if (bgm_available)
    {
        load_music("bgm", "background.mp3");
        play_music("bgm", -1);
    }

    while (!quit_requested())
    {
        process_events();

        if (key_down(ESCAPE_KEY))
        {
            break;
        }

        color sky_color = rgb_color(13, 16, 23);
        clear_screen(sky_color);

        //////////////////////////////////////////////////////////
        // Draw the large frame of the right-side control panel.
        //////////////////////////////////////////////////////////
        fill_rectangle(COLOR_DARK_GRAY, state.panel_x, 0, 320, 720);
        draw_text("Star Map", COLOR_BLACK, "my_font", 24, state.panel_x + 100, 15);

        ///////////////////////////////////
        // Module 1: Observation Location
        ///////////////////////////////////
        int module1_y = 50;
        draw_module_background(state.panel_x + 10, module1_y, 300, 130, "Observation Location");

        draw_text("City Name:", COLOR_BLACK, "my_font", 14, state.panel_x + 20, module1_y + 40);

        // Generate a text box to read the city name entered by the user.
        state.input_city = text_box(state.input_city, rectangle_from(state.panel_x + 100, module1_y + 30, 190, 28));

        // Based on the city name entered by the user, search for it in the city array.
        city_data found_city;

        for (int i = 0; i < star_map.cities.length(); i++)
        {
            if (star_map.cities[i].city_name == state.input_city)
            {
                found_city = star_map.cities[i];
                star_map.observer.current_city = found_city;
                star_map.observer.timezone = found_city.timezone;
                break;
            }
        }

        // If the entered city is found, print its latitude, longitude, and name.
        if (found_city.city_name != "")
        {
            draw_text("Latitude: " + to_string(found_city.latitude), COLOR_BLACK, "my_font", 14, state.panel_x + 20, module1_y + 75);
            draw_text("Longitude: " + to_string(found_city.longitude), COLOR_BLACK, "my_font", 14, state.panel_x + 20, module1_y + 100);
            draw_text(found_city.city_name + ", " + found_city.country, COLOR_BLACK, "my_font", 12, state.panel_x + 180, module1_y + 110);
        }
        else
        {
            draw_text("City not found.", COLOR_RED, "my_font", 14, state.panel_x + 20, module1_y + 75);
        }

        ////////////////////////////////
        // Module 2: Observation Time
        ///////////////////////////////
        int module2_y = module1_y + 130 + 15; // Placed slightly below module 1
        draw_module_background(state.panel_x + 10, module2_y, 300, 165, "Observation Time");

        draw_text("Date (YYYY/MM/DD):", COLOR_BLACK, "my_font", 14, state.panel_x + 20, module2_y + 40);
        state.input_year = text_box(state.input_year, rectangle_from(state.panel_x + 160, module2_y + 30, 45, 28));
        state.input_month = text_box(state.input_month, rectangle_from(state.panel_x + 210, module2_y + 30, 30, 28));
        state.input_day = text_box(state.input_day, rectangle_from(state.panel_x + 245, module2_y + 30, 30, 28));

        draw_text("Time (HH:MM:SS):", COLOR_BLACK, "my_font", 14, state.panel_x + 20, module2_y + 80);
        draw_text(":", COLOR_BLACK, "my_font", 28, state.panel_x + 170, module2_y + 71);
        draw_text(":", COLOR_BLACK, "my_font", 28, state.panel_x + 205, module2_y + 71);
        state.input_hour = text_box(state.input_hour, rectangle_from(state.panel_x + 140, module2_y + 70, 23, 28));
        state.input_minute = text_box(state.input_minute, rectangle_from(state.panel_x + 175, module2_y + 70, 23, 28));
        state.input_second = text_box(state.input_second, rectangle_from(state.panel_x + 210, module2_y + 70, 23, 28));

        // Draw timezone
        string utc_text = "";
        if (found_city.city_name != "")
        {
            int timezone = star_map.observer.timezone;
            if (timezone >= 0)
            {
                utc_text = "(UTC+" + to_string(timezone) + ")";
            }
            else
            {
                utc_text = "(UTC" + to_string(timezone) + ")";
            }
        }
        draw_text(utc_text, COLOR_BLACK, "my_font", 14, state.panel_x + 245, module2_y + 80);

        // Check that if all time input boxes are filled with valid numbers.
        bool time_valid = check_timeboxs(state);

        if (time_valid)
        {
            star_map.observer.year = to_integer(state.input_year);
            star_map.observer.month = to_integer(state.input_month);
            star_map.observer.day = to_integer(state.input_day);
            star_map.observer.hour = to_integer(state.input_hour);
            star_map.observer.minute = to_integer(state.input_minute);
            star_map.observer.second = to_integer(state.input_second);
        }

        // Time control buttons
        if (time_valid && button("-1 Hour", rectangle_from(state.panel_x + 20, module2_y + 115, 80, 28)))
        {
            star_map.observer.hour -= 1;
            if (star_map.observer.hour < 0)
            {
                star_map.observer.hour = 23;
                star_map.observer.day -= 1;
                if (star_map.observer.day < 1)
                {
                    star_map.observer.month -= 1;
                    if (star_map.observer.month < 1)
                    {
                        star_map.observer.month = 12;
                        star_map.observer.year -= 1;
                        state.input_year = to_string(star_map.observer.year);
                    }
                    star_map.observer.day = process_date(star_map.observer.month, star_map.observer.year);
                    state.input_month = to_string(star_map.observer.month);
                }
                state.input_day = to_string(star_map.observer.day);
            }

            // Update the time previously entered by the user
            state.input_hour = to_string(star_map.observer.hour);
        }

        if (time_valid && button("+1 Hour", rectangle_from(state.panel_x + 220, module2_y + 115, 80, 28)))
        {
            star_map.observer.hour += 1;
            if (star_map.observer.hour > 23)
            {
                star_map.observer.hour = 0;
                star_map.observer.day += 1;

                // See how many days are there in this month.
                int max_days = process_date(star_map.observer.month, star_map.observer.year);
                if (star_map.observer.day > max_days)
                {
                    star_map.observer.day = 1;
                    star_map.observer.month += 1;
                    if (star_map.observer.month > 12)
                    {
                        star_map.observer.month = 1;
                        star_map.observer.year += 1;
                        state.input_year = to_string(star_map.observer.year);
                    }
                    state.input_month = to_string(star_map.observer.month);
                }
                state.input_day = to_string(star_map.observer.day);
            }

            // Update the time previously entered by the user
            state.input_hour = to_string(star_map.observer.hour);
        }

        ////////////////////////////////
        // Module 3: Selected Target
        ///////////////////////////////
        int module3_y = module2_y + 165 + 15;
        draw_module_background(state.panel_x + 10, module3_y, 300, 150, "Selected Target");

        draw_text("Target Name:", COLOR_BLACK, "my_font", 14, state.panel_x + 20, module3_y + 43);
        state.input_target = text_box(state.input_target, rectangle_from(state.panel_x + 110, module3_y + 32, 100, 28));

        bool target_found = false;
        for (int i = 0; i < star_map.celestial_objects.length(); i++)
        {
            if (star_map.celestial_objects[i]->name == state.input_target)
            {
                target = star_map.celestial_objects[i];
                target_found = true;
                break;
            }
        }

        if (target_found && found_city.city_name != "" && time_valid)
        {
            star_map.calculate_celestical_object(*target);
            star_map.checkvisibility(*target);

            draw_text("Target Altitude: " + to_string(target->altitude) + "°", COLOR_BLACK, "my_font", 14, state.panel_x + 20, module3_y + 75);
            draw_text("Target Azimuth: " + to_string(target->azimuth) + "°", COLOR_BLACK, "my_font", 14, state.panel_x + 20, module3_y + 100);

            string vis = "";
            switch (target->visibility)
            {
            case EXCELLENT:
                vis = "Excellent";
                break;
            case GOOD:
                vis = "Good";
                break;
            case POOR:
                vis = "Poor";
                break;
            case BELOW_HORIZON:
                vis = "Below Horizon";
                break;
            }
            draw_text("Visibility: " + vis, COLOR_BLACK, "my_font", 14, state.panel_x + 20, module3_y + 125);
        }
        else if (target_found && found_city.city_name == "")
        {
            draw_text("Please select a city first.", COLOR_RED, "my_font", 14, state.panel_x + 20, module3_y + 75);
        }
        else if (target_found && !time_valid)
        {
            draw_text("Please enter a valid time first.", COLOR_RED, "my_font", 14, state.panel_x + 20, module3_y + 75);
        }
        else if (!target_found)
        {
            draw_text("Target not found.", COLOR_RED, "my_font", 14, state.panel_x + 20, module3_y + 75);
        }

        ////////////////////////////////
        // Module 4: Meridian Flip
        ///////////////////////////////
        int module4_y = module3_y + 150 + 15;
        draw_module_background(state.panel_x + 10, module4_y, 300, 120, "Meridian Flip");

        if (target_found && found_city.city_name != "" && time_valid)
        {
            star_map.determine_meridian_warning(*target);
            if (target->meridian_warning)
            {
                int total_minutes = target->hours_to_meridian * 60.0;
                int hours = total_minutes / 60;
                int minutes = total_minutes % 60;

                draw_text("Status: Warning!", COLOR_RED, "my_font", 18, state.panel_x + 20, module4_y + 45);
                draw_text("Target crossing in " + to_string(hours) + "h " + to_string(minutes) + "m.", COLOR_RED, "my_font", 14, state.panel_x + 20, module4_y + 75);
                draw_text("Please recalibrate your telescope.", COLOR_RED, "my_font", 14, state.panel_x + 20, module4_y + 95);
            }
            else
            {
                draw_text("Status: Safe", COLOR_GREEN, "my_font", 18, state.panel_x + 20, module4_y + 45);
                draw_text("(No immediate flip needed)", COLOR_GREEN, "my_font", 14, state.panel_x + 20, module4_y + 70);
            }
        }
        else if (target_found && found_city.city_name == "")
        {
            draw_text("Please select a city first.", COLOR_RED, "my_font", 14, state.panel_x + 20, module4_y + 45);
        }
        else if (target_found && !time_valid)
        {
            draw_text("Please enter a valid time first.", COLOR_RED, "my_font", 14, state.panel_x + 20, module4_y + 45);
        }
        else if (!target_found && found_city.city_name != "")
        {
            draw_text("Please select a target first.", COLOR_RED, "my_font", 14, state.panel_x + 20, module4_y + 45);
        }
        else
        {
            draw_text("Please select a city and target first.", COLOR_RED, "my_font", 14, state.panel_x + 20, module4_y + 45);
        }


        //////////////////////////////////////////////
        /// A button to display all celestial bodies.
        //////////////////////////////////////////////
        if (state.show_all == true)
        {
            state.button_text = "Hide All Stars";
        }
        else
        {
            state.button_text = "Show All Stars";
        }

        if (button(state.button_text, rectangle_from(state.panel_x + 170, module4_y + 125, 120, 30)))
        {
            if (state.show_all == true)
            {
                state.show_all = false;
            }
            else
            {
                state.show_all = true;
            }
        }

        //////////////////////////////////////////////
        /// A button to pause or play background music
        //////////////////////////////////////////////
        if (!bgm_available)
        {
            state.bgm_text = "BGM unavailable";
        }
        else if (music_paused())
        {
            state.bgm_text = "Play BGM";
        }
        else
        {
            state.bgm_text = "Pause BGM";
        }

        if (button(state.bgm_text, rectangle_from(state.panel_x + 20, module4_y + 125, 120, 30)) && bgm_available)
        {
            if (music_paused())
            {
                resume_music();
            }
            else
            {
                pause_music();
            }
        }

        ////////////////////////////////////////////////////
        /// A slider used to zoom in or out of the star map.
        ////////////////////////////////////////////////////
        state.sliderss = slider(state.sliderss, 0.5, 1.5, rectangle_from(state.panel_x - 240, module4_y + 120, 220, 30));

        //////////////////////////////////
        // Draw the star map on the left
        //////////////////////////////////

        // The center point of the star chart, which is when the altitude angle is 90°(zenith).
        int center_x = 480;
        int center_y = 360;

        // The radius of the outermost great circle, scaled by the slider value.
        double max_radius = 320.0 * state.sliderss;

        // Draw three concentric circles with different Altitudes.
        draw_circle(COLOR_DIM_GRAY, center_x, center_y, max_radius); // Altitude 0° (Horizon)
        draw_circle(COLOR_DIM_GRAY, center_x, center_y, max_radius * 60.0 / 90.0); // Altitude 30°
        draw_circle(COLOR_DIM_GRAY, center_x, center_y, max_radius * 30.0 / 90.0); // Altitude 60°

        // Draw two dotted lines: east-west and north-south.
        int line_length = 8; // The length of each solid line segment.
        int line_gap = 8;    // The spacing between each pair of lines.

        double current_r = -max_radius;
        while (current_r < max_radius)
        {
            // Calculate where this short solid line should end.
            double end_r = current_r + line_length;

            // Check if the endpoint exceeds the star map.
            if (end_r > max_radius)
            {
                end_r = max_radius;
            }

            // Start drawing each small solid line segment little by little.
            draw_line(COLOR_DIM_GRAY, center_x + current_r, center_y, center_x + end_r, center_y);
            draw_line(COLOR_DIM_GRAY, center_x, center_y + current_r, center_x, center_y + end_r);

            // Set a new starting point.
            current_r = end_r + line_gap;
        }

        // Mark the four directions: east, south, west, and north.
        draw_text("N", COLOR_LIGHT_GRAY, "my_font", 18, center_x - 6, center_y - max_radius - 25);
        draw_text("S", COLOR_LIGHT_GRAY, "my_font", 18, center_x - 6, center_y + max_radius + 10);
        draw_text("W", COLOR_LIGHT_GRAY, "my_font", 18, center_x - max_radius - 30, center_y - 8);
        draw_text("E", COLOR_LIGHT_GRAY, "my_font", 18, center_x + max_radius + 15, center_y - 8);

        // Draw a cross to represent the zenith.
        draw_line(COLOR_WHITE, center_x - 10, center_y, center_x + 10, center_y);
        draw_line(COLOR_WHITE, center_x, center_y - 10, center_x, center_y + 10);

        // Label the altitude(90°, 60°, 30°, 0°)
        draw_text("90°", COLOR_LIGHT_GRAY, "my_font", 14, center_x + 12, center_y - 16);
        draw_text("60°", COLOR_LIGHT_GRAY, "my_font", 14, center_x + (max_radius * 30.0 / 90.0) + 5, center_y - 16);
        draw_text("30°", COLOR_LIGHT_GRAY, "my_font", 14, center_x + (max_radius * 60.0 / 90.0) + 5, center_y - 16);
        draw_text("0°", COLOR_LIGHT_GRAY, "my_font", 14, center_x + max_radius + 5, center_y - 16);

        //////////////////////////////////
        // Draw Celestial Objects
        //////////////////////////////////
        if (found_city.city_name != "" && time_valid)
        {
            for (int i = 0; i < star_map.celestial_objects.length(); i++)
            {
                celestial_object *object = star_map.celestial_objects[i];

                if (state.show_all == true || is_permanent(object) || is_target(object, target))
                {
                    star_map.calculate_celestical_object(*object);

                    // Obtain the x and y coordinates of the celestial object.
                    double object_x, object_y;
                    star_map.star_map_coordinates(object->altitude, object->azimuth, center_x, center_y, max_radius, object_x, object_y);

                    // Read the color of each celestial object.
                    color draw_color = object->exclusive_color;

                    // Determine if it is below the horizon; if so, increase the transparency.
                    if (object->altitude < 0.0)
                    {
                        draw_color = rgba_color(red_of(draw_color), green_of(draw_color), blue_of(draw_color), 50);
                    }

                    // The size of each celestial objects is determined by its apparent magnitude.
                    double object_radius = 5.0 - (object->apparent_magnitude / 2.0);

                    // If user click on a star on the star map, it will be set as the target.
                    if (mouse_clicked(LEFT_BUTTON))
                    {
                        if (object_x - object_radius < mouse_x() && mouse_x() < object_x + object_radius && object_y - object_radius < mouse_y() && mouse_y() < object_y + object_radius)
                        {
                            // Set the celestial object user just pointed to as the target.
                            state.input_target = object->name;
                        }
                    }

                    // Use the polymorphism of objects to draw different celestial objects.
                    bool is_target_object = (is_target(object, target) && target_found);
                    object->draw_on_map(object_x, object_y, object_radius, draw_color, is_target_object);
                    draw_text(object->name, draw_color, "my_font", 12, object_x + object_radius + 2, object_y - object_radius - 2);
                }
            }
        }

        ///////////////////////
        // Observation Status
        ///////////////////////
        // Draw a box in the upper left corner to display the observations status
        draw_module_background(20, 20, 260, 80, "Observation Status");

        // The API will only be called to retrieve the weather if the user enters the correct city,
        // and this city is different from the city where cloud cover was previously retrieved.
        if (found_city.city_name != "" && time_valid)
        {
            if (found_city.city_name != state.last_city_call_API)
            {
                star_map.get_weather();
                state.last_city_call_API = found_city.city_name;
            }
            
            // If the city hasn't changed, update from the stored data.
            star_map.update_weather_data();
        }
        else
        {
            state.last_city_call_API = "";
        }

        if (found_city.city_name != "" && time_valid)
        {
            if (star_map.current_weather.forecast_list.empty())
            {
                draw_text("Weather unavailable.", COLOR_BLACK, "my_font", 14, 30, 55);
                draw_text("Set OPENWEATHER_API_KEY to enable.", COLOR_BLACK, "my_font", 12, 30, 75);
            }
            else
            {
                draw_text("Cloud Cover: " + to_string(star_map.current_weather.cloud_cover) + "%", COLOR_BLACK, "my_font", 14, 30, 55);
                draw_text("Condition: " + star_map.current_weather.condition, COLOR_BLACK, "my_font", 14, 30, 75);
            }
        }
        else if (found_city.city_name == "")
        {
            draw_text("Please select a city first.", COLOR_BLACK, "my_font", 14, 30, 55);
        }
        else if (!time_valid)
        {
            draw_text("Please enter a valid time.", COLOR_BLACK, "my_font", 14, 30, 55);
        }

        draw_interface();
        refresh_screen(60);
    }
}
