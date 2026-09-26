# Calendar Watchface

A Pebble watchface that shows the current time at the top and your next 3
upcoming calendar events at the bottom.

## How it works

- The watch app (`src/c/calendar-face.c`) draws the time and up to 3 event
  rows (time + title), and persists the last-received events so they
  survive a watch reboot.
- The PebbleKit JS companion (`src/pkjs/index.js`, `src/pkjs/ics-parser.js`)
  fetches your calendar's iCalendar (`.ics`) feed, finds the next 3 upcoming
  events (expanding simple daily/weekly recurring events), and sends them to
  the watch over `AppMessage`. It refetches every 15 minutes and whenever
  the Pebble app reconnects.

## Setup

1. Get the secret `.ics` feed URL for your calendar, e.g. in Google
   Calendar: **Settings → [your calendar] → Integrate calendar → Secret
   address in iCal format**.
2. Install the watchface on your phone, open its settings from the Pebble
   app, and paste the `.ics` URL in.

## Building

This is a standard `package.json`-based Pebble project:

```
pebble build
pebble install --phone <phone-ip>   # or --emulator basalt
```

Requires the Pebble/Rebble SDK tooling (`pebble-tool`).
