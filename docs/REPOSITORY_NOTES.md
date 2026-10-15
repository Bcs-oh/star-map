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

## Inherited limitations

- Solar-system objects use static catalog coordinates for every date.
- City and target matching is exact and case-sensitive.
- Date/time fields validate integer syntax without full range or calendar validation.
- Fractional UTC offsets are truncated in the displayed timezone label.
- Very faint catalog objects can receive a negative calculated marker radius.
- Forecast timestamps outside the returned interval use endpoint approximations.
- A network request is synchronous and can temporarily pause the interface. Live authenticated weather retrieval has not been verified.

## Validation

The modular project compiled successfully on macOS arm64 with the installed SplashKit SDK and Apple clang 21.0.0. The application window and replacement font rendered successfully. A temporary local harness supplied the README's example inputs and rendered the screenshot with weather and background music disabled; that harness is not part of the repository.

Windows/Linux execution, successful authenticated weather retrieval, and exhaustive astronomy correctness have not been verified.
