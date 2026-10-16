#include "observation_analysis.h"
#include <algorithm>
#include <cmath>
#include <limits>

static const double PI = 3.14159265358979323846;

static int days_in_month(int month, int year)
{
    if (month == 2)
    {
        return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0 ? 29 : 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11)
    {
        return 30;
    }
    return 31;
}

static long long days_before_year(int year)
{
    long long previous_year = year - 1;
    return previous_year * 365 + previous_year / 4 - previous_year / 100 + previous_year / 400;
}

static long long day_number(const observer_data &observer)
{
    long long days = days_before_year(observer.year);
    for (int month = 1; month < observer.month; month++)
    {
        days += days_in_month(month, observer.year);
    }
    return days + observer.day - 1;
}

static long long local_minute_number(const observer_data &observer)
{
    return day_number(observer) * 1440 + observer.hour * 60 + observer.minute;
}

static long long final_local_minute()
{
    return days_before_year(10000) * 1440 - 1;
}

static bool valid_position_inputs(const observer_data &observer, const celestial_object &target)
{
    return valid_observation(observer) &&
           std::isfinite(observer.current_city.latitude) &&
           observer.current_city.latitude >= -90 && observer.current_city.latitude <= 90 &&
           std::isfinite(observer.current_city.longitude) &&
           observer.current_city.longitude >= -180 && observer.current_city.longitude <= 180 &&
           std::isfinite(target.right_ascension) && std::isfinite(target.declination) &&
           target.declination >= -90 && target.declination <= 90;
}

bool valid_observation(const observer_data &observer)
{
    if (observer.year < 1 || observer.year > 9999 || observer.month < 1 || observer.month > 12)
    {
        return false;
    }
    return observer.day >= 1 && observer.day <= days_in_month(observer.month, observer.year) &&
           observer.hour >= 0 && observer.hour <= 23 &&
           observer.minute >= 0 && observer.minute <= 59 &&
           observer.second >= 0 && observer.second <= 59 &&
           std::isfinite(observer.timezone) && observer.timezone >= -24 && observer.timezone <= 24;
}

double observation_julian_date(const observer_data &observer)
{
    if (!valid_observation(observer))
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Proleptic Gregorian 0001-01-01 00:00 UTC is Julian date 1721425.5.
    return 1721425.5 + static_cast<double>(day_number(observer)) +
           observer.hour / 24.0 + observer.minute / 1440.0 + observer.second / 86400.0 -
           observer.timezone / 24.0;
}

bool shift_observation_minutes(observer_data &observer, int delta)
{
    if (!valid_observation(observer))
    {
        return false;
    }

    long long next_minute = local_minute_number(observer) + static_cast<long long>(delta);
    if (next_minute < 0 || next_minute > final_local_minute())
    {
        return false;
    }

    long long next_day = next_minute / 1440;
    int lower_year = 1;
    int upper_year = 9999;
    while (lower_year < upper_year)
    {
        int middle_year = (lower_year + upper_year + 1) / 2;
        if (days_before_year(middle_year) <= next_day)
        {
            lower_year = middle_year;
        }
        else
        {
            upper_year = middle_year - 1;
        }
    }

    int next_year = lower_year;
    int remaining_days = static_cast<int>(next_day - days_before_year(next_year));
    int next_month = 1;
    while (remaining_days >= days_in_month(next_month, next_year))
    {
        remaining_days -= days_in_month(next_month, next_year);
        next_month += 1;
    }

    // Assign only after all bounds checks; a rejected shift leaves every field intact.
    observer.year = next_year;
    observer.month = next_month;
    observer.day = remaining_days + 1;
    observer.hour = static_cast<int>((next_minute % 1440) / 60);
    observer.minute = static_cast<int>(next_minute % 60);
    return true;
}

sky_position position_at_observation(const observer_data &observer, const celestial_object &target)
{
    if (!valid_position_inputs(observer, target))
    {
        double invalid = std::numeric_limits<double>::quiet_NaN();
        return {invalid, invalid};
    }

    double days = observation_julian_date(observer) - 2451545.0;
    double sidereal_degrees = std::fmod(280.46061837 + 360.98564736629 * days, 360.0);
    double local_sidereal_degrees = std::fmod(sidereal_degrees + observer.current_city.longitude, 360.0);
    double right_ascension_hours = std::fmod(target.right_ascension, 24.0);
    double hour_angle = (local_sidereal_degrees - right_ascension_hours * 15.0) * PI / 180.0;
    double latitude = observer.current_city.latitude * PI / 180.0;
    double declination = target.declination * PI / 180.0;

    double east = -std::cos(declination) * std::sin(hour_angle);
    double north = std::sin(declination) * std::cos(latitude) -
                   std::cos(declination) * std::cos(hour_angle) * std::sin(latitude);
    double up = std::sin(declination) * std::sin(latitude) +
                std::cos(declination) * std::cos(hour_angle) * std::cos(latitude);

    sky_position result;
    result.altitude = std::asin(std::max(-1.0, std::min(1.0, up))) * 180.0 / PI;
    if (std::hypot(east, north) > 1e-12)
    {
        result.azimuth = std::atan2(east, north) * 180.0 / PI;
        if (result.azimuth < 0)
        {
            result.azimuth += 360.0;
        }
        if (result.azimuth >= 360.0)
        {
            result.azimuth = 0;
        }
    }
    return result;
}

bool supports_time_analysis(const celestial_object &target)
{
    return dynamic_cast<const Deep_sky_object *>(&target) != nullptr;
}

target_analysis analyze_target(const observer_data &anchor, const celestial_object &target,
                               int duration_minutes, int step_minutes, double threshold)
{
    target_analysis result;
    if (!supports_time_analysis(target) || !valid_position_inputs(anchor, target) ||
        duration_minutes < 0 || duration_minutes > 1440 || step_minutes <= 0 ||
        !std::isfinite(threshold) || threshold < -90 || threshold > 90)
    {
        return result;
    }

    long long available_minutes = final_local_minute() - local_minute_number(anchor);
    result.duration_minutes = static_cast<int>(std::min<long long>(duration_minutes, available_minutes));
    result.step_minutes = step_minutes;

    int minute = 0;
    bool in_window = false;
    altitude_interval current_window;
    while (true)
    {
        observer_data sample_observer = anchor;
        shift_observation_minutes(sample_observer, minute);
        altitude_sample sample;
        sample.minute_offset = minute;
        sample.position = position_at_observation(sample_observer, target);
        result.samples.push_back(sample);

        if (sample.position.altitude > result.peak_altitude)
        {
            result.peak_altitude = sample.position.altitude;
            result.peak_minute = minute;
        }

        // The tolerance includes a sample lying exactly on the threshold despite rounding.
        bool above_threshold = sample.position.altitude + 1e-9 >= threshold;
        if (above_threshold)
        {
            if (!in_window)
            {
                current_window.start_minute = minute;
                in_window = true;
            }
            current_window.end_minute = minute;
        }
        else if (in_window)
        {
            result.windows.push_back(current_window);
            in_window = false;
        }

        if (minute == result.duration_minutes)
        {
            break;
        }
        minute = static_cast<int>(std::min<long long>(static_cast<long long>(minute) + step_minutes,
                                                     result.duration_minutes));
    }

    if (in_window)
    {
        result.windows.push_back(current_window);
    }
    return result;
}
