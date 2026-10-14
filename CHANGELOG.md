# Changelog

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
