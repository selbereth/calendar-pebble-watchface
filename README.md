# Calendar Watchface

A Pebble watchface that shows the time, date, weather and sunrise/sunset at
the top and your next upcoming calendar events below, as many as fit the screen
(up to 11).

## How it works

- The watch app (`src/c/calendar-face.c`) draws the header and as many event
  rows (time + title) as fit, and persists the last-received events so they
  survive a watch reboot.
- The PebbleKit JS companion (`src/pkjs/index.js`) subscribes to the Pebble
  mobile app's built-in `calendar/event` plugin source and pushes the next 11
  upcoming events to the watch over `AppMessage`. This reads whatever
  calendars are already synced through the Pebble app itself -- no ICS URL,
  no separate companion app, no manual configuration.

## Setup

This relies on the mobile app's **experimental plugin API** (v1.14.0+),
which is off by default:

1. In the Pebble app: **Settings → Debug → Show debug options**, then enable
   **Use experimental plugins**.
2. Make sure your calendars are synced and enabled in the Pebble app's
   **Calendar** screen.
3. Install this watchface. It should pick up your upcoming events
   automatically.

The plugin API is experimental and may change; the store listing says so
(see `store/LISTING.md`).

## Building

This is a standard `package.json`-based Pebble project:

```
pebble build
pebble install --phone <phone-ip>   # or --emulator basalt
```

Requires the Pebble/Rebble SDK tooling (`pebble-tool`).
