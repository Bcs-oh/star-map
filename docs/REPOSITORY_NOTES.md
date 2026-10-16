# Repository notes

This repository edition was prepared from the supplied FIT1045 High Distinction (HD) final coursework files in October 2026. The original Drive files were retained unchanged. Later work develops the local repository into a desktop observing workbench.

## Initial GitHub preparation

- Retained the modular C++ implementation and CSV catalogs; omitted the duplicate consolidated source file.
- Removed the embedded OpenWeather credential and read `OPENWEATHER_API_KEY` from the process environment instead.
- Initialized observation state and cloud-cover fields, cleared stale forecasts, skipped requests without a key, and displayed an unavailable state when no forecast is present.
- Replaced the original Arial resource with unmodified Liberation Sans and its SIL Open Font License.
- Used SplashKit's standard `resources/fonts` and `resources/sounds` directories.
- Made background music optional and omitted the original soundtrack and videos.
- Added documentation, third-party notices, Git ignore rules, and the original design sketch.

## Input reliability update

The October 16 reliability update added complete Gregorian calendar and clock validation, including leap-year rules and years `1–9999`. Invalid input keeps the current observation. Hour stepping shares the same rollover logic and stays within the supported year limits.

UTC labels preserve fractional offsets as hours and minutes without changing the numeric offset used in astronomical calculations. Object markers have a one-pixel minimum for both drawing and click selection.

The earlier GUI helper regression suite passed 175 checks with the local SplashKit installation. Earlier visual checks recorded [Kathmandu's quarter-hour timezone](images/fractional-timezone.png) and [rejection of February 30](images/invalid-date.png). Those images show the earlier interface, before the workbench redesign.

## Desktop workbench update

The updated C++/SplashKit interface provides a 1280 × 880 dark three-column workspace with observation setup, a searchable catalog with type filters and paging, the sky map, selected-target details, and a local observing plan. The interface uses English labels. The Above horizon toggle filters the catalog; the map always plots altitude `0°` or higher to preserve its hemisphere projection.

The observation interval has a fixed anchor. The slider, altitude-curve click, hour buttons, and Play/Pause change the viewed time within the following 24 hours; **Apply** and **Now** establish a new interval. **Now** uses the selected city's catalog UTC offset. A supported target's altitude curve normally uses 97 samples, from offset 0 to 1440 minutes at 15-minute spacing. The maximum is the highest sampled altitude. Windows connect consecutive samples at or above the threshold, with sampled boundaries rather than interpolated crossings. Near the end of year `9999`, the interval stops at the last supported local minute. These geometric estimates do not incorporate darkness, clouds, refraction, precession, or local obstructions.

Planning estimates apply only to instances of the catalog's `Deep_sky_object` class. `Star`, `Satellite`, and `Planet` entries are excluded; the supplied Sun entry is a `Star`. Sun, Moon, and planet entries retain the coursework's static coordinates and are unsupported for time-dependent planning. The catalog remains available for exploration, but the redesign does not introduce an orbital ephemeris model.

Local Save and Load use `.star-map-plan.txt`; CSV Export uses `observing-plan.csv`. Plans are saved and loaded explicitly, without automatic session restoration. The local format is versioned, validates imported state, and preserves the current session if loading fails. Saving uses an adjacent temporary file before replacing the saved plan. CSV estimates use the same analysis as the workbench rather than a separate calculation path.

The Makefile builds the application and regression executables in `build/`. Run all commands from the repository root so catalog fixtures and resources resolve consistently.

## Validation scope

The earlier modular project compiled and rendered on macOS arm64 with the installed SplashKit SDK and Apple clang 21.0.0. Weather and background music were disabled during those interface checks.

The complete workbench compiled on the same macOS arm64 installation. `make test SKM=~/.splashkit/skm` passed 1,096 checks: 175 GUI/calendar helpers, 772 altitude-analysis checks, and 149 plan persistence/export checks. They include independent date/position fixtures, observer-state preservation, year-boundary sampling, malformed imports, and quoted CSV content.

A local integration harness checked fixed-anchor scrubbing, immediate target/analysis switching, category matching, the truncated year-end interval, and weather invalidation when resetting a session. The actual SplashKit renderer was run with deterministic example state to inspect the [target view](images/star-map.png), [plan panel](images/observing-plan.png), unsupported Solar System target state, and a time eight hours after the anchor. The example list is not a default plan. Native desktop input automation could not attach to the SDL window, so these rendering and integration checks are not a claim of automated end-to-end mouse/keyboard coverage.

Windows/Linux execution, successful authenticated weather retrieval, and exhaustive astronomy correctness remain unverified.

## Provenance and remaining limits

AI assistance was used for repository preparation and the subsequent workbench implementation, source review, documentation, and local verification. This note does not describe or infer how the original coursework was authored, and the HD outcome belongs to the original coursework submission.

The supplied CSV catalogs and design sketch did not include documented upstream redistribution licenses. No project-wide license has been selected. See [third-party notices](../THIRD_PARTY_NOTICES.md) for the separately licensed font and the SplashKit dependency.

Weather remains optional. The header shows the first fetched forecast entry rather than interpolating for the simulated time, and starting or loading a session clears its state. The synchronous request may pause the interface. Live authenticated retrieval has not been verified.
