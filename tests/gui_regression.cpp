#include "../gui.h"
#include <cmath>
#include <iostream>

int checks = 0;
int failures = 0;

void expect(bool passed, const string &description)
{
    checks += 1;
    if (!passed)
    {
        failures += 1;
        std::cerr << "FAIL: " << description << '\n';
    }
}

int main()
{
    struct DateCase
    {
        string year, month, day, hour, minute, second;
        bool valid;
    };
    const DateCase dates[] = {
        {"2026", "10", "15", "20", "00", "00", true},
        {"2024", "02", "29", "23", "59", "59", true},
        {"2000", "02", "29", "00", "00", "00", true},
        {"1", "1", "1", "0", "0", "0", true},
        {"9999", "12", "31", "23", "59", "59", true},
        {"2026", "02", "29", "20", "00", "00", false},
        {"1900", "02", "29", "20", "00", "00", false},
        {"2026", "04", "31", "20", "00", "00", false},
        {"2026", "00", "15", "20", "00", "00", false},
        {"2026", "13", "15", "20", "00", "00", false},
        {"2026", "10", "00", "20", "00", "00", false},
        {"2026", "10", "32", "20", "00", "00", false},
        {"2026", "10", "15", "-1", "00", "00", false},
        {"2026", "10", "15", "24", "00", "00", false},
        {"2026", "10", "15", "20", "60", "00", false},
        {"2026", "10", "15", "20", "00", "60", false},
        {"2026", "10", "15", "20", "-1", "00", false},
        {"2026", "10", "15", "20", "00", "-1", false},
        {"0", "10", "15", "20", "00", "00", false},
        {"-1", "10", "15", "20", "00", "00", false},
        {"10000", "10", "15", "20", "00", "00", false},
        {"99999999999999999999", "10", "15", "20", "00", "00", false},
        {"2026", "99999999999999999999", "15", "20", "00", "00", false},
        {"", "10", "15", "20", "00", "00", false},
        {"2026", "", "15", "20", "00", "00", false},
        {"2026", "10", "", "20", "00", "00", false},
        {"2026", "10", "15", "", "00", "00", false},
        {"2026", "10", "15", "20", "", "00", false},
        {"2026", "10", "15", "20", "00", "", false},
        {"2026", "2.5", "15", "20", "00", "00", false},
        {"year", "10", "15", "20", "00", "00", false}
    };

    for (const DateCase &test : dates)
    {
        GUI_State state;
        state.input_year = test.year;
        state.input_month = test.month;
        state.input_day = test.day;
        state.input_hour = test.hour;
        state.input_minute = test.minute;
        state.input_second = test.second;
        expect(check_timeboxs(state) == test.valid,
               "date " + test.year + "/" + test.month + "/" + test.day +
               " " + test.hour + ":" + test.minute + ":" + test.second);
    }

    struct ShiftCase
    {
        int year, month, day, hour, change;
        int expected_year, expected_month, expected_day, expected_hour;
        bool changed;
    };
    const ShiftCase shifts[] = {
        {2024, 3, 1, 0, -1, 2024, 2, 29, 23, true},
        {2025, 3, 1, 0, -1, 2025, 2, 28, 23, true},
        {1900, 3, 1, 0, -1, 1900, 2, 28, 23, true},
        {2000, 2, 28, 23, 1, 2000, 2, 29, 0, true},
        {2024, 2, 29, 23, 1, 2024, 3, 1, 0, true},
        {2026, 4, 30, 23, 1, 2026, 5, 1, 0, true},
        {2026, 12, 31, 23, 1, 2027, 1, 1, 0, true},
        {2027, 1, 1, 0, -1, 2026, 12, 31, 23, true},
        {1, 1, 1, 0, -1, 1, 1, 1, 0, false},
        {9999, 12, 31, 23, 1, 9999, 12, 31, 23, false},
        {2026, 10, 15, 20, 0, 2026, 10, 15, 20, false}
    };

    for (const ShiftCase &test : shifts)
    {
        observer_data observer{};
        observer.year = test.year;
        observer.month = test.month;
        observer.day = test.day;
        observer.hour = test.hour;
        observer.minute = 15;
        observer.second = 45;
        observer.timezone = 5.75;
        bool changed = shift_observation_hour(observer, test.change);
        expect(changed == test.changed && observer.year == test.expected_year &&
               observer.month == test.expected_month && observer.day == test.expected_day &&
               observer.hour == test.expected_hour && observer.minute == 15 &&
               observer.second == 45 && observer.timezone == 5.75,
               "hour shift from " + to_string(test.year) + "/" + to_string(test.month) +
               "/" + to_string(test.day) + " " + to_string(test.hour));
    }

    expect(format_utc_offset(8) == "(UTC+8)", "whole-hour UTC offset");
    expect(format_utc_offset(0) == "(UTC+0)", "zero UTC offset");
    expect(format_utc_offset(-6) == "(UTC-6)", "negative UTC offset");
    expect(format_utc_offset(5.5) == "(UTC+5:30)", "half-hour UTC offset");
    expect(format_utc_offset(5.75) == "(UTC+5:45)", "quarter-hour UTC offset");
    expect(format_utc_offset(-3.5) == "(UTC-3:30)", "negative half-hour offset");
    expect(format_utc_offset(-0.5) == "(UTC-0:30)", "negative sub-hour offset");
    expect(format_utc_offset(5.0 + 5.0 / 60.0) == "(UTC+5:05)", "minute zero padding");

    Star_map_data catalog{};
    catalog.load_cities("cities.csv");
    catalog.load_stars("star.csv");
    expect(catalog.cities.length() == 212, "city fixture loaded from repository root");
    expect(catalog.celestial_objects.length() == 119, "object fixture loaded from repository root");

    for (int i = 0; i < catalog.celestial_objects.length(); i++)
    {
        celestial_object *object = catalog.celestial_objects[i];
        expect(object_marker_radius(object->apparent_magnitude) >= 1.0,
               "visible and clickable radius for " + object->name);
    }
    expect(object_marker_radius(11.0) == 1.0, "magnitude 11 stays visible");
    expect(object_marker_radius(10.0) == 1.0, "magnitude 10 stays visible");
    expect(std::abs(object_marker_radius(-26.74) - 18.37) < 0.000001, "Sun size unchanged");
    expect(std::abs(object_marker_radius(-11.46) - 10.73) < 0.000001, "Moon size unchanged");

    std::cout << checks << " checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
