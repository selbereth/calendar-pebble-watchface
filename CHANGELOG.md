# Changelog

## 1.0.0

- Initial release.
- Shows a big 12-hour clock, the date, current temperature with today's high and low, and
  sunrise and sunset, with your upcoming calendar events below (as many as fit, up to 11).
- Events come from the Pebble app's built-in calendar sync (no ICS URL or setup in the
  watchface). Requires the Pebble app's experimental plugin API (Settings > Debug > Show
  debug options > "Use experimental plugins").
- Weather and sun times come from Open-Meteo using the phone's location.
- Square screens use a compact header; round screens stack the clock above the info lines.
