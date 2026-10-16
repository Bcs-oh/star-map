#include "../observation_analysis.h"
#include <cmath>
#include <climits>
#include <iostream>
#include <limits>

static int checks = 0;
static int failures = 0;

static void expect(bool passed, const string &description)
{
    checks++;
    if (!passed)
    {
        failures++;
        std::cerr << "FAIL: " << description << '\n';
    }
}

static bool near(double actual, double expected, double tolerance = 0.000001)
{
    return std::isfinite(actual) && std::abs(actual - expected) <= tolerance;
}

static double angle_difference(double first, double second)
{
    return std::abs(std::remainder(first - second, 360.0));
}

static observer_data observation(int year = 2026, int month = 10, int day = 15,
                                 int hour = 20, int minute = 0, int second = 0,
                                 double timezone = 8, double latitude = 3.139,
                                 double longitude = 101.6869)
{
    observer_data observer{};
    observer.current_city = {"Test city", latitude, longitude, "TEST", timezone};
    observer.year = year;
    observer.month = month;
    observer.day = day;
    observer.hour = hour;
    observer.minute = minute;
    observer.second = second;
    observer.timezone = timezone;
    return observer;
}

static bool same_observer(const observer_data &first, const observer_data &second)
{
    return first.year == second.year && first.month == second.month && first.day == second.day &&
           first.hour == second.hour && first.minute == second.minute && first.second == second.second &&
           first.timezone == second.timezone &&
           first.current_city.city_name == second.current_city.city_name &&
           first.current_city.latitude == second.current_city.latitude &&
           first.current_city.longitude == second.current_city.longitude &&
           first.current_city.country == second.current_city.country &&
           first.current_city.timezone == second.current_city.timezone;
}

int main()
{
    const observer_data valid_dates[] = {
        observation(), observation(1, 1, 1, 0, 0, 0, 0),
        observation(9999, 12, 31, 23, 59, 59, 0),
        observation(2024, 2, 29), observation(2000, 2, 29),
        observation(2026, 10, 15, 23, 59, 59, 5.75)
    };
    for (const observer_data &observer : valid_dates)
    {
        expect(valid_observation(observer), "valid Gregorian observation");
    }
    const observer_data invalid_dates[] = {
        observation(0), observation(10000), observation(1900, 2, 29),
        observation(2026, 2, 29), observation(2026, 4, 31),
        observation(2026, 0), observation(2026, 13), observation(2026, 10, 0),
        observation(2026, 10, 32), observation(2026, 10, 15, -1),
        observation(2026, 10, 15, 24), observation(2026, 10, 15, 20, -1),
        observation(2026, 10, 15, 20, 60), observation(2026, 10, 15, 20, 0, -1),
        observation(2026, 10, 15, 20, 0, 60),
        observation(2026, 10, 15, 20, 0, 0, std::numeric_limits<double>::infinity()),
        observation(2026, 10, 15, 20, 0, 0, 25)
    };
    for (const observer_data &observer : invalid_dates)
    {
        expect(!valid_observation(observer), "invalid observation rejected");
        expect(std::isnan(observation_julian_date(observer)), "invalid date has no Julian date");
    }

    expect(near(observation_julian_date(observation(2000, 1, 1, 12, 0, 0, 0)), 2451545.0),
           "independent J2000 noon epoch");
    expect(near(observation_julian_date(observation(1970, 1, 1, 0, 0, 0, 0)), 2440587.5),
           "independent Unix epoch Julian date");
    expect(near(observation_julian_date(observation(1, 1, 1, 0, 0, 0, 0)), 1721425.5),
           "proleptic Gregorian first day");
    expect(near(observation_julian_date(observation(2000, 1, 1, 17, 45, 0, 5.75)), 2451545.0),
           "quarter-hour UTC offset preserves J2000 instant");
    expect(near(observation_julian_date(observation(2024, 3, 1)) -
                observation_julian_date(observation(2024, 2, 28)), 2), "leap February spans two days");
    expect(near(observation_julian_date(observation(1900, 3, 1)) -
                observation_julian_date(observation(1900, 2, 28)), 1), "century February spans one day");

    struct ShiftCase
    {
        observer_data start;
        int delta;
        int year, month, day, hour, minute;
        bool changed;
    };
    const ShiftCase shifts[] = {
        {observation(2024, 3, 1, 0, 5, 45, 5.75), -15, 2024, 2, 29, 23, 50, true},
        {observation(1900, 3, 1, 0, 5, 45, 5.75), -15, 1900, 2, 28, 23, 50, true},
        {observation(2000, 2, 28, 23, 55, 45, 5.75), 15, 2000, 2, 29, 0, 10, true},
        {observation(2026, 4, 30, 23, 55, 45, 5.75), 15, 2026, 5, 1, 0, 10, true},
        {observation(2026, 12, 31, 23, 55, 45, 5.75), 15, 2027, 1, 1, 0, 10, true},
        {observation(2027, 1, 1, 0, 5, 45, 5.75), -15, 2026, 12, 31, 23, 50, true},
        {observation(2026, 10, 15, 20, 0, 45, 5.75), 90, 2026, 10, 15, 21, 30, true},
        {observation(2026, 10, 15, 20, 0, 45, 5.75), -90, 2026, 10, 15, 18, 30, true},
        {observation(2026, 10, 15, 20, 0, 45, 5.75), 0, 2026, 10, 15, 20, 0, true},
        {observation(1, 1, 1, 0, 0, 45, 5.75), -1, 1, 1, 1, 0, 0, false},
        {observation(9999, 12, 31, 23, 59, 45, 5.75), 1, 9999, 12, 31, 23, 59, false}
    };
    for (const ShiftCase &test : shifts)
    {
        observer_data shifted = test.start;
        bool changed = shift_observation_minutes(shifted, test.delta);
        expect(changed == test.changed && shifted.year == test.year && shifted.month == test.month &&
               shifted.day == test.day && shifted.hour == test.hour && shifted.minute == test.minute,
               "minute shift calendar result");
        expect(shifted.second == test.start.second && shifted.timezone == test.start.timezone &&
               shifted.current_city.city_name == test.start.current_city.city_name &&
               shifted.current_city.latitude == test.start.current_city.latitude &&
               shifted.current_city.longitude == test.start.current_city.longitude &&
               shifted.current_city.country == test.start.current_city.country &&
               shifted.current_city.timezone == test.start.current_city.timezone,
               "minute shift preserves seconds, timezone, and city");
        if (!changed)
        {
            expect(same_observer(shifted, test.start), "rejected minute shift is atomic");
        }
    }
    observer_data invalid_shift = observation(2026, 2, 29);
    observer_data invalid_before = invalid_shift;
    expect(!shift_observation_minutes(invalid_shift, 15) && same_observer(invalid_shift, invalid_before),
           "invalid starting date does not mutate");
    observer_data large_shift = observation(5000, 1, 1, 0, 0, 45, 5.75);
    observer_data large_before = large_shift;
    expect(shift_observation_minutes(large_shift, INT_MAX), "large minute delta does not overflow");
    expect(near((observation_julian_date(large_shift) - observation_julian_date(large_before)) * 1440,
                static_cast<double>(INT_MAX), 0.00001), "large delta represents requested elapsed minutes");
    observer_data out_of_range = observation(9999, 12, 31, 23, 59, 45);
    observer_data out_of_range_before = out_of_range;
    expect(!shift_observation_minutes(out_of_range, INT_MAX) &&
           same_observer(out_of_range, out_of_range_before), "large rejected delta is atomic");

    observer_data j2000 = observation(2000, 1, 1, 12, 0, 0, 0, 0, 0);
    const double j2000_ra = 280.46061837 / 15.0;
    Deep_sky_object transit("Transit fixture", j2000_ra, 0, 4, COLOR_WHITE, "Test");
    sky_position transit_position = position_at_observation(j2000, transit);
    expect(near(transit_position.altitude, 90), "equatorial meridian transit reaches zenith");
    expect(near(transit_position.azimuth, 0), "zenith azimuth uses documented finite convention");
    Deep_sky_object east("East fixture", j2000_ra + 6, 0, 4, COLOR_WHITE, "Test");
    Deep_sky_object west("West fixture", j2000_ra - 6, 0, 4, COLOR_WHITE, "Test");
    sky_position east_position = position_at_observation(j2000, east);
    sky_position west_position = position_at_observation(j2000, west);
    expect(near(east_position.altitude, 0) && near(east_position.azimuth, 90), "eastern horizon geometry");
    expect(near(west_position.altitude, 0) && near(west_position.azimuth, 270), "western horizon geometry");
    observer_data northern = j2000;
    northern.current_city.latitude = 45;
    sky_position southern_transit = position_at_observation(northern, transit);
    expect(near(southern_transit.altitude, 45) && near(southern_transit.azimuth, 180),
           "equatorial object transits south at northern latitude");
    Deep_sky_object north_target("North fixture", j2000_ra, 60, 4, COLOR_WHITE, "Test");
    sky_position northern_transit = position_at_observation(northern, north_target);
    expect(near(northern_transit.altitude, 75) && angle_difference(northern_transit.azimuth, 0) < 0.000001,
           "northern meridian geometry");
    observer_data north_pole = j2000;
    north_pole.current_city.latitude = 90;
    Deep_sky_object pole_target("Pole fixture", 0, 30, 4, COLOR_WHITE, "Test");
    sky_position polar_position = position_at_observation(north_pole, pole_target);
    expect(near(polar_position.altitude, 30) && std::isfinite(polar_position.azimuth), "polar position remains finite");
    observer_data nepal = observation(2000, 1, 1, 17, 45, 0, 5.75, 0, 0);
    sky_position nepal_position = position_at_observation(nepal, east);
    expect(near(nepal_position.altitude, east_position.altitude) &&
           angle_difference(nepal_position.azimuth, east_position.azimuth) < 0.000001,
           "equivalent UTC instants share positions with fractional timezone");
    observer_data bad_location = j2000;
    bad_location.current_city.latitude = 91;
    expect(std::isnan(position_at_observation(bad_location, transit).altitude), "invalid latitude has no position");

    Star_map_data legacy{};
    legacy.load_stars("star.csv");
    expect(legacy.celestial_objects.length() == 119, "catalog fixture loaded from repository root");
    const observer_data comparison_observers[] = {
        observation(), observation(2024, 2, 29, 23, 59, 45, 5.75, 27.7172, 85.3240),
        observation(2000, 1, 1, 12, 0, 0, 0, -37.8136, 144.9631),
        observation(1900, 3, 1, 7, 5, 15, -3.5, 51.5074, -0.1278)
    };
    for (const observer_data &observer : comparison_observers)
    {
        legacy.observer = observer;
        expect(near(observation_julian_date(observer), legacy.calclute_JD()), "Julian date matches retained algorithm");
        for (int index = 0; index < legacy.celestial_objects.length(); index++)
        {
            celestial_object &target = *legacy.celestial_objects[index];
            legacy.calculate_celestical_object(target);
            sky_position position = position_at_observation(observer, target);
            expect(near(position.altitude, target.altitude) &&
                   angle_difference(position.azimuth, target.azimuth) < 0.000001,
                   "position matches retained catalog algorithm for " + target.name);
        }
    }

    Deep_sky_object target("Analysis fixture", 0.712, 41.269, 3.44, COLOR_WHITE, "Galaxy");
    target.altitude = 12.3;
    target.azimuth = 45.6;
    target.visibility = GOOD;
    target.meridian_warning = true;
    target.hours_to_meridian = 2.5;
    observer_data anchor = observation();
    observer_data anchor_before = anchor;
    target_analysis analysis = analyze_target(anchor, target);
    expect(analysis.samples.size() == 97 && analysis.duration_minutes == 1440 && analysis.step_minutes == 15,
           "default analysis has fixed 24-hour, 97-sample range");
    expect(same_observer(anchor, anchor_before) && target.altitude == 12.3 && target.azimuth == 45.6 &&
           target.visibility == GOOD && target.meridian_warning && target.hours_to_meridian == 2.5 &&
           target.name == "Analysis fixture" && target.right_ascension == 0.712 && target.declination == 41.269,
           "sampling never mutates observer or selected target");
    double measured_peak = -90;
    int measured_peak_minute = 0;
    for (int index = 0; index < static_cast<int>(analysis.samples.size()); index++)
    {
        const altitude_sample &sample = analysis.samples[index];
        observer_data at_sample = anchor;
        shift_observation_minutes(at_sample, index * 15);
        sky_position expected = position_at_observation(at_sample, target);
        expect(sample.minute_offset == index * 15 && near(sample.position.altitude, expected.altitude) &&
               angle_difference(sample.position.azimuth, expected.azimuth) < 0.000001,
               "sample remains anchored to original observation");
        if (sample.position.altitude > measured_peak)
        {
            measured_peak = sample.position.altitude;
            measured_peak_minute = sample.minute_offset;
        }
    }
    expect(near(analysis.peak_altitude, measured_peak) && analysis.peak_minute == measured_peak_minute,
           "reported peak identifies highest sampled altitude");
    for (const altitude_sample &sample : analysis.samples)
    {
        bool included = false;
        for (const altitude_interval &window : analysis.windows)
        {
            included = included || (sample.minute_offset >= window.start_minute && sample.minute_offset <= window.end_minute);
        }
        expect(included == (sample.position.altitude + 1e-9 >= 30), "sampled threshold windows include exactly qualifying samples");
    }

    observer_data circumpolar = observation(2026, 10, 15, 20, 0, 0, 0, 80, 0);
    Deep_sky_object always_high("Always high", 0, 85, 4, COLOR_WHITE, "Test");
    Deep_sky_object always_low("Always low", 0, -85, 4, COLOR_WHITE, "Test");
    target_analysis all_day = analyze_target(circumpolar, always_high, 1440, 15, 0);
    expect(all_day.windows.size() == 1 && all_day.windows[0].start_minute == 0 && all_day.windows[0].end_minute == 1440,
           "all-day threshold interval includes both endpoints");
    expect(analyze_target(circumpolar, always_low, 1440, 15, 0).windows.empty(), "never-above-threshold has no windows");
    target_analysis exact_threshold = analyze_target(north_pole, pole_target, 60, 15, 30);
    expect(exact_threshold.windows.size() == 1 && exact_threshold.windows[0].end_minute == 60,
           "threshold equality tolerates numerical rounding");
    target_analysis partial_step = analyze_target(anchor, target, 62, 15);
    expect(partial_step.samples.size() == 6 && partial_step.samples.back().minute_offset == 62,
           "non-divisible duration includes exact final endpoint");
    target_analysis large_step = analyze_target(anchor, target, 60, INT_MAX);
    expect(large_step.samples.size() == 2 && large_step.samples.back().minute_offset == 60,
           "large step remains bounded and includes endpoint");
    expect(analyze_target(anchor, target, 0).samples.size() == 1, "zero-duration analysis contains anchor sample");
    observer_data terminal = observation(9999, 12, 31, 23, 50, 45, 5.75);
    target_analysis truncated = analyze_target(terminal, target);
    expect(truncated.duration_minutes == 9 && truncated.samples.size() == 2 &&
           truncated.samples.back().minute_offset == 9, "analysis truncates safely at final representable local minute");
    expect(analyze_target(anchor, target, -1).samples.empty() &&
           analyze_target(anchor, target, 1441).samples.empty() &&
           analyze_target(anchor, target, 60, 0).samples.empty() &&
           analyze_target(anchor, target, 60, 15, 91).samples.empty() &&
           analyze_target(anchor, target, 60, 15, std::numeric_limits<double>::quiet_NaN()).samples.empty(),
           "invalid analysis parameters rejected");
    expect(analyze_target(bad_location, target).samples.empty(), "invalid location analysis rejected");
    Star sun("Sun", 4, 22, -26.74, COLOR_RED);
    Satellite moon("Moon", 19, -24, -11, COLOR_WHITE);
    Planet mars("Mars", 12, 5, 1, COLOR_RED);
    expect(supports_time_analysis(target) && !supports_time_analysis(sun) &&
           !supports_time_analysis(moon) && !supports_time_analysis(mars), "time analysis limited to deep-sky catalog types");
    expect(analyze_target(anchor, sun).samples.empty() && analyze_target(anchor, moon).samples.empty() &&
           analyze_target(anchor, mars).samples.empty(), "static solar-system catalog data is not offered as predictions");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
