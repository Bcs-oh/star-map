#include "gui.h"
#include "observation_analysis.h"
#include "observation_plan.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

int process_date(int month, int year)
{
    if (month == 2) return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0 ? 29 : 28;
    return month == 4 || month == 6 || month == 9 || month == 11 ? 30 : 31;
}

bool check_timeboxs(const GUI_State &state)
{
    if (state.input_year.length() > 4 || state.input_month.length() > 2 ||
        state.input_day.length() > 2 || state.input_hour.length() > 2 ||
        state.input_minute.length() > 2 || state.input_second.length() > 2) return false;
    if (!is_integer(state.input_year) || !is_integer(state.input_month) ||
        !is_integer(state.input_day) || !is_integer(state.input_hour) ||
        !is_integer(state.input_minute) || !is_integer(state.input_second)) return false;
    int year = to_integer(state.input_year), month = to_integer(state.input_month);
    int day = to_integer(state.input_day), hour = to_integer(state.input_hour);
    int minute = to_integer(state.input_minute), second = to_integer(state.input_second);
    return year >= 1 && year <= 9999 && month >= 1 && month <= 12 &&
           day >= 1 && day <= process_date(month, year) && hour >= 0 && hour <= 23 &&
           minute >= 0 && minute <= 59 && second >= 0 && second <= 59;
}

bool shift_observation_hour(observer_data &observer, int change)
{
    return (change == -1 || change == 1) && shift_observation_minutes(observer, change * 60);
}

string format_utc_offset(double timezone)
{
    int minutes = static_cast<int>(std::round(std::abs(timezone) * 60));
    string label = "(UTC" + string(timezone < 0 ? "-" : "+") + to_string(minutes / 60);
    if (minutes % 60 != 0) label += ":" + string(minutes % 60 < 10 ? "0" : "") + to_string(minutes % 60);
    return label + ")";
}

double object_marker_radius(double magnitude) { return std::max(1.0, 5.0 - magnitude / 2.0); }

namespace
{
const color BACKGROUND = rgb_color(11, 16, 25), PANEL = rgb_color(18, 25, 37);
const color BORDER = rgb_color(40, 53, 72), INK = rgb_color(225, 233, 244);
const color MUTED = rgb_color(142, 159, 182), ACCENT = rgb_color(106, 193, 238);
const color GREEN = rgb_color(133, 211, 178), AMBER = rgb_color(237, 190, 113);
const double MAP_X = 604, MAP_Y = 413, MAP_RADIUS = 183;
const int PAGE_SIZE = 11;
const string PLAN_PATH = ".star-map-plan.txt", EXPORT_PATH = "observing-plan.csv";

struct workbench_state
{
    GUI_State fields;
    observation_session session;
    target_analysis analysis;
    string query, category = "All", status = "Choose a target, explore its path, build an observing plan.";
    string weather_status = "Weather: not requested";
    int catalog_page = 0, plan_page = 0, offset = 0, maximum_offset = 1440;
    bool plan_tab = false, path = true, playing = false, dirty = true;
    unsigned int last_tick = 0;
};

string lower(string value)
{
    for (char &ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return value;
}

string decimal(double value, int precision = 1)
{
    if (!std::isfinite(value)) return "--";
    std::ostringstream out; out << std::fixed << std::setprecision(precision) << value; return out.str();
}

string padded(int value, int width = 2)
{
    std::ostringstream out; out << std::setw(width) << std::setfill('0') << value; return out.str();
}

string clock_label(const observer_data &o) { return padded(o.hour) + ":" + padded(o.minute); }
string date_label(const observer_data &o) { return padded(o.year, 4) + "-" + padded(o.month) + "-" + padded(o.day); }
string at_minute(const observer_data &anchor, int minute)
{
    observer_data at = anchor;
    if (!shift_observation_minutes(at, minute)) return "--";
    return clock_label(at) + (at.day != anchor.day || at.month != anchor.month || at.year != anchor.year ? " +1d" : "");
}

void text(const string &label, double x, double y, int size = 14, color ink = INK)
{ draw_text(label, ink, "my_font", size, x, y); }

string fit(const string &label, int size, double width)
{
    string result = label;
    while (!result.empty() && text_width(result + "...", "my_font", size) > width) result.pop_back();
    return result == label ? label : result + "...";
}

void card(double x, double y, double width, double height)
{ fill_rectangle(PANEL, x, y, width, height); draw_rectangle(BORDER, x, y, width, height); }

bool inside(double x, double y, double width, double height)
{ return mouse_x() >= x && mouse_x() <= x + width && mouse_y() >= y && mouse_y() <= y + height; }

bool action(const string &label, double x, double y, double width, bool active = false, bool enabled = true)
{
    bool hover = enabled && inside(x, y, width, 29);
    fill_rectangle(active ? rgb_color(31, 61, 82) : hover ? rgb_color(39, 54, 74) : rgb_color(25, 35, 50), x, y, width, 29);
    draw_rectangle(active ? rgb_color(70, 128, 160) : BORDER, x, y, width, 29);
    color ink = !enabled ? rgb_color(86, 102, 125) : active ? ACCENT : INK;
    text(label, x + (width - text_width(label, "my_font", 12)) / 2, y + 7, 12, ink);
    return hover && mouse_clicked(LEFT_BUTTON);
}

string object_type(const celestial_object &object)
{
    const auto *deep = dynamic_cast<const Deep_sky_object *>(&object);
    if (deep) return deep->object_type;
    if (dynamic_cast<const Planet *>(&object)) return "Planet";
    if (dynamic_cast<const Satellite *>(&object)) return "Satellite";
    return "Star";
}

bool matches_category(const celestial_object &object, const string &category)
{
    string kind = lower(object_type(object));
    if (category == "All") return true;
    if (category == "Galaxies") return kind.find("galaxy") != string::npos;
    if (category == "Nebulae") return kind.find("nebula") != string::npos || kind.find("remnant") != string::npos;
    if (category == "Clusters") return kind.find("cluster") != string::npos;
    if (category == "Solar") return !supports_time_analysis(object);
    return supports_time_analysis(object) && kind.find("galaxy") == string::npos &&
           kind.find("nebula") == string::npos && kind.find("remnant") == string::npos && kind.find("cluster") == string::npos;
}

celestial_object *find_target(Star_map_data &map, const string &name)
{
    for (int i = 0; i < map.celestial_objects.length(); ++i)
        if (map.celestial_objects[i]->name == name) return map.celestial_objects[i];
    return nullptr;
}

bool find_city(const Star_map_data &map, const string &name, city_data &result)
{
    for (int i = 0; i < map.cities.length(); ++i)
        if (lower(map.cities[i].city_name) == lower(name)) { result = map.cities[i]; return true; }
    return false;
}

void sync_fields(workbench_state &state, const observer_data &o)
{
    state.fields.input_city = o.current_city.city_name;
    state.fields.input_year = padded(o.year, 4); state.fields.input_month = padded(o.month);
    state.fields.input_day = padded(o.day); state.fields.input_hour = padded(o.hour);
    state.fields.input_minute = padded(o.minute); state.fields.input_second = padded(o.second);
}

void reset_session(workbench_state &state, Star_map_data &map, const observer_data &anchor)
{
    state.session.anchor = anchor; state.maximum_offset = 1440;
    observer_data end = anchor;
    if (!shift_observation_minutes(end, 1440))
    {
        int low = 0, high = 1440;
        while (low < high)
        {
            int middle = (low + high + 1) / 2; end = anchor;
            if (shift_observation_minutes(end, middle)) low = middle; else high = middle - 1;
        }
        state.maximum_offset = low;
    }
    state.offset = 0; state.playing = false; state.dirty = true;
    state.weather_status = "Weather: not requested";
    map.current_weather.forecast_list.clear();
    map.current_weather.condition = "Unavailable";
    map.observer = anchor; sync_fields(state, anchor);
}

void set_offset(workbench_state &state, Star_map_data &map, int offset)
{
    offset = std::clamp(offset, 0, state.maximum_offset);
    observer_data current = state.session.anchor;
    if (shift_observation_minutes(current, offset))
    { state.offset = offset; map.observer = current; sync_fields(state, current); }
    if (offset == state.maximum_offset) state.playing = false;
}

observer_data current_time(city_data city)
{
    observer_data o{}; o.current_city = city; o.timezone = city.timezone;
    std::time_t now = std::time(nullptr) + static_cast<long>(std::round(city.timezone * 3600));
    const std::tm *local = std::gmtime(&now);
    o.year = local->tm_year + 1900; o.month = local->tm_mon + 1; o.day = local->tm_mday;
    o.hour = local->tm_hour; o.minute = local->tm_min; o.second = 0; return o;
}

void select(workbench_state &state, celestial_object *&target, celestial_object *next)
{
    if (!next) return;
    target = next; state.session.selected_target = next->name; state.dirty = true;
}

void refresh_analysis(workbench_state &state, const celestial_object *target)
{
    if (!state.dirty) return;
    state.analysis = target ? analyze_target(state.session.anchor, *target, 1440, 15, state.session.min_altitude) : target_analysis{};
    state.dirty = false;
}

bool planned(const workbench_state &state, const string &name)
{ return std::find(state.session.targets.begin(), state.session.targets.end(), name) != state.session.targets.end(); }

void projection(const sky_position &position, double &x, double &y)
{
    double radians = position.azimuth * 3.141592653589793 / 180;
    double radius = MAP_RADIUS * (90 - position.altitude) / 90;
    x = MAP_X + radius * std::sin(radians); y = MAP_Y - radius * std::cos(radians);
}

void update_positions(Star_map_data &map)
{
    for (int i = 0; i < map.celestial_objects.length(); ++i)
    {
        auto *object = map.celestial_objects[i];
        sky_position p = position_at_observation(map.observer, *object);
        object->altitude = p.altitude; object->azimuth = p.azimuth; map.checkvisibility(*object);
    }
}

void draw_map(Star_map_data &map, workbench_state &state, celestial_object *&target)
{
    card(246, 170, 716, 465);
    text("SKY VIEW", 264, 187, 13, MUTED); text(clock_label(map.observer) + " local", 350, 183, 20);
    if (action(state.path ? "Path: on" : "Path: off", 848, 181, 96, state.path)) state.path = !state.path;
    for (int altitude : {0, 30, 60})
    {
        double radius = MAP_RADIUS * (90 - altitude) / 90;
        draw_circle(BORDER, MAP_X, MAP_Y, radius);
        text(to_string(altitude) + " deg", MAP_X + 7, MAP_Y - radius + 5, 11, MUTED);
    }
    draw_line(BORDER, MAP_X - MAP_RADIUS, MAP_Y, MAP_X + MAP_RADIUS, MAP_Y);
    draw_line(BORDER, MAP_X, MAP_Y - MAP_RADIUS, MAP_X, MAP_Y + MAP_RADIUS);
    text("N", MAP_X - 5, MAP_Y - MAP_RADIUS - 19, 13, ACCENT);
    text("E", MAP_X + MAP_RADIUS + 12, MAP_Y - 7, 13, MUTED);
    text("S", MAP_X - 5, MAP_Y + MAP_RADIUS + 6, 13, MUTED);
    text("W", MAP_X - MAP_RADIUS - 22, MAP_Y - 7, 13, MUTED);
    if (state.path && target && supports_time_analysis(*target))
        for (std::size_t i = 1; i < state.analysis.samples.size(); ++i)
        {
            const auto &a = state.analysis.samples[i - 1].position, &b = state.analysis.samples[i].position;
            if (a.altitude < 0 || b.altitude < 0) continue;
            double ax, ay, bx, by; projection(a, ax, ay); projection(b, bx, by);
            draw_line(rgb_color(54, 110, 139), ax, ay, bx, by);
        }
    double nearest = 12; celestial_object *clicked = nullptr;
    for (int i = 0; i < map.celestial_objects.length(); ++i)
    {
        auto *object = map.celestial_objects[i];
        if (!std::isfinite(object->altitude) || object->altitude < 0) continue;
        double x, y; projection({object->altitude, object->azimuth}, x, y);
        double radius = std::clamp(object_marker_radius(object->apparent_magnitude), 2.0, 5.5);
        object->draw_on_map(x, y, radius, object == target ? ACCENT : rgb_color(125, 153, 179), false);
        if (planned(state, object->name)) draw_circle(GREEN, x, y, radius + 4);
        if (object == target) { draw_circle(ACCENT, x, y, radius + 8); text(object->name, x + 12, y - 10, 15, ACCENT); }
        if (mouse_clicked(LEFT_BUTTON))
        {
            double distance = std::hypot(mouse_x() - x, mouse_y() - y);
            if (distance < nearest) { nearest = distance; clicked = object; }
        }
    }
    if (clicked) select(state, target, clicked);
    text("Above-horizon hemisphere / triangles: deep sky / green rings: planned", 264, 614, 11, MUTED);
}

void draw_catalog(Star_map_data &map, workbench_state &state, celestial_object *&target)
{
    card(16, 170, 214, 685); text("TARGET CATALOG", 30, 188, 13, MUTED);
    string query = text_box(state.query, rectangle_from(30, 214, 186, 29));
    if (query != state.query) { state.query = query; state.catalog_page = 0; }
    text("Search name or object type", 30, 248, 11, MUTED);
    const string categories[] = {"All", "Galaxies", "Nebulae", "Clusters", "Solar", "Other"};
    for (int i = 0; i < 6; ++i)
        if (action(categories[i], 30 + (i % 3) * 63, 271 + (i / 3) * 34, 60, state.category == categories[i]))
        { state.category = categories[i]; state.catalog_page = 0; }
    if (action(state.session.above_horizon_only ? "Above horizon: on" : "Above horizon: off", 30, 343, 186, state.session.above_horizon_only))
    { state.session.above_horizon_only = !state.session.above_horizon_only; state.catalog_page = 0; }
    std::vector<celestial_object *> results; string search = lower(state.query);
    for (int i = 0; i < map.celestial_objects.length(); ++i)
    {
        auto *object = map.celestial_objects[i];
        if (matches_category(*object, state.category) &&
            (search.empty() || lower(object->name + " " + object_type(*object)).find(search) != string::npos) &&
            (!state.session.above_horizon_only || object->altitude >= 0)) results.push_back(object);
    }
    int pages = std::max(1, (static_cast<int>(results.size()) + PAGE_SIZE - 1) / PAGE_SIZE);
    state.catalog_page = std::clamp(state.catalog_page, 0, pages - 1);
    if (inside(16, 379, 214, 426))
    {
        double scroll = mouse_wheel_scroll().y;
        if (scroll != 0) state.catalog_page = std::clamp(state.catalog_page + (scroll < 0 ? 1 : -1), 0, pages - 1);
    }
    text(std::to_string(results.size()) + " matches / " + to_string(map.celestial_objects.length()) + " objects", 30, 385, 11, MUTED);
    for (int row = 0; row < PAGE_SIZE; ++row)
    {
        int index = state.catalog_page * PAGE_SIZE + row; if (index >= static_cast<int>(results.size())) break;
        auto *object = results[index]; double y = 411 + row * 35;
        if (object == target) fill_rectangle(rgb_color(30, 54, 74), 24, y - 3, 198, 34);
        if (inside(24, y - 3, 198, 34) && mouse_clicked(LEFT_BUTTON)) select(state, target, object);
        text(object->name + (planned(state, object->name) ? "  +" : ""), 32, y, 14, object == target ? ACCENT : INK);
        text(fit(object_type(*object), 10, 121), 32, y + 17, 10, MUTED);
        text(decimal(object->altitude, 0) + " deg", 169, y + 6, 11, object->altitude >= 0 ? GREEN : MUTED);
    }
    if (results.empty()) text("No matching targets.", 30, 425, 13, MUTED);
    if (action("Prev", 30, 812, 54, false, state.catalog_page > 0)) --state.catalog_page;
    text(to_string(state.catalog_page + 1) + " / " + to_string(pages), 102, 819, 12, MUTED);
    if (action("Next", 162, 812, 54, false, state.catalog_page + 1 < pages)) ++state.catalog_page;
}

void draw_curve(Star_map_data &map, workbench_state &state, celestial_object *target)
{
    card(246, 650, 716, 205); text("ALTITUDE OVER TIME", 264, 665, 13, MUTED);
    text(target ? target->name : "Choose a target", 430, 662, 18, ACCENT);
    if (action(state.playing ? "Pause" : "Play", 864, 658, 80, state.playing, state.maximum_offset > 0))
    { state.playing = !state.playing; state.last_tick = current_ticks(); }
    const double x = 291, y = 701, width = 639, height = 89;
    auto curve_y = [=](double altitude) { return y + (90 - altitude) / 180 * height; };
    for (int altitude : {-90, 0, 90})
    {
        draw_line(BORDER, x, curve_y(altitude), x + width, curve_y(altitude));
        text(to_string(altitude), 263, curve_y(altitude) - 6, 10, MUTED);
    }
    draw_line(rgb_color(99, 84, 59), x, curve_y(state.session.min_altitude), x + width, curve_y(state.session.min_altitude));
    for (const auto &window : state.analysis.windows)
        fill_rectangle(rgba_color(74, 147, 117, 28), x + width * window.start_minute / 1440, y,
                       width * (window.end_minute - window.start_minute) / 1440, height);
    for (std::size_t i = 1; i < state.analysis.samples.size(); ++i)
    {
        const auto &a = state.analysis.samples[i - 1], &b = state.analysis.samples[i];
        draw_line(ACCENT, x + width * a.minute_offset / 1440, curve_y(a.position.altitude),
                  x + width * b.minute_offset / 1440, curve_y(b.position.altitude));
    }
    if (state.analysis.samples.empty()) text("Time analysis is available for deep-sky targets only.", 366, 731, 13, MUTED);
    double current_x = x + width * state.offset / 1440; draw_line(INK, current_x, y, current_x, y + height);
    if (target && supports_time_analysis(*target)) fill_circle(ACCENT, current_x, curve_y(target->altitude), 3.5);
    if (inside(x, y, width, height) && mouse_down(LEFT_BUTTON))
        set_offset(state, map, static_cast<int>(std::round((mouse_x() - x) / width * 96)) * 15);
    for (int hour = 0; hour <= 24; hour += 6) text("+" + to_string(hour) + "h", x + width * hour / 24 - 8, 795, 10, MUTED);
    fill_rectangle(BORDER, 291, 822, 639, 3);
    fill_rectangle(ACCENT, 291, 822, 639.0 * state.offset / 1440, 3);
    fill_circle(ACCENT, 291 + 639.0 * state.offset / 1440, 823, 5);
    if (inside(285, 814, 651, 20) && mouse_down(LEFT_BUTTON))
        set_offset(state, map, static_cast<int>(std::round((mouse_x() - 291) / 639 * 96)) * 15);
    text("Anchor " + date_label(state.session.anchor) + " " + clock_label(state.session.anchor) + " / 15 min samples / geometric altitude", 264, 839, 10, MUTED);
}

void draw_inspector(Star_map_data &map, workbench_state &state, celestial_object *&target)
{
    card(978, 170, 286, 685);
    if (action("Target", 994, 184, 119, !state.plan_tab)) state.plan_tab = false;
    if (action("Plan (" + std::to_string(state.session.targets.size()) + ")", 1121, 184, 126, state.plan_tab)) state.plan_tab = true;
    if (!state.plan_tab)
    {
        text(target ? target->name : "No target selected", 994, 231, 29, ACCENT);
        if (target)
        {
            text(object_type(*target), 995, 269, 14, MUTED);
            text("ALTITUDE", 995, 308, 11, MUTED); text("AZIMUTH", 1128, 308, 11, MUTED);
            text(decimal(target->altitude) + " deg", 995, 326, 23); text(decimal(target->azimuth) + " deg", 1128, 326, 23);
            text(target->altitude < 0 ? "Below the horizon" : "Above the horizon", 995, 365, 13, target->altitude < 0 ? AMBER : GREEN);
            text("Magnitude " + decimal(target->apparent_magnitude) + " / RA " + decimal(target->right_ascension, 2) + " h", 995, 390, 12, MUTED);
            text("Declination " + decimal(target->declination) + " deg", 995, 410, 12, MUTED);
            draw_line(BORDER, 994, 439, 1248, 439); text("24H ALTITUDE ANALYSIS", 995, 457, 12, MUTED);
            if (!state.analysis.samples.empty())
            {
                text("Peak " + decimal(state.analysis.peak_altitude) + " deg at " + at_minute(state.session.anchor, state.analysis.peak_minute), 995, 482, 14);
                text("Intervals above " + decimal(state.session.min_altitude, 0) + " deg", 995, 515, 12, MUTED);
                if (state.analysis.windows.empty()) text("No sampled interval in this session.", 995, 541, 12, AMBER);
                for (std::size_t i = 0; i < state.analysis.windows.size() && i < 3; ++i)
                {
                    const auto &window = state.analysis.windows[i];
                    text(at_minute(state.session.anchor, window.start_minute) + " - " + at_minute(state.session.anchor, window.end_minute), 995, 541 + i * 22, 13, GREEN);
                }
            }
            else
            {
                text("Fixed-coordinate catalog sample.", 995, 485, 12, AMBER);
                text("Solar System motion is not modeled.", 995, 508, 12, MUTED);
                text("Predictive time analysis unavailable.", 995, 531, 12, MUTED);
            }
            if (action(planned(state, target->name) ? "Remove from plan" : "+ Add to observing plan", 994, 608, 254, planned(state, target->name)))
            {
                auto &targets = state.session.targets;
                if (planned(state, target->name)) targets.erase(std::remove(targets.begin(), targets.end(), target->name), targets.end());
                else targets.push_back(target->name);
                state.status = target->name + (planned(state, target->name) ? " added to your observing plan." : " removed from your observing plan.");
            }
        }
    }
    else
    {
        text("Observing plan", 994, 231, 25); text("Select a row to inspect / x to remove", 995, 268, 12, MUTED);
        int pages = std::max(1, (static_cast<int>(state.session.targets.size()) + 6) / 7);
        state.plan_page = std::clamp(state.plan_page, 0, pages - 1);
        for (int row = 0; row < 7; ++row)
        {
            int index = state.plan_page * 7 + row; if (index >= static_cast<int>(state.session.targets.size())) break;
            const string name = state.session.targets[index]; auto *object = find_target(map, name); double y = 300 + row * 39;
            if (target && name == target->name) fill_rectangle(rgb_color(30, 54, 74), 990, y - 3, 258, 35);
            if (inside(994, y, 207, 33) && mouse_clicked(LEFT_BUTTON)) select(state, target, object);
            text(to_string(index + 1) + ". " + name, 997, y + 2, 15);
            text(object ? fit(object_type(*object), 10, 174) : "Unknown", 1013, y + 21, 10, MUTED);
            if (action("x", 1218, y, 29)) { state.session.targets.erase(state.session.targets.begin() + index); break; }
        }
        if (state.session.targets.empty())
        { text("Your plan is empty.", 995, 310, 15, MUTED); text("Add targets from the Target tab.", 995, 340, 12, MUTED); }
        if (action("Prev", 994, 580, 62, false, state.plan_page > 0)) --state.plan_page;
        text(to_string(state.plan_page + 1) + " / " + to_string(pages), 1092, 587, 12, MUTED);
        if (action("Next", 1186, 580, 62, false, state.plan_page + 1 < pages)) ++state.plan_page;
        string error;
        if (action("Save", 994, 621, 76)) state.status = save_observation_plan(PLAN_PATH, state.session, error) ? "Plan saved to " + PLAN_PATH : "Save failed: " + error;
        if (action("Load", 1081, 621, 76))
        {
            observation_session loaded;
            if (load_observation_plan(PLAN_PATH, map, loaded, error))
            {
                state.session = loaded; reset_session(state, map, loaded.anchor);
                target = find_target(map, loaded.selected_target); state.status = "Saved observing session loaded.";
            }
            else state.status = "Load failed: " + error;
        }
        if (action("Export", 1168, 621, 80)) state.status = export_observation_plan_csv(EXPORT_PATH, state.session, map, error) ? "Exported observing-plan.csv in the project folder." : "Export failed: " + error;
    }
    draw_line(BORDER, 994, 674, 1248, 674); text("MINIMUM ALTITUDE", 995, 691, 11, MUTED);
    for (int i = 0; i < 3; ++i)
    {
        int threshold = 30 + i * 15;
        if (action(to_string(threshold) + " deg", 994 + i * 86, 715, 80, state.session.min_altitude == threshold))
        { state.session.min_altitude = threshold; state.dirty = true; }
    }
    text("Geometric planning aid only.", 995, 768, 12, MUTED);
    text("Daylight, clouds and moonlight are", 995, 789, 11, MUTED);
    text("not included in altitude intervals.", 995, 807, 11, MUTED);
    text("Catalog UTC offsets are fixed (no DST).", 995, 831, 10, MUTED);
}
}

void start_gui(Star_map_data &star_map, celestial_object *target)
{
    open_window("Star Map | Observing Workbench", 1280, 880);
    load_font("my_font", "LiberationSans-Regular.ttf");
    set_interface_font("my_font"); set_interface_font_size(12); set_interface_border_color(BORDER);
    set_interface_accent_color(ACCENT, 0.9); set_interface_element_color(rgb_color(29, 40, 57), 0.9); set_interface_text_color(INK);
    const bool bgm_available = std::ifstream("resources/sounds/background.mp3").good();
    bool bgm_playing = false; if (bgm_available) load_music("bgm", "background.mp3");
    workbench_state state; city_data city;
    if (star_map.cities.length() == 0 || star_map.celestial_objects.length() == 0)
    { write_line("Star Map: missing cities.csv or star.csv. Run from the project folder."); close_all_windows(); return; }
    if (!find_city(star_map, "Kuala Lumpur", city)) city = star_map.cities[0];
    reset_session(state, star_map, current_time(city));
    select(state, target, target ? target : find_target(star_map, "M31"));
    if (!target) select(state, target, star_map.celestial_objects[0]);
    while (!quit_requested())
    {
        process_events(); if (key_typed(ESCAPE_KEY)) break;
        if (state.playing && current_ticks() - state.last_tick >= 350)
        { set_offset(state, star_map, state.offset + 15); state.last_tick = current_ticks(); }
        clear_screen(BACKGROUND); text("STAR MAP", 24, 17, 25); text("OBSERVING WORKBENCH", 192, 25, 12, MUTED);
        text("Your sky. Your targets. Your next session.", 500, 25, 13, MUTED);
        text("OFFLINE CATALOG / " + to_string(star_map.celestial_objects.length()) + " TARGETS", 1032, 26, 11, ACCENT); card(16, 62, 1248, 93);
        text("CITY", 30, 72, 10, MUTED); text("DATE / YYYY MM DD", 252, 72, 10, MUTED); text("LOCAL TIME / HH MM SS", 439, 72, 10, MUTED);
        text("Apply edits to start a new session", 603, 72, 10, MUTED);
        auto &fields = state.fields;
        fields.input_city = text_box(fields.input_city, rectangle_from(30, 91, 203, 29));
        fields.input_year = text_box(fields.input_year, rectangle_from(252, 91, 62, 29));
        fields.input_month = text_box(fields.input_month, rectangle_from(322, 91, 42, 29));
        fields.input_day = text_box(fields.input_day, rectangle_from(372, 91, 42, 29));
        fields.input_hour = text_box(fields.input_hour, rectangle_from(439, 91, 42, 29));
        fields.input_minute = text_box(fields.input_minute, rectangle_from(489, 91, 42, 29));
        fields.input_second = text_box(fields.input_second, rectangle_from(539, 91, 42, 29));
        bool apply = action("Apply / new 24h", 603, 91, 132, true), now = action("Now", 745, 91, 64);
        if (apply || now)
        {
            city_data found;
            if (!find_city(star_map, fields.input_city, found)) state.status = "City not found. Use a complete catalog city name (for example: Kuala Lumpur).";
            else if (!now && !check_timeboxs(fields)) state.status = "Invalid calendar date or time. Valid years: 1-9999.";
            else
            {
                observer_data anchor = now ? current_time(found) : star_map.observer;
                if (!now)
                {
                    anchor.current_city = found; anchor.timezone = found.timezone;
                    anchor.year = to_integer(fields.input_year); anchor.month = to_integer(fields.input_month); anchor.day = to_integer(fields.input_day);
                    anchor.hour = to_integer(fields.input_hour); anchor.minute = to_integer(fields.input_minute); anchor.second = to_integer(fields.input_second);
                }
                reset_session(state, star_map, anchor);
                state.status = "New 24-hour session anchored at " + date_label(anchor) + " " + clock_label(anchor) + ".";
            }
        }
        if (action("-1h", 819, 91, 54, false, state.offset > 0)) set_offset(state, star_map, state.offset - 60);
        if (action("+1h", 883, 91, 54, false, state.offset < state.maximum_offset)) set_offset(state, star_map, state.offset + 60);
        if (action("Weather", 952, 91, 78))
        {
            star_map.get_weather();
            state.weather_status = star_map.current_weather.forecast_list.empty() ? "Weather: unavailable (optional API key)" : "Next forecast cloud: " + to_string(star_map.current_weather.forecast_list.front().cloud_cover) + "%";
        }
        if (bgm_available && action(bgm_playing ? "Music: on" : "Music: off", 1040, 91, 100, bgm_playing))
        { bgm_playing = !bgm_playing; if (bgm_playing) play_music("bgm", -1); else stop_music(); }
        text(fit(star_map.observer.current_city.city_name + ", " + star_map.observer.current_city.country + " " + format_utc_offset(star_map.observer.timezone), 11, 386), 30, 131, 11, MUTED);
        text("Lat " + decimal(star_map.observer.current_city.latitude, 2) + " / Lon " + decimal(star_map.observer.current_city.longitude, 2), 439, 131, 11, MUTED);
        text(state.weather_status, 820, 131, 11, MUTED);
        update_positions(star_map);
        refresh_analysis(state, target);
        draw_catalog(star_map, state, target); refresh_analysis(state, target);
        draw_map(star_map, state, target); refresh_analysis(state, target);
        draw_curve(star_map, state, target); draw_inspector(star_map, state, target);
        text(fit(state.status, 11, 1230), 24, 863, 11, MUTED); draw_interface(); refresh_screen(60);
    }
    if (bgm_playing) stop_music(); close_all_windows();
}
