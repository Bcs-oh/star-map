#ifndef STAR_MAP_H
#define STAR_MAP_H

#include "splashkit.h"
#include "splashkit-arrays.h"
#include "celestial_objects.h"
#include <cmath>
#include <fstream>
#include "utilities.h"
#include <vector>
#include <cstdlib>

/**
 * A struct that stores city data.
 */
struct city_data
{
    string city_name;
    double latitude;
    double longitude;
    string country;
    double timezone;
};

/**
 * A struct that stores observer data.
 */
struct observer_data
{
    city_data current_city;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
    double timezone;
};

/**
 * A struct that stores a single forecast entry.
 */
struct forecast_data
{
    int dt;
    int cloud_cover = 0;
};

/**
 * A struct that stores weather data.
 */
struct weather_data
{
    string API_URL;
    string API_key;
    int cloud_cover = 0;
    string condition;
    std::vector<forecast_data> forecast_list;
};

/**
 * A struct that stores all the data needed in a star map.
 * It handles the conversion from UTC to sidereal time,
 * as well as the conversion from right ascension and declination to altitude and azimuth.
 */
struct Star_map_data
{
    dynamic_array<celestial_object *> celestial_objects;
    dynamic_array<city_data> cities;
    int target_id;
    observer_data observer;
    weather_data current_weather; // Declare the weather_data instance!

    // Destructor to clean up dynamically allocated memory
    ~Star_map_data()
    {
        for (int i = 0; i < celestial_objects.length(); i++)
        {
            delete celestial_objects[i];
        }
    }

    // Read city data from the CSV file and store it in the cities array.
    void load_cities(string file_name)
    {
        using std::getline;
        using std::ifstream;

        ifstream file(file_name);
        if (!file.is_open())
        {
            write_line("Error! Could not open the file: " + file_name);
            return;
        }

        string line;
        // Skip the header (City,Latitude,Longitude,Country,TC_Offset)
        getline(file, line);

        // Read the file line by line
        while (getline(file, line))
        {
            int first_comma = line.find(',');
            int second_comma = line.find(',', first_comma + 1);
            int third_comma = line.find(',', second_comma + 1);
            int fourth_comma = line.find(',', third_comma + 1);

            // Get the city's name, latitude, longitude, country, and timezone offset.
            string name = line.substr(0, first_comma);
            string latitude_string = line.substr(first_comma + 1, second_comma - first_comma - 1);
            string longitude_string = line.substr(second_comma + 1, third_comma - second_comma - 1);
            string country_string = line.substr(third_comma + 1, fourth_comma - third_comma - 1);
            string timezone_string = line.substr(fourth_comma + 1);

            city_data new_city;
            new_city.city_name = name;
            new_city.latitude = to_double(latitude_string);
            new_city.longitude = to_double(longitude_string);
            new_city.country = country_string;
            new_city.timezone = to_double(timezone_string);
            cities.add(new_city);
        }
        file.close();
    }

    // Prompt the user to enter a city and set the observer's coordinates.
    void select_city()
    {
        if (cities.length() == 0)
        {
            write_line("Error: No cities stored in array.");
            return;
        }

        while (true)
        {
            string input_city = read_string("Enter your city name: ");

            for (int i = 0; i < cities.length(); i++)
            {
                if (cities[i].city_name == input_city)
                {
                    write_line();
                    write_line("Latitude: " + to_string(cities[i].latitude));
                    write_line("Longitude: " + to_string(cities[i].longitude));
                    write_line(cities[i].city_name + ", " + cities[i].country);

                    // Update observer's current city
                    observer.current_city = cities[i];
                    observer.timezone = cities[i].timezone;
                    return;
                }
            }
            write_line("City not found. Please check your spelling and try again.");
        }
    }

    // Read star data from the CSV file and store it in the celestial_objects array.
    bool load_stars(string file_name)
    {
        using std::getline;
        using std::ifstream;

        ifstream file(file_name);
        if (!file.is_open())
        {
            write_line("Failed to open stars file: " + file_name);
            return false;
        }

        string line;

        // Skip the header (Name,Right_ascension,Declination,Apparent_magnitude,Type)
        getline(file, line);

        while (getline(file, line))
        {
            int comma1 = line.find(',');
            int comma2 = line.find(',', comma1 + 1);
            int comma3 = line.find(',', comma2 + 1);
            int comma4 = line.find(',', comma3 + 1);

            if (comma1 != -1 && comma2 != -1 && comma3 != -1 && comma4 != -1)
            {
                string name_string = line.substr(0, comma1);
                string ra_string = line.substr(comma1 + 1, comma2 - comma1 - 1);
                string dec_string = line.substr(comma2 + 1, comma3 - comma2 - 1);
                string mag_string = line.substr(comma3 + 1, comma4 - comma3 - 1);
                string type_string = line.substr(comma4 + 1);

                // Convert a string to a number
                double ra = to_double(ra_string);
                double dec = to_double(dec_string);
                double mag = to_double(mag_string);

                // The exclusive color is assigned based on the name of the celestial body;
                // the default value is white.
                color object_color = COLOR_WHITE;

                if (name_string == "Sun")
                {
                    object_color = COLOR_RED;
                }
                else if (name_string == "Moon")
                {
                    object_color = COLOR_LIGHT_GRAY;
                }
                else if (name_string == "Mercury")
                {
                    object_color = COLOR_GRAY;
                }
                else if (name_string == "Venus")
                {
                    object_color = COLOR_LIGHT_YELLOW;
                }
                else if (name_string == "Mars")
                {
                    object_color = COLOR_ORANGE;
                }
                else if (name_string == "Jupiter")
                {
                    object_color = COLOR_BROWN;
                }
                else if (name_string == "Saturn")
                {
                    object_color = COLOR_GOLD;
                }
                else if (name_string == "Uranus")
                {
                    object_color = COLOR_LIGHT_BLUE;
                }
                else if (name_string == "Neptune")
                {
                    object_color = COLOR_BLUE;
                }

                celestial_object *new_object = nullptr;
                if (type_string == "Planet")
                {
                    new_object = new Planet(name_string, ra, dec, mag, object_color);
                }
                else if (type_string == "Star")
                {
                    new_object = new Star(name_string, ra, dec, mag, object_color);
                }
                else if (type_string == "Satellite")
                {
                    new_object = new Satellite(name_string, ra, dec, mag, object_color);
                }
                else
                {
                    new_object = new Deep_sky_object(name_string, ra, dec, mag, object_color, type_string);
                }

                celestial_objects.add(new_object);
            }
        }
        file.close();
        return true;
    }

    // Prompt the user to enter a celestial object name and return a pointer to it.
    celestial_object *select_star()
    {
        if (celestial_objects.length() == 0)
        {
            write_line("Error: There are no celestial objects loaded.");
            return nullptr;
        }

        while (true)
        {
            string input_star = read_string("Enter target name: ");

            for (int i = 0; i < celestial_objects.length(); i++)
            {
                if (celestial_objects[i]->name == input_star)
                {
                    write_line("Target selected: " + celestial_objects[i]->name);
                    return celestial_objects[i];
                }
            }

            write_line("Target not found. Please check your spelling and try again.");
        }
        return nullptr;
    }

    // Prompt the user to enter their observation time.
    void set_observation_time()
    {
        write_line();
        observer.year = read_integer("Enter Year (e.g., 2026): ");
        observer.month = read_integer("Enter Month (1-12): ", 1, 12);
        observer.day = read_integer("Enter Day (1-31): ", 1, 31);
        observer.hour = read_integer("Enter Hour (0-23): ", 0, 23);
        observer.minute = read_integer("Enter Minute (0-59): ", 0, 59);
        observer.second = read_integer("Enter Second (0-59): ", 0, 59);
        observer.timezone = observer.current_city.timezone; // Automatically set to city timezone
    }

    // Calculate the current Julian Date (JD)
    double calclute_JD()
    {
        int year = observer.year;
        int month = observer.month;
        // Step1: Convert to UTC and Decimal Days
        double DF = (observer.hour / 24.0) + (observer.minute / 1440.0) + (observer.second / 86400.0);
        double D = observer.day + DF;

        // step2: The January/February Shift
        if (month <= 2)
        {
            year -= 1;
            month += 12;
        }

        // step3: Run the Core Algorithm
        // 1.The Gregorian Correction (accounting for centurial leap years):
        int A = year / 100;
        int B = A / 4;
        int C = 2 - A + B;

        // 2.The Year and Month terms:
        int E = 365.25 * (year + 4716);
        int F = 30.6001 * (month + 1);

        // step4: The Final Sum
        double local_JD = C + D + E + F - 1524.5;
        double UTC_JD = local_JD - (observer.timezone / 24.0);
        return UTC_JD;
    }

    // Calculate the altitude and azimuth of a celestial object
    void calculate_celestical_object(celestial_object &target)
    {
        double current_latitude = observer.current_city.latitude;
        double current_longitude = observer.current_city.longitude;
        // Get LST hour.
        double LST_hour = calculate_LST(calclute_JD(), current_longitude);

        // Calculate the Local Hour Angle (LHA)
        double LHA = LST_hour - target.right_ascension;

        // Calclute altitude and azimuth
        // Global constant for Pi
        const double PI = 3.14159265358979323846;
        // Convert angles to radians
        double latitude_radians = current_latitude * (PI / 180.0);
        double declination_radians = target.declination * (PI / 180.0);
        double LHA_radians = (LHA * 15.0) * (PI / 180.0);

        // calclute altitude
        double sin_altitude = sin(declination_radians) * sin(latitude_radians) + cos(declination_radians) * cos(latitude_radians) * cos(LHA_radians);
        double altitude_radians = asin(sin_altitude);
        double altitude_degree = altitude_radians * (180.0 / PI);

        // calclute azimuth
        double cos_azimuth = (sin(declination_radians) - sin(altitude_radians) * sin(latitude_radians)) / (cos(altitude_radians) * cos(latitude_radians));
        double azimuth_radians = acos(cos_azimuth);
        double azimuth_degree = azimuth_radians * (180.0 / PI);

        // If the celestial object is at east of the meridian, the azimuth angle should be subtracted from 360.
        if (sin(LHA_radians) > 0)
        {
            azimuth_degree = 360.0 - azimuth_degree;
        }

        // save the result
        target.altitude = altitude_degree;
        target.azimuth = azimuth_degree;

        // Calculate time to meridian flip
        //  Step 1: Ensure LHA is a positive number between 0 and 24 hours.
        double positive_LHA = LHA;
        if (positive_LHA < 0.0)
        {
            positive_LHA += 24.0;
        }
        else
        {
            positive_LHA -= 24.0;
        }

        // Step 2: Determine if it's approaching or has already passed the meridian.
        // LHA means "hours since passing the meridian".
        if (positive_LHA > 12.0)
        {
            // It is in the eastern sky, approaching the meridian.
            target.hours_to_meridian = 24.0 - positive_LHA;
        }
        else
        {
            // It is in the western sky, already passed the meridian.
            target.hours_to_meridian = -positive_LHA;
        }
    }

    // Determine the visibility level based on altitude
    void checkvisibility(celestial_object &target)
    {
        if (target.altitude <= 0.0)
        {
            target.visibility = BELOW_HORIZON;
        }
        else if (target.altitude < 30.0)
        {
            target.visibility = POOR;
        }
        else if (target.altitude < 60.0)
        {
            target.visibility = GOOD;
        }
        else
        {
            target.visibility = EXCELLENT;
        }
    }

    // Determine whether a meridian warning should be issued
    void determine_meridian_warning(celestial_object &target)
    {
        // Issuing a warning if the object is above horizon and crossing meridian within 3 hours (0 to 3.0 hours)
        if (target.altitude > 0.0 && target.hours_to_meridian > 0.0 && target.hours_to_meridian <= 3.0)
        {
            target.meridian_warning = true;
        }
        else
        {
            target.meridian_warning = false;
        }
    }

    // Calculating local sidereal time(LST)
    double calculate_LST(double JD, double longitude)
    {
        // Step 1: Calculate Days Since the J2000.0 Epoch (d)
        double d = JD - 2451545;

        // Step 2: Calculate Greenwich Mean Sidereal Time (GMST)
        double GMST = 280.46061837 + 360.98564736629 * d;
        GMST = fmod(GMST, 360.0);
        if (GMST < 0)
        {
            GMST += 360;
        }

        // Step 3: Adjust for Your Local Longitude
        double LST = fmod(GMST + longitude, 360);
        if (LST < 0)
        {
            LST += 360;
        }

        // Step 4: Convert to Hours.
        double LST_hour = LST / 15.0;
        return LST_hour;
    }

    void print_LST(double LST_hour)
    {
        int LST_int_hour = LST_hour;
        double remainder_minute = (LST_hour - LST_int_hour) * 60.0;
        int LST_minute = remainder_minute;
        double LST_second = (remainder_minute - LST_minute) * 60.0;
        write("LST : ");
        write(to_string(LST_int_hour));
        write("h ");
        write(to_string(LST_minute));
        write("m ");
        write(to_string(LST_second));
        write_line("s");
    }

    // Obtain cloud cover and determine weather conditions from API
    void get_weather()
    {
        http_response get_data;
        string response;

        string lat = to_string(observer.current_city.latitude);
        string lon = to_string(observer.current_city.longitude);

        // Utilize the variables defined in the weather_data struct
        const char *api_key = std::getenv("OPENWEATHER_API_KEY");
        current_weather.forecast_list.clear();
        current_weather.cloud_cover = 0;
        current_weather.condition = "Unavailable";
        if (api_key == nullptr || api_key[0] == '\0')
        {
            return;
        }
        current_weather.API_key = api_key;
        current_weather.API_URL = "https://api.openweathermap.org/data/2.5/forecast?lat=" + lat + "&lon=" + lon + "&appid=" + current_weather.API_key;

        get_data = http_get(current_weather.API_URL, 443);
        response = http_response_to_string(get_data);
        free_response(get_data);

        // Convert a string to a JSON object
        json json_response = json_from_string(response);
        if (!json_has_key(json_response, "list"))
        {
            free_json(json_response);
            return;
        }
        std::vector<json> weather_list;
        json_read_array(json_response, "list", weather_list);

        // Clear previous forecast data
        current_weather.forecast_list.clear();

        for (int i = 0; i < weather_list.size(); i++)
        {
            json item = weather_list[i];
            int dt = json_read_number_as_int(item, "dt");
            json clouds_object = json_read_object(item, "clouds");
            int cloud_cover = json_read_number_as_int(clouds_object, "all");

            forecast_data data;
            data.dt = dt;
            data.cloud_cover = cloud_cover;
            current_weather.forecast_list.push_back(data);

            free_json(clouds_object);
            free_json(item);
        }

        // Free the JSON objects to prevent memory leaks
        free_json(json_response);
    }

    // Update the cloud volume and weather conditions
    // according to the stored forecast data and the current observation time.
    void update_weather_data()
    {
        if (current_weather.forecast_list.empty())
        {
            return;
        }

        // Convert the JD at this time to a Unix Timestamp.
        int user_unix = (calclute_JD() - 2440587.5) * 86400;

        int target_cloud_cover = 0;
        int previous_cloud_cover = 0;
        int previous_dt = 0;

        for (int i = 0; i < current_weather.forecast_list.size(); i++)
        {
            int dt = current_weather.forecast_list[i].dt;
            int cloud_cover = current_weather.forecast_list[i].cloud_cover;

            // The first time the input time is less than the received time
            if (user_unix < dt)
            {
                // This indicates that the user entered a past time, so the first data point is taken.
                if (i == 0)
                {
                    target_cloud_cover = cloud_cover;
                }
                // Compare whether it's closer to the previous time or closer to the current time
                else
                {
                    // If it's close to the previous one, then take the previous cloud data.
                    if ((user_unix - previous_dt) < (dt - user_unix))
                    {
                        target_cloud_cover = previous_cloud_cover;
                    }
                    // If it's close to the next one, then take this cloud cover data.
                    else
                    {
                        target_cloud_cover = cloud_cover;
                    }
                }
                break;
            }
            // Store the current data to the previous one and compare it for the next loop.
            previous_dt = dt;
            previous_cloud_cover = cloud_cover;

            // If none of the conditions are met when iterating to the last record
            //(meaning the user-input time exceeds the 5-day forecast),
            // then the last record will be used by default.
            if (i == current_weather.forecast_list.size() - 1)
            {
                target_cloud_cover = cloud_cover;
            }
        }
        current_weather.cloud_cover = target_cloud_cover;

        // Judging observation conditions by cloud cover
        if (current_weather.cloud_cover <= 10)
        {
            current_weather.condition = "Cloudless";
        }
        else if (current_weather.cloud_cover <= 30)
        {
            current_weather.condition = "Partly Sunny";
        }
        else if (current_weather.cloud_cover <= 60)
        {
            current_weather.condition = "Partly Cloudy";
        }
        else if (current_weather.cloud_cover <= 85)
        {
            current_weather.condition = "Mostly Cloudy";
        }
        else
        {
            current_weather.condition = "Overcast";
        }
    }

    // Convert altitude and azimuth into x and y coordinates.
    void star_map_coordinates(double altitude, double azimuth, double center_x, double center_y, double max_radius, double &x, double &y)
    {
        // Convert azimuth to radians.
        const double PI = 3.14159265358979323846;
        double azimuth_radians = azimuth * (PI / 180.0);
        
        // Calculate the mapping radius (R).
        double R = max_radius * ( 90 - altitude ) / 90;

        // Calculate the x and y coordinates and save them directly.
        x = center_x + R * sin(azimuth_radians);
        y = center_y - R * cos(azimuth_radians);
    }
};

#endif
