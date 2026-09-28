# AGENTS.md

## Project overview

A Pebble watchface ("Calendar") that shows the current time at the top and
the next 3 upcoming calendar events at the bottom. See `README.md` for full
setup/build instructions.

- `src/c/calendar-face.c` — watch app: draws time + up to 3 event rows
  (time + title), persists last-received events across reboot.
- `src/pkjs/index.js` — PebbleKit JS companion: subscribes to the Pebble
  mobile app's built-in experimental `calendar/event` plugin source and
  pushes the next 3 events to the watch over `AppMessage`. No ICS feed, no
  separate companion app.
- `package.json` — standard Pebble `package.json` project config
  (`pebble` key: UUID, target platforms, `messageKeys` shared between the
  JS and C sides).

## Build & test

This is a `pebble-tool` project; there is no other test suite.

```
pebble build
pebble install --phone <phone-ip>   # or --emulator basalt
```

`pebble build` requires the Pebble/Rebble SDK tooling. If it isn't
available in this environment, say so rather than claiming a build was
verified.

## Conventions

- Keep `messageKeys` in `package.json` in sync with the `dict['EVENT_..._TITLE'/'_TIME']`
  keys sent from `src/pkjs/index.js` and the keys read in `src/c/calendar-face.c`.
- The plugin API used in `index.js` (`Pebble.subscribeToSource`,
  category `calendar`/item `event`) is experimental (mobile app v1.14.0+,
  requires enabling "Use experimental plugins" in the Pebble app's debug
  settings). Don't assume it behaves like a stable, documented API.
- This targets sideloading/personal use, not an app store submission.
