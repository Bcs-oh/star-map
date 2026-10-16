#include "../observation_plan.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

namespace
{
int checks = 0;
int failures = 0;
std::string fixture_dir;
std::vector<std::string> fixture_files;

void expect(bool passed, const std::string &description)
{
    ++checks;
    if (!passed) { ++failures; std::cerr << "FAIL: " << description << '\n'; }
}

std::string fixture(const std::string &name)
{
    const std::string path = fixture_dir + "/" + name;
    fixture_files.push_back(path);
    return path;
}

void write_fixture(const std::string &path, const std::string &content)
{
    std::ofstream out(path, std::ios::binary);
    out << content;
    out.close();
    expect(!out.fail(), "write isolated fixture");
}

std::string read_fixture(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

bool same_session(const observation_session &a, const observation_session &b)
{
    return a.anchor.current_city.city_name == b.anchor.current_city.city_name &&
        a.anchor.current_city.country == b.anchor.current_city.country &&
        a.anchor.current_city.latitude == b.anchor.current_city.latitude &&
        a.anchor.current_city.longitude == b.anchor.current_city.longitude &&
        a.anchor.current_city.timezone == b.anchor.current_city.timezone &&
        a.anchor.year == b.anchor.year && a.anchor.month == b.anchor.month &&
        a.anchor.day == b.anchor.day && a.anchor.hour == b.anchor.hour &&
        a.anchor.minute == b.anchor.minute && a.anchor.second == b.anchor.second &&
        a.anchor.timezone == b.anchor.timezone && a.min_altitude == b.min_altitude &&
        a.above_horizon_only == b.above_horizon_only && a.selected_target == b.selected_target &&
        a.targets == b.targets;
}

std::string replace_once(std::string text, const std::string &from, const std::string &to)
{
    const std::size_t found = text.find(from);
    if (found == std::string::npos) { expect(false, "test mutation exists"); return text; }
    text.replace(found, from.size(), to);
    return text;
}

void reject_load(const std::string &path, const std::string &content, const Star_map_data &catalog,
                 const observation_session &sentinel, const std::string &description)
{
    write_fixture(path, content);
    observation_session loaded = sentinel;
    std::string error;
    expect(!load_observation_plan(path, catalog, loaded, error), description + " is rejected");
    expect(!error.empty(), description + " reports a reason");
    expect(same_session(loaded, sentinel), description + " leaves all caller fields unchanged");
}
}

int main()
{
#ifdef _WIN32
    const char *temp = std::getenv("TEMP");
    if (!temp) { std::cerr << "TEMP is unavailable.\n"; return 1; }
    for (int i = 0; i < 100; ++i)
    {
        fixture_dir = std::string(temp) + "/star-map-plan-test-" + std::to_string(std::rand()) + "-" + std::to_string(i);
        if (_mkdir(fixture_dir.c_str()) == 0) break;
        fixture_dir.clear();
    }
#else
    char pattern[] = "/tmp/star-map-plan-test-XXXXXX";
    char *created = mkdtemp(pattern);
    if (created) fixture_dir = created;
#endif
    if (fixture_dir.empty()) { std::cerr << "Cannot create isolated test fixtures.\n"; return 1; }

    Star_map_data catalog;
    const city_data city = {"Kuala Lumpur", 3.139, 101.6869, "Malaysia", 8};
    const city_data quoted_city = {"Port, \"Example\"\nNorth", 27.7172, 85.324, "Example country", 5.75};
    catalog.cities.add(city);
    catalog.cities.add(quoted_city);
    catalog.celestial_objects.add(new Deep_sky_object("M42, \"Orion\"", 5.59, -5.39, 4, COLOR_WHITE, "Nebula, emission"));
    catalog.celestial_objects.add(new Planet("Mars", 3.5, 13.2, 1, COLOR_RED));
    catalog.celestial_objects.add(new Deep_sky_object("Andromeda", 0.712, 41.269, 3.4, COLOR_WHITE, "Galaxy"));

    observation_session original;
    original.anchor = {city, 2026, 10, 16, 20, 15, 45, 8};
    original.min_altitude = 32.25;
    original.above_horizon_only = false;
    original.selected_target = "Andromeda";
    original.targets = {"Andromeda", "Mars"};
    const std::string plan_path = fixture("plan.txt");
    const std::string bad_path = fixture("bad.txt");
    const std::string csv_path = fixture("plan.csv");
    std::string error = "stale error";
    expect(save_observation_plan(plan_path, original, error), "save valid plan");
    expect(error.empty(), "successful save clears previous error");
    const std::string baseline = read_fixture(plan_path);
    observation_session loaded;
    expect(load_observation_plan(plan_path, catalog, loaded, error), "load valid plan");
    expect(error.empty() && same_session(original, loaded), "round trip preserves all session fields");

    observation_session quoted = original;
    quoted.anchor.current_city = quoted_city;
    quoted.anchor.timezone = quoted_city.timezone;
    quoted.selected_target = "M42, \"Orion\"";
    quoted.targets = {"M42, \"Orion\"", "Mars", "m42, \"orion\""};
    expect(save_observation_plan(plan_path, quoted, error), "save CSV commas, quotes and embedded newline");
    const std::string escaped = read_fixture(plan_path);
    expect(escaped.find("\"Port, \"\"Example\"\"\nNorth\"") != std::string::npos, "city name is quoted and quotes escaped");
    expect(escaped.find("\"M42, \"\"Orion\"\"\"") != std::string::npos, "target name is quoted and quotes escaped");
    expect(load_observation_plan(plan_path, catalog, loaded, error), "load quoted CSV fields");
    quoted.targets.pop_back();
    expect(same_session(quoted, loaded), "quoted round trip and case-insensitive target deduplication");
    expect(loaded.anchor.current_city.latitude == quoted_city.latitude && loaded.anchor.timezone == 5.75,
           "load retains catalog coordinates and fractional UTC offset");

    write_fixture(plan_path, replace_once(baseline, "city,Kuala Lumpur", "city,kUaLa lUmPuR") + "target,andromeda\n");
    expect(load_observation_plan(plan_path, catalog, loaded, error), "case-insensitive exact catalog names are accepted");
    expect(same_session(original, loaded), "names are canonicalized and duplicate target rows deduplicated");
    std::string crlf;
    for (char c : baseline) crlf += c == '\n' ? "\r\n" : std::string(1, c);
    write_fixture(plan_path, crlf);
    expect(load_observation_plan(plan_path, catalog, loaded, error), "CRLF plan loads");
    write_fixture(plan_path, baseline.substr(0, baseline.size() - 1));
    expect(load_observation_plan(plan_path, catalog, loaded, error), "final record need not end with a newline");

    observation_session sentinel = quoted;
    sentinel.min_altitude = 61;
    const std::vector<std::pair<std::string, std::string>> corrupt = {
        {"empty file", ""},
        {"unsupported version", replace_once(baseline, "STAR_MAP_PLAN,1", "STAR_MAP_PLAN,2")},
        {"unknown city", replace_once(baseline, "Kuala Lumpur", "Atlantis")},
        {"partial city name", replace_once(baseline, "Kuala Lumpur", "Kuala")},
        {"unknown list target", replace_once(baseline, "target,Mars", "target,Unknown planet")},
        {"unknown selected target", replace_once(baseline, "selected_target,Andromeda", "selected_target,Unknown galaxy")},
        {"missing setting", replace_once(baseline, "selected_target,Andromeda\n", "")},
        {"duplicate setting", baseline + "minimum_altitude,30\n"},
        {"unknown setting", baseline + "latitude,90\n"},
        {"extra fields", replace_once(baseline, "city,Kuala Lumpur", "city,Kuala Lumpur,unexpected")},
        {"empty target", baseline + "target,\n"},
        {"invalid leap date", replace_once(baseline, "time,2026,10,16,20,15,45", "time,2026,2,29,20,15,45")},
        {"hour outside range", replace_once(baseline, "time,2026,10,16,20,15,45", "time,2026,10,16,24,15,45")},
        {"integer overflow", replace_once(baseline, "time,2026", "time,9999999999999999999999")},
        {"number suffix", replace_once(baseline, "minimum_altitude,32.25", "minimum_altitude,32.25abc")},
        {"numeric whitespace", replace_once(baseline, "minimum_altitude,32.25", "minimum_altitude, 32.25")},
        {"negative threshold", replace_once(baseline, "minimum_altitude,32.25", "minimum_altitude,-1")},
        {"threshold above zenith", replace_once(baseline, "minimum_altitude,32.25", "minimum_altitude,90.01")},
        {"NaN threshold", replace_once(baseline, "minimum_altitude,32.25", "minimum_altitude,nan")},
        {"infinite threshold", replace_once(baseline, "minimum_altitude,32.25", "minimum_altitude,1e999")},
        {"invalid Boolean", replace_once(baseline, "above_horizon_only,0", "above_horizon_only,true")},
        {"unterminated quote", baseline + "target,\"Andromeda\n"},
        {"unexpected quote", baseline + "target,Andro\"meda\n"},
        {"text after quote", baseline + "target,\"Andromeda\"oops\n"},
        {"NUL byte", baseline + std::string("target,A\0B\n", 11)},
        {"oversized field", baseline + "target," + std::string(4097, 'A') + "\n"},
        {"oversized file", std::string(1024 * 1024 + 1, 'A')}
    };
    for (const auto &test : corrupt) reject_load(bad_path, test.second, catalog, sentinel, test.first);
    loaded = sentinel;
    expect(!load_observation_plan(fixture_dir + "/missing.txt", catalog, loaded, error), "missing file rejected");
    expect(!error.empty() && same_session(loaded, sentinel), "missing file keeps caller state and reports error");

    expect(save_observation_plan(plan_path, original, error), "atomic replace of an existing valid plan");
    expect(read_fixture(plan_path) == baseline, "replacement contains the complete new plan");
    observation_session invalid = original;
    invalid.min_altitude = std::numeric_limits<double>::quiet_NaN();
    expect(!save_observation_plan(plan_path, invalid, error), "invalid session cannot overwrite saved plan");
    expect(!error.empty() && read_fixture(plan_path) == baseline, "failed save preserves existing file");
    invalid = original;
    invalid.anchor.timezone = 7;
    expect(!save_observation_plan(plan_path, invalid, error), "inconsistent city and observer timezone rejected");
    expect(!save_observation_plan(fixture_dir + "/missing-parent/plan.txt", original, error), "save fails cleanly in missing parent");
    expect(!save_observation_plan(fixture_dir, original, error), "cannot atomically replace a directory");
    struct stat info;
    expect(stat(fixture_dir.c_str(), &info) == 0 && (info.st_mode & S_IFDIR), "failed rename preserves destination directory");

    observation_session no_selection = original;
    no_selection.selected_target.clear();
    no_selection.targets.clear();
    expect(save_observation_plan(plan_path, no_selection, error) && load_observation_plan(plan_path, catalog, loaded, error),
           "empty target list and selected target save and load");
    expect(same_session(loaded, no_selection), "empty list session round trips");

    expect(export_observation_plan_csv(csv_path, quoted, catalog, error), "export observing CSV");
    const std::string exported = read_fixture(csv_path);
    expect(error.empty() && exported.find("sampled_peak_local,sampled_peak_altitude_deg,above_threshold_windows_local") != std::string::npos,
           "export includes sampled peak and threshold window columns");
    expect(exported.find("\"M42, \"\"Orion\"\"\",\"Nebula, emission\"") != std::string::npos,
           "export uses and quotes real catalog name and type");
    expect(exported.find(",15,24,") != std::string::npos && exported.find("not a visibility guarantee") != std::string::npos,
           "export states 15-minute geometric sampling and practical limitation");
    expect(exported.find("Mars,Planet,") != std::string::npos && exported.find("Time analysis unsupported") != std::string::npos,
           "moving target exported with actual type and unsupported time analysis label");
    expect(exported.find("2026-10-16 20:15:45") != std::string::npos, "export includes the fixed local anchor with seconds");
    invalid = quoted;
    invalid.targets.push_back("Missing object");
    expect(!export_observation_plan_csv(csv_path, invalid, catalog, error), "unknown export target rejected");
    expect(!error.empty() && read_fixture(csv_path) == exported, "failed export preserves the existing CSV");
    expect(!export_observation_plan_csv(csv_path, no_selection, catalog, error), "empty observing list reports useful export error");
    invalid = original;
    invalid.anchor.year = 9999; invalid.anchor.month = 12; invalid.anchor.day = 31;
    expect(!export_observation_plan_csv(csv_path, invalid, catalog, error), "export rejects a truncated 24-hour date range");
    expect(read_fixture(csv_path) == exported, "date-range export failure preserves previous CSV");

    for (const std::string &path : fixture_files) std::remove(path.c_str());
#ifdef _WIN32
    const int removed = _rmdir(fixture_dir.c_str());
#else
    const int removed = rmdir(fixture_dir.c_str());
#endif
    expect(removed == 0, "all own fixtures and temporary output files cleaned");
    std::cout << checks << " plan checks, " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
