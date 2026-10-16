#ifndef OBSERVATION_ANALYSIS_H
#define OBSERVATION_ANALYSIS_H

#include "star_map.h"

struct sky_position
{
    double altitude = 0;
    double azimuth = 0;
};

struct altitude_sample
{
    int minute_offset = 0;
    sky_position position;
};

struct altitude_interval
{
    int start_minute = 0;
    int end_minute = 0;
};

struct target_analysis
{
    std::vector<altitude_sample> samples;
    std::vector<altitude_interval> windows;
    double peak_altitude = -90;
    int peak_minute = 0;
    int duration_minutes = 0;
    int step_minutes = 15;
};

/** Validate a Gregorian date/time in years 1-9999 and a civil UTC offset. */
bool valid_observation(const observer_data &observer);

/** Return the UTC Julian date, or NaN for invalid observation fields. */
double observation_julian_date(const observer_data &observer);

/** Shift local calendar minutes atomically; preserve seconds, city, and timezone. */
bool shift_observation_minutes(observer_data &observer, int delta);

/** Return a catalog position without changing the observer or celestial object.
 * Invalid coordinates return NaN. Azimuth is defined as zero at the zenith/nadir.
 */
sky_position position_at_observation(const observer_data &observer, const celestial_object &target);

/** Only fixed deep-sky catalog objects support the planning analysis. */
bool supports_time_analysis(const celestial_object &target);

/** Sample 0-24 hours from a fixed anchor, including the final endpoint.
 * Windows join consecutive samples at/above the altitude threshold; they are
 * sampled altitude windows, not exact rise/set times or visibility guarantees.
 * Invalid arguments and unsupported targets return an empty result. The range
 * is truncated when it would extend beyond local year 9999.
 */
target_analysis analyze_target(const observer_data &anchor, const celestial_object &target,
                               int duration_minutes = 1440, int step_minutes = 15,
                               double threshold = 30);

#endif
