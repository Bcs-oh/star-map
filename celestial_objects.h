#ifndef CELESTIAL_OBJECTS_H
#define CELESTIAL_OBJECTS_H

#include "splashkit.h"

/**
 * List four different levels of visibility.
 */
enum Visibility
{
    EXCELLENT,
    GOOD,
    POOR,
    BELOW_HORIZON
};

/**
 * A struct that stores celestial data.
 */
class celestial_object
{
public:
    string name;
    double right_ascension;
    double declination;
    double apparent_magnitude;
    color exclusive_color;
    double altitude;
    double azimuth;
    Visibility visibility;
    bool meridian_warning; // Determine whether to issue a warning.
    double hours_to_meridian;

    // Constructor:
    celestial_object()
    {
        name = "Unknown";
        right_ascension = 0.0;
        declination = 0.0;
        apparent_magnitude = 0.0;
        exclusive_color = COLOR_WHITE;
        altitude = 0.0;
        azimuth = 0.0;
        visibility = EXCELLENT;
        meridian_warning = false;
    }

    virtual ~celestial_object() = default;

    virtual void print_info()
    {
        write_line("Target Altitude: " + to_string(altitude));
        write_line("Target Azimuth: " + to_string(azimuth));

        string vis = "Unknown";
        switch (visibility)
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
        write_line("Visibility: " + vis);

        write_line("Meridian Flip");
        if (meridian_warning == true)
        {
            int total_minutes = hours_to_meridian * 60.0;
            int hours = total_minutes / 60;
            int minutes = total_minutes % 60;
            write_line("Status: Warning!");
            write_line( name + "crossing in " + to_string(hours) + "h " + to_string(minutes) + "m." + " Please recalibrate your telescope.");
        }
        else
        {
            write_line("Status: Safe (No immediate flip needed)");
        }
    }

    // By default, celestial objects are drawn as circles.
    virtual void draw_on_map(double x, double y, double radius, color draw_color, bool is_target)
    {
        if (is_target)
        {
            fill_circle(draw_color, x, y, radius);
            draw_circle(COLOR_WHITE, x, y, radius + 6.0 ); 
        }
        else
        {
            fill_circle(draw_color, x, y, radius);
        }
    }
};

class Planet : public celestial_object
{
public:
    Planet(string names, double ra, double dec, double mag, color colors)
    {
        name = names;
        right_ascension = ra;
        declination = dec;
        apparent_magnitude = mag;
        exclusive_color = colors;
    }

    void print_info() override
    {
        write_line("Select Target: " + name + "(Planet)");
        celestial_object::print_info();
    }
};

class Deep_sky_object : public celestial_object
{
public:
    string object_type;

    Deep_sky_object(string names, double ra, double dec, double mag, color colors, string type)
    {
        name = names;
        right_ascension = ra;
        declination = dec;
        apparent_magnitude = mag;
        exclusive_color = colors;
        object_type = type;
    }

    void print_info() override
    {
        write_line("Select Target: " + name + "(" + object_type + ")");
        celestial_object::print_info();
    }

    // If it is a deep-sky object, draw it as a triangle.
    void draw_on_map(double x, double y, double radius, color draw_color, bool is_target) override
    {
        double x1 = x;
        double y1 = y - radius;
        double x2 = x - radius;
        double y2 = y + radius;
        double x3 = x + radius;
        double y3 = y + radius;

        if (is_target)
        {
            fill_triangle(draw_color, x1, y1, x2, y2, x3, y3);
            draw_triangle(COLOR_WHITE, x, y1 - 6, x2 - 6, y2 + 6, x3 + 6, y3 + 6);
        }
        else
        {
            fill_triangle(draw_color, x1, y1, x2, y2, x3, y3);
        }
    }
};

class Star : public celestial_object
{
public:
    Star(string names, double ra, double dec, double mag, color colors)
    {
        name = names;
        right_ascension = ra;
        declination = dec;
        apparent_magnitude = mag;
        exclusive_color = colors;
    }

    void print_info() override
    {
        write_line("Select Target: " + name + "(Star)");
        celestial_object::print_info();
    }
};

class Satellite : public celestial_object
{
public:
    Satellite(string names, double ra, double dec, double mag, color colors)
    {
        name = names;
        right_ascension = ra;
        declination = dec;
        apparent_magnitude = mag;
        exclusive_color = colors;
    }

    void print_info() override
    {
        write_line("Select Target: " + name + "(Satellite)");
        celestial_object::print_info();
    }
};

#endif