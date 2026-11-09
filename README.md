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

## Settings

Open the watchface's settings in the Pebble app: 12-hour, 24-hour or follow-the-watch time;
date as words (`Nov 9`) or numbers (`11/9`, `9.11.`, `11-9`), month or day first, weekday on
or off; white on black or inverted. The page is built in `src/pkjs/index.js` (`configPage`),
stored in the phone's `localStorage`, and sent to the watch over `AppMessage` (keys
`TIME_FORMAT`, `DATE_STYLE`, `DATE_DAY_FIRST`, `DATE_SEPARATOR`, `SHOW_WEEKDAY`, `INVERT`),
where it is persisted and the layout is rebuilt. Event times arrive as 24-hour text and the
watch re-writes them for the 12-hour setting.

## Setup

1. Make sure your calendars are synced and enabled in the Pebble app's
   **Calendar** screen.
2. Install this watchface and allow the Calendar permission. It should pick up your upcoming
   events automatically. No debug setting is needed.

## Building

This is a standard `package.json`-based Pebble project:

```
pebble build
pebble install --phone <phone-ip>   # or --emulator basalt
```

For a build the Core Devices app treats as using permissions (needed for the store and for
real-phone testing), package with the app's script instead of plain `pebble build`:
`python3 <coredevices-mobileapp>/src/scripts/pack-plugin-pbw.py .`

Requires the Pebble/Rebble SDK tooling (`pebble-tool`).
