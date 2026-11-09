# Changelog

## 1.1.0

- New settings page (open it from the watchface's settings in the Pebble app):
  - Time: 12-hour, 24-hour, or follow the watch's own setting. Event times follow it too
    (`9:30a` in 12-hour mode).
  - Event times: same as the clock, 12-hour, or 24-hour, independent of the clock.
  - Date: words (`Nov 9`) or numbers (`11/9`), month first or day first, separator `/`, `.`
    (`9.11.`) or `-`, weekday shown or hidden.
  - Colours: white on black, or inverted (black on white).
  - Temperature: Fahrenheit or Celsius.
- Setup and the store listing no longer ask for "Use experimental plugins".
- The date no longer has a double space before single-digit days.

## 1.0.1

- Declares permissions for the Core Devices app's new permission system: Calendar, Location,
  and Internet limited to api.open-meteo.com (weather and sun times).
- Build is marked as using permissions (`manifestVersion: 2`).

## 1.0.0

- Initial release.
- Shows your upcoming calendar events with their start times, as many as fit the screen (up to 11).
- Also a big 12-hour clock, the date, current temperature with today's high and low, and
  sunrise and sunset.
- Events come from the Pebble app's built-in calendar sync (no ICS URL or setup in the
  watchface). Requires the Pebble app's experimental plugin API (Settings > Debug > Show
  debug options > "Use experimental plugins").
- Weather and sun times come from Open-Meteo using the phone's location.
- Square screens use a compact header; round screens stack the clock above the info lines.
