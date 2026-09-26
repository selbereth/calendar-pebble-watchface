# Calendar Watchface

A Pebble watchface that shows the current time at the top and your next 3
upcoming calendar events at the bottom.

## How it works

- The watch app (`src/c/calendar-face.c`) draws the time and up to 3 event
  rows (time + title), and persists the last-received events so they
  survive a watch reboot.
- The PebbleKit JS companion (`src/pkjs/index.js`) subscribes to the Pebble
  mobile app's built-in `calendar/event` plugin source and pushes the next 3
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
3. Install this watchface. It should pick up your next 3 events
   automatically.

Since the plugin API is still experimental and unstable, this is meant for
sideloading/personal use, not an app store submission.

## Building

This is a standard `package.json`-based Pebble project:

```
pebble build
pebble install --phone <phone-ip>   # or --emulator basalt
```

Requires the Pebble/Rebble SDK tooling (`pebble-tool`).
