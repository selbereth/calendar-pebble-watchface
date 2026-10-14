# Store listing: Calendar

Copy for the Pebble/Rebble developer portal. Screenshots are in `screenshots/`
(one per platform, sample data). The portal wants a source URL, a category and
a release note on every upload.

| Field | Value |
|---|---|
| Name | Calendar |
| Type | Watchface |
| Category | Daily |
| Version | 1.0.0 |
| Platforms | aplite, basalt, chalk, diorite, emery, flint, gabbro |
| Bundle | `build/calendar-pebble-watchface.pbw` |
| Source URL | https://github.com/selbereth/calendar-pebble-watchface (check the repo is public before submitting) |
| Support contact | samuelt12@gmail.com |
| Developer name | selbereth (`author` in `package.json`; the store's developer name is set on the dashboard) |
| Icon | `resources/images/icon.png` (25x25, wired in as the menu icon) |
| Store icons | `assets/icon_80.png` (iconSmall) and `assets/icon_144.png` (iconLarge), the sizes `pebble publish --icon-small/--icon-large` ask for; used at app creation only, so for the live listing upload them on the dashboard |
| Banner | `assets/banner_720x320.png` (720x320; the size is a guess, the publish tool does not say, so check the dashboard's requirement) |

## Description

**Short (the `--description` / one-liner):** Your upcoming calendar events on your watch face, with a big clock, date, weather and sunrise/sunset.

**Long:**

Your calendar, on your watch face. Calendar lists your next appointments, with their
start times, right under the clock, as many as fit your screen (up to 11). Events come from
the calendars already synced in the Pebble mobile app, so there is no account to link, no
ICS URL and no separate app to set up.

Nice extras: a big 12-hour clock, the date, the current temperature with today's high and
low, and sunrise and sunset. The last events and weather are kept, so the face is never blank
after a reboot. Square screens use a compact header; round screens stack the clock above the
info lines.

## Setup (required)

Calendar reads events through the Pebble mobile app's **experimental plugin
API** (app v1.14.0 or newer), which is off by default:

1. In the Pebble app: **Settings > Debug > Show debug options**, then turn on
   **Use experimental plugins**.
2. Make sure your calendars are synced and enabled in the app's **Calendar**
   screen.

Without this the watch shows "No upcoming events". The plugin API is
experimental and may change in a future app release.

## Notes for the reviewer

The calendar data comes from the Pebble mobile app's experimental plugin API, which the
platform docs say is not ready for the store and may change. Without "Use experimental
plugins" turned on, the face shows "No upcoming events". Say so in the reviewer-notes
field.

## Publishing

- **Core Devices store (default):** `pebble login` once (opens a browser), then
  `pebble publish --non-interactive --release-notes "<notes>" --description "<description>"
  --screenshots store/screenshots/*.png`. Every successful publish goes live immediately
  (`pebble_tool/commands/publish.py` hard-codes `isPublished` and `visible` to true; the
  `--is-published` flag does nothing). The UUID in `package.json` must never change.
- **Rebble store:** manual upload of `build/calendar-pebble-watchface.pbw` and the same
  screenshots through the Rebble developer portal; the publish tool does not target it.

## Permissions and data

- **Calendar:** reads event names and start times from the Pebble app.
- **Location:** used only to fetch local weather and sun times.
- **Network:** requests weather from [Open-Meteo](https://open-meteo.com/),
  sending the phone's coordinates. No account, no key, nothing else is sent.
- Temperatures are in Fahrenheit (a constant in `src/pkjs/index.js`).

## Release notes (1.0.0)

First release. Time, date, weather, sunrise/sunset and your next calendar
events on every Pebble platform, with inline or stacked layout depending on
screen size.
