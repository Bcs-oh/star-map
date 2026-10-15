# Repository preparation notes

This repository edition was prepared from the supplied FIT1045 coursework files in October 2026. The original Drive files were retained unchanged.

## Changes from the supplied coursework

- Retained the modular C++ implementation and CSV catalogs; omitted the duplicate consolidated source file.
- Removed the embedded OpenWeather credential and read `OPENWEATHER_API_KEY` from the process environment instead.
- Initialized observation state and cloud-cover fields. Cleared stale forecasts before a request, skipped requests without a key, checked for a forecast list, and displayed an unavailable state when no forecast is present.
- Replaced the original Arial resource with the unmodified Liberation Sans font and its license.
- Used SplashKit's standard `resources/fonts` and `resources/sounds` directories.
- Made background music optional and omitted the original soundtrack and videos.
- Added README documentation, third-party notices, a Git ignore file, the original sketch, and a sample application screenshot.

The original astronomical calculations and overall interface design were retained. AI assistance was used for this repository preparation: source review, documentation, credential removal, resource adjustments, and local verification. This note does not describe or infer how the original coursework was authored.

## Observation reliability update

The October 16 update adds complete calendar and clock validation, including Gregorian leap-year rules and years 1–9999. Invalid input displays an inline warning and does not update the observation. The two hour controls share the same rollover logic and cannot move beyond the supported year limits.

UTC labels preserve fractional offsets as hours and minutes without changing the double used in astronomical calculations. Object marker radii have a one-pixel minimum, used for both drawing and click selection. The original bright-object sizes are retained.

The accompanying `tests/gui_regression.cpp` performs 175 checks against calendar edge cases, time rollover, timezone labels, and the complete supplied object catalog. All checks passed with the local SplashKit installation. The README documents how to run them.

Visual checks were rendered from the updated interface with a temporary local harness: [Kathmandu's quarter-hour timezone](images/fractional-timezone.png) and [rejection of February 30](images/invalid-date.png). Weather and background music were disabled during these checks.

## Inherited limitations

- Solar-system objects use static catalog coordinates for every date.
- City and target matching is exact and case-sensitive.
- Forecast timestamps outside the returned interval use endpoint approximations.
- A network request is synchronous and can temporarily pause the interface. Live authenticated weather retrieval has not been verified.

## Validation

The modular project compiled successfully on macOS arm64 with the installed SplashKit SDK and Apple clang 21.0.0. The application window and replacement font rendered successfully. A temporary local harness supplied the README's example inputs and rendered the screenshot with weather and background music disabled; that harness is not part of the repository.

Windows/Linux execution, successful authenticated weather retrieval, and exhaustive astronomy correctness have not been verified.
