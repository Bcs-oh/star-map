#include "observation_plan.h"
#include "observation_analysis.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iterator>
#include <locale>
#include <set>
#include <sstream>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace
{
const std::size_t max_file_bytes = 1024 * 1024;
const std::size_t max_field_bytes = 4096;
const std::size_t max_rows = 4096;

std::string folded(const std::string &value)
{
    std::string result = value;
    for (char &c : result) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return result;
}

bool valid_text(const std::string &value, bool allow_empty = false)
{
    return (allow_empty || !value.empty()) && value.size() <= max_field_bytes &&
           value.find('\0') == std::string::npos;
}

std::string csv_field(const std::string &value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos) return value;
    std::string escaped = "\"";
    for (char c : value)
    {
        if (c == '"') escaped += '"';
        escaped += c;
    }
    return escaped + '"';
}

void csv_row(std::ostringstream &out, const std::vector<std::string> &fields)
{
    for (std::size_t i = 0; i < fields.size(); ++i)
    {
        if (i) out << ',';
        out << csv_field(fields[i]);
    }
    out << '\n';
}

std::string number(double value)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(17) << value;
    return out.str();
}

template <typename T>
bool parse_number(const std::string &text, T &value)
{
    if (text.empty()) return false;
    std::istringstream in(text);
    in.imbue(std::locale::classic());
    T parsed{};
    in >> std::noskipws >> parsed;
    if (in.fail() || !in.eof()) return false;
    value = parsed;
    return true;
}

// A small strict CSV reader: quoted fields can contain commas, quotes and newlines.
bool parse_csv(const std::string &text, std::vector<std::vector<std::string>> &rows,
               std::string &error)
{
    enum state { start, unquoted, quoted, closed } current = start;
    std::vector<std::string> row;
    std::string field;
    bool record_started = false;
    const auto finish_field = [&]() { row.push_back(field); field.clear(); current = start; };
    const auto finish_row = [&]() { finish_field(); rows.push_back(row); row.clear(); record_started = false; };
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        const char c = text[i];
        if (c == '\0') { error = "Plan contains a NUL byte."; return false; }
        record_started = true;
        if (current == quoted)
        {
            if (c == '"') current = closed;
            else field += c;
        }
        else if (current == closed && c == '"')
        {
            field += '"';
            current = quoted;
        }
        else if (c == ',') finish_field();
        else if (c == '\n' || c == '\r')
        {
            if (c == '\r')
            {
                if (i + 1 == text.size() || text[i + 1] != '\n')
                { error = "Plan has an invalid line ending."; return false; }
                ++i;
            }
            finish_row();
        }
        else if (current == closed)
        { error = "Unexpected text after a quoted CSV field."; return false; }
        else if (c == '"')
        {
            if (current != start)
            { error = "Unexpected quote in an unquoted CSV field."; return false; }
            current = quoted;
        }
        else { field += c; current = unquoted; }
        if (field.size() > max_field_bytes || row.size() > 16 || rows.size() > max_rows)
        { error = "Plan exceeds the supported size limits."; return false; }
    }
    if (current == quoted) { error = "Plan has an unterminated quoted field."; return false; }
    if (record_started || !row.empty() || !field.empty()) finish_row();
    if (rows.size() > max_rows) { error = "Plan exceeds the supported row limit."; return false; }
    return true;
}

bool valid_session(const observation_session &session, std::string &error)
{
    const city_data &city = session.anchor.current_city;
    if (!valid_text(city.city_name) || !std::isfinite(city.latitude) ||
        !std::isfinite(city.longitude) || !std::isfinite(city.timezone) ||
        city.latitude < -90 || city.latitude > 90 || city.longitude < -180 ||
        city.longitude > 180 || city.timezone < -14 || city.timezone > 14 ||
        !valid_observation(session.anchor) ||
        std::fabs(session.anchor.timezone - city.timezone) > 1e-9)
    { error = "Choose a valid city, local date and time before saving or exporting."; return false; }
    if (!std::isfinite(session.min_altitude) || session.min_altitude < 0 || session.min_altitude > 90)
    { error = "Minimum altitude must be between 0 and 90 degrees."; return false; }
    if (!valid_text(session.selected_target, true) || session.targets.size() > max_rows - 6)
    { error = "Plan target names or count are invalid."; return false; }
    for (const std::string &name : session.targets)
        if (!valid_text(name)) { error = "Plan contains an empty or invalid target name."; return false; }
    return true;
}

const city_data *find_city(const Star_map_data &catalog, const std::string &name)
{
    const std::string key = folded(name);
    for (int i = 0; i < catalog.cities.length(); ++i)
        if (folded(catalog.cities[i].city_name) == key) return &catalog.cities[i];
    return nullptr;
}

const celestial_object *find_target(const Star_map_data &catalog, const std::string &name)
{
    const std::string key = folded(name);
    for (int i = 0; i < catalog.celestial_objects.length(); ++i)
        if (catalog.celestial_objects[i] && folded(catalog.celestial_objects[i]->name) == key)
            return catalog.celestial_objects[i];
    return nullptr;
}

std::string object_type(const celestial_object &object)
{
    const Deep_sky_object *deep_sky = dynamic_cast<const Deep_sky_object *>(&object);
    if (deep_sky) return deep_sky->object_type;
    if (dynamic_cast<const Planet *>(&object)) return "Planet";
    if (dynamic_cast<const Satellite *>(&object)) return "Satellite";
    if (dynamic_cast<const Star *>(&object)) return "Star";
    return "Catalog object";
}

std::string local_time(observer_data observer, int minute_offset = 0)
{
    if (!shift_observation_minutes(observer, minute_offset)) return "Unsupported date";
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setfill('0') << std::setw(4) << observer.year << '-'
        << std::setw(2) << observer.month << '-' << std::setw(2) << observer.day << ' '
        << std::setw(2) << observer.hour << ':' << std::setw(2) << observer.minute << ':'
        << std::setw(2) << observer.second;
    return out.str();
}

// Use an exclusive adjacent temporary file, then atomically replace the destination.
// No existing temporary file or destination is truncated if writing fails.
bool atomic_write(const std::string &path, const std::string &content, std::string &error)
{
    if (path.empty() || path.find('\0') != std::string::npos)
    { error = "The output path is invalid."; return false; }
    static std::atomic<unsigned long long> sequence{0};
    const auto clock_value = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string temporary;
    int fd = -1;
    for (int attempt = 0; attempt < 100; ++attempt)
    {
        temporary = path + ".tmp-" + std::to_string(clock_value) + "-" +
                    std::to_string(sequence.fetch_add(1));
#ifdef _WIN32
        fd = _open(temporary.c_str(), _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
        fd = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
#endif
        if (fd >= 0 || errno != EEXIST) break;
    }
    if (fd < 0) { error = "Cannot create a temporary output file: " + std::string(std::strerror(errno)); return false; }
#ifdef _WIN32
    FILE *file = _fdopen(fd, "wb");
#else
    FILE *file = fdopen(fd, "wb");
#endif
    if (!file)
    {
#ifdef _WIN32
        _close(fd);
#else
        close(fd);
#endif
        std::remove(temporary.c_str());
        error = "Cannot open the temporary output stream.";
        return false;
    }
    const bool written = std::fwrite(content.data(), 1, content.size(), file) == content.size();
    const bool flushed = std::fflush(file) == 0;
    const bool closed_ok = std::fclose(file) == 0;
    if (!written || !flushed || !closed_ok)
    {
        std::remove(temporary.c_str());
        error = "Could not finish writing the output file; the original file was kept.";
        return false;
    }
#ifdef _WIN32
    const bool renamed = MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    const bool renamed = std::rename(temporary.c_str(), path.c_str()) == 0;
#endif
    if (!renamed)
    {
        std::remove(temporary.c_str());
        error = "Could not replace the output file; the original file was kept.";
        return false;
    }
    error.clear();
    return true;
}
}

bool save_observation_plan(const std::string &path, const observation_session &session,
                           std::string &error)
{
    error.clear();
    if (!valid_session(session, error)) return false;
    std::ostringstream out;
    out.imbue(std::locale::classic());
    csv_row(out, {"STAR_MAP_PLAN", "1"});
    csv_row(out, {"city", session.anchor.current_city.city_name});
    csv_row(out, {"time", std::to_string(session.anchor.year), std::to_string(session.anchor.month),
                  std::to_string(session.anchor.day), std::to_string(session.anchor.hour),
                  std::to_string(session.anchor.minute), std::to_string(session.anchor.second)});
    csv_row(out, {"minimum_altitude", number(session.min_altitude)});
    csv_row(out, {"above_horizon_only", session.above_horizon_only ? "1" : "0"});
    csv_row(out, {"selected_target", session.selected_target});
    std::set<std::string> seen;
    for (const std::string &target : session.targets)
        if (seen.insert(folded(target)).second) csv_row(out, {"target", target});
    const std::string content = out.str();
    if (content.size() > max_file_bytes)
    { error = "Plan exceeds the supported size limit."; return false; }
    return atomic_write(path, content, error);
}

bool load_observation_plan(const std::string &path, const Star_map_data &catalog,
                           observation_session &session, std::string &error)
{
    error.clear();
    if (path.empty() || path.find('\0') != std::string::npos)
    { error = "The plan path is invalid."; return false; }
    std::ifstream file(path, std::ios::binary);
    if (!file) { error = "Cannot open the saved plan."; return false; }
    file.seekg(0, std::ios::end);
    const std::streampos length = file.tellg();
    if (length < 0 || length > static_cast<std::streampos>(max_file_bytes))
    { error = "Saved plan is unreadable or too large."; return false; }
    file.seekg(0);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad()) { error = "Error while reading the saved plan."; return false; }
    if (content.size() > max_file_bytes || content.size() != static_cast<std::size_t>(length))
    { error = "Saved plan changed while it was being read."; return false; }
    std::vector<std::vector<std::string>> rows;
    if (!parse_csv(content, rows, error)) return false;
    if (rows.empty() || rows[0] != std::vector<std::string>{"STAR_MAP_PLAN", "1"})
    { error = "This is not a supported Star Map plan (version 1)."; return false; }

    observation_session candidate;
    std::set<std::string> keys;
    std::set<std::string> targets;
    for (std::size_t i = 1; i < rows.size(); ++i)
    {
        const std::vector<std::string> &row = rows[i];
        const std::string &key = row[0];
        const std::size_t required = key == "time" ? 7 : 2;
        if (row.size() != required)
        { error = "Wrong field count in saved plan row " + std::to_string(i + 1) + "."; return false; }
        if (key != "target" && !keys.insert(key).second)
        { error = "Saved plan contains a duplicate setting: " + key; return false; }
        if (key == "city")
        {
            const city_data *city = find_city(catalog, row[1]);
            if (!city) { error = "Saved plan contains an unknown city: " + row[1]; return false; }
            candidate.anchor.current_city = *city;
            candidate.anchor.timezone = city->timezone;
        }
        else if (key == "time")
        {
            int *values[] = {&candidate.anchor.year, &candidate.anchor.month, &candidate.anchor.day,
                             &candidate.anchor.hour, &candidate.anchor.minute, &candidate.anchor.second};
            for (int j = 0; j < 6; ++j)
                if (!parse_number(row[j + 1], *values[j]))
                { error = "Saved plan has an invalid date or time number."; return false; }
        }
        else if (key == "minimum_altitude")
        {
            if (!parse_number(row[1], candidate.min_altitude))
            { error = "Saved plan has an invalid minimum altitude."; return false; }
        }
        else if (key == "above_horizon_only")
        {
            if (row[1] != "0" && row[1] != "1")
            { error = "Saved plan has an invalid horizon filter."; return false; }
            candidate.above_horizon_only = row[1] == "1";
        }
        else if (key == "target" || key == "selected_target")
        {
            if (key == "selected_target" && row[1].empty()) candidate.selected_target.clear();
            else
            {
                const celestial_object *object = find_target(catalog, row[1]);
                if (!object) { error = "Saved plan contains an unknown target: " + row[1]; return false; }
                if (key == "selected_target") candidate.selected_target = object->name;
                else if (targets.insert(folded(object->name)).second) candidate.targets.push_back(object->name);
            }
        }
        else { error = "Saved plan has an unknown setting: " + key; return false; }
    }
    const std::set<std::string> required_keys = {"city", "time", "minimum_altitude", "above_horizon_only", "selected_target"};
    if (keys != required_keys)
    { error = "Saved plan is missing required settings."; return false; }
    if (!valid_session(candidate, error)) return false;
    session = candidate;
    error.clear();
    return true;
}

bool export_observation_plan_csv(const std::string &path, const observation_session &session,
                                 const Star_map_data &catalog, std::string &error)
{
    error.clear();
    if (!valid_session(session, error)) return false;
    if (session.targets.empty()) { error = "Add at least one target to the observing plan before exporting."; return false; }
    const city_data *city = find_city(catalog, session.anchor.current_city.city_name);
    if (!city) { error = "The observing city is not in the catalog."; return false; }
    observer_data anchor = session.anchor;
    anchor.current_city = *city;
    anchor.timezone = city->timezone;
    if (!valid_observation(anchor)) { error = "The catalog city or observing time is invalid."; return false; }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    csv_row(out, {"target", "type", "city", "anchor_local", "utc_offset_hours", "minimum_altitude_deg",
                  "above_horizon_only", "altitude_at_anchor_deg", "azimuth_at_anchor_deg", "sampled_peak_local",
                  "sampled_peak_altitude_deg", "above_threshold_windows_local", "sampling_minutes",
                  "analysis_duration_hours", "model_note"});
    std::set<std::string> seen;
    for (const std::string &name : session.targets)
    {
        const celestial_object *object = find_target(catalog, name);
        if (!object) { error = "The observing plan contains an unknown target: " + name; return false; }
        if (!seen.insert(folded(object->name)).second) continue;
        const sky_position position = position_at_observation(anchor, *object);
        if (!std::isfinite(position.altitude) || !std::isfinite(position.azimuth))
        { error = "Cannot calculate a geometric position for " + object->name + "."; return false; }
        std::string peak_time, peak_altitude, windows, sampling, duration;
        std::string note = "Time analysis unsupported for this catalog object; current fixed-coordinate geometric position only; not a visibility guarantee.";
        if (supports_time_analysis(*object))
        {
            observer_data end = anchor;
            if (!shift_observation_minutes(end, 1440))
            { error = "The 24-hour analysis extends beyond the supported date range."; return false; }
            const target_analysis analysis = analyze_target(anchor, *object, 1440, 15, session.min_altitude);
            if (analysis.samples.empty() || !std::isfinite(analysis.peak_altitude))
            { error = "Could not sample the target " + object->name + "."; return false; }
            peak_time = local_time(anchor, analysis.peak_minute);
            peak_altitude = number(analysis.peak_altitude);
            for (const altitude_interval &window : analysis.windows)
            {
                if (!windows.empty()) windows += "; ";
                windows += local_time(anchor, window.start_minute) + " / " + local_time(anchor, window.end_minute);
            }
            if (windows.empty()) windows = "None in sampled interval";
            sampling = "15";
            duration = "24";
            note = "Geometric fixed-catalog estimate sampled every 15 min; windows have 15-min resolution; not a visibility guarantee. No weather, daylight, atmospheric extinction or horizon obstructions.";
        }
        csv_row(out, {object->name, object_type(*object), city->city_name, local_time(anchor),
                      number(anchor.timezone), number(session.min_altitude), session.above_horizon_only ? "1" : "0",
                      number(position.altitude), number(position.azimuth), peak_time, peak_altitude,
                      windows, sampling, duration, note});
    }
    return atomic_write(path, out.str(), error);
}
