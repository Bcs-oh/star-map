# Star Map

A C++ desktop observing workbench built with SplashKit, developed from a FIT1045 High Distinction (HD) final coursework project.

Choose a location and time, search a catalog of celestial objects, inspect a target on the sky map, and build a local observing plan. The workbench adds a 24-hour altitude view to the original coursework's coordinate calculations.

![Star Map desktop observing workbench](docs/images/star-map.png)

Example session with three planned targets; the application starts with an empty plan. [View the observing-plan panel](docs/images/observing-plan.png).

## Features

- **Observation setup:** choose from 212 cities, validate the local date and time, and keep fractional UTC offsets such as Kathmandu's `UTC+5:45`.
- **Searchable catalog:** search 119 objects, filter by object type, and click a result or sky-map marker to select a target.
- **Desktop workspace:** use a dark three-column layout for setup and catalog, the sky map, and selected-target details.
- **Time exploration:** scrub through a fixed 24-hour interval, click the altitude curve, or use Play/Pause to advance the viewed time.
- **Altitude windows:** for deep-sky targets, estimate intervals above a chosen altitude threshold. These are geometric estimates, not predictions of observing conditions.
- **Local plans:** save and load a target list and observation setup, and export estimates to CSV.
- **Optional weather and music:** use your own OpenWeather key or local audio file.

## Build and run

Install the official [SplashKit SDK and SplashKit Manager](https://splashkit.io/installation/) for your operating system. The commands below assume `skm`, `clang++`, and `make` are available in the SplashKit terminal.

Run from the repository root:

```bash
make
make run
```

If `skm` is installed outside your PATH on macOS or Linux:

```bash
make SKM=~/.splashkit/skm
make run SKM=~/.splashkit/skm
```

The application is built in `build/`. Run it from the repository root so it can find `cities.csv`, `star.csv`, and `resources/`.

You can also compile without Make:

```bash
skm clang++ -std=c++17 star_map.cpp gui.cpp utilities.cpp observation_analysis.cpp observation_plan.cpp -o star-map
./star-map
```

On Windows, use the terminal provided by the official SplashKit setup. For the direct compiler command, run `./star-map.exe`. See the [official SKM documentation](https://github.com/splashkit/skm) for SDK setup.

## Use the workbench

1. Choose a city and a valid local date and time, then select **Apply**. City names follow `cities.csv`; for example, `Kuala Lumpur`. **Now** uses the current time at the selected city's catalog UTC offset.
2. Search the catalog, choose All, Galaxies, Nebulae, Clusters, Solar, or Other, and page through matching results. Click a result or a plotted marker to inspect its altitude, azimuth, and catalog information. **Above horizon** filters the catalog; the sky map always shows the hemisphere at altitude `0°` or higher. **Path** toggles the selected target's sampled path.
3. Move the time slider, click the altitude curve, use **-1h / +1h**, or select **Play/Pause** to explore the following 24 hours. The interval stays anchored to the applied observation time. **Apply** or **Now** establishes a new interval.
4. For a deep-sky target, inspect the altitude curve and choose a minimum altitude of **30°**, **45°**, or **60°** to estimate useful windows. A target above the threshold may still be unobservable because of daylight, clouds, light pollution, terrain, or other conditions.
5. Use the target details and plan tabs to **Add** or **Remove** targets. **Save** the local plan, **Load** it in a later session, or **Export** its sampled estimates to CSV.

Dates must exist in the Gregorian calendar, with a year from `1` to `9999`. Hours are `0–23`; minutes and seconds are `0–59`. Invalid input displays a warning and keeps the previous observation. Hour stepping handles leap days and year rollover within those limits. Press **Escape** or close the window to exit.

## Saved files

The workbench writes its files in the directory from which it is launched:

- `.star-map-plan.txt` stores the observation setup and plan in a versioned local format. A failed load keeps the current session intact.
- `observing-plan.csv` contains the plan's exported estimates. Deep-sky estimates normally use 97 samples across a 24-hour interval, including both endpoints, at 15-minute spacing. Peak and window boundaries are sampled times, without interpolation; they are approximate.

These files are local working data and are excluded from Git. Save and Export replace their corresponding local files. Plans load only when you select **Load**; the application does not automatically restore a previous session.

## Regression checks

With SplashKit installed, run from the repository root:

```bash
make test
```

Or use the installed manager explicitly:

```bash
make test SKM=~/.splashkit/skm
```

The Makefile builds separate checks in `build/` for calendar and GUI helpers, altitude analysis, and local plan persistence/export. They exercise calculations and file handling without opening a window or calling the weather service. They do not establish ephemeris accuracy or replace a manual check of the interface.

## Optional weather and music

The workbench runs without an API key. To enable weather, provide your own OpenWeather key in the launching terminal:

```bash
export OPENWEATHER_API_KEY="your-own-key"
make run
```

Select **Weather** to request a forecast. The application reads the environment variable directly; it does not load a `.env` file. If the key is absent or a forecast is unavailable, the interface displays an unavailable state. The header shows cloud cover from the first returned forecast entry, independently of the simulated viewing time. Starting or loading a session clears the previous forecast. A request is synchronous and may briefly pause the interface. Successful authenticated weather retrieval has not been verified for this repository edition.

For background music, place an audio file you are permitted to use at `resources/sounds/background.mp3`, then use the music toggle. Audio files are ignored by Git. The soundtrack used in the original coursework is not included.

## Project structure

```text
star_map.cpp                 Application entry point
star_map.h                   Catalog loading and observation calculations
celestial_objects.h          Celestial object classes and rendering
gui.cpp / gui.h              Desktop workbench and interaction
observation_analysis.cpp/.h  Sampled altitude curves and threshold windows
observation_plan.cpp/.h      Local plan persistence and CSV export
utilities.cpp / .h           Input and utility functions
cities.csv                   212 observation locations
star.csv                     119 celestial objects
tests/                       Regression checks
resources/fonts/             Liberation Sans and its OFL license
docs/                        Original design sketch and repository notes
```

## Scope and provenance

The original project was a FIT1045 HD final coursework submission. This repository develops that project into a desktop workbench while preserving its C++/SplashKit foundation and supplied catalogs. The [original interface sketch](docs/images/design-sketch.jpg) records its design stage. The duplicate consolidated submission source is omitted in favor of the modular implementation.

This is an educational visualization. Catalog right ascension and declination are fixed, including those for the Sun, Moon, and planets. There is no orbital ephemeris calculation, so these entries must not be treated as date-accurate observing targets. Planning estimates are limited to the catalog's deep-sky object class; star, satellite, and planet entries are excluded. Deep-sky altitude windows omit refraction, precession, daylight, and local obstructions. Near the end of year `9999`, the sampled interval stops at the supported calendar limit.

AI assistance was used for GitHub preparation and the later workbench update, including source review, implementation, documentation, and regression checks. This does not describe or infer how the original coursework was authored.

See [repository notes](docs/REPOSITORY_NOTES.md) for the update history and validation scope, and [third-party notices](THIRD_PARTY_NOTICES.md) for dependencies, font attribution, and unresolved catalog licensing. No project-wide open-source license has been selected.
