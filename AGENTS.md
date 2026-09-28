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

On macOS (Apple Silicon), none of the tooling is available out of the box.
What actually worked, in order:

1. **`pebble-tool`**: install via pip in a venv using **Python 3.10+**
   (Homebrew's `python@3.13`, not the system/Xcode Python 3.9 — pebble-tool
   5.0.38+ requires it). Install with `--no-deps`, then install its other
   deps manually, *skipping* `pypkjs`:
   ```
   python3.13 -m venv .pebble-venv && source .pebble-venv/bin/activate
   pip install --no-deps pebble-tool
   pip install cobs colorama freetype-py google-auth-oauthlib google-auth \
     httplib2 libpebble2 oauth2client packaging pillow progressbar2 \
     pyasn1-modules pyasn1 pypng pyqrcode pyserial requests rsa six \
     sourcemap websocket-client websockify wheel
   ```
   `pypkjs` (pebble-tool's own dependency) requires `stpyv8`, which has no
   prebuilt wheel for macOS arm64 and won't build from source. It's only
   needed for running the JS companion inside the emulator — `pebble build`
   and `pebble install` work fine without it.

2. **SDK**: `pebble sdk install 4.33.1` (or newer/latest available).

3. **`wscript`**: newer SDKs require a `wscript` file in the project root
   (waf build rules) or `pebble build` fails with "This project is very
   outdated, and cannot be handled by this SDK." One is checked into this
   repo already; if it's ever missing, copy it from
   `~/Library/Application Support/Pebble SDK/SDKs/<version>/sdk-core/pebble/common/templates/app/wscript`.

4. **ARM toolchain**: Homebrew's `arm-none-eabi-gcc` formula ships *without*
   newlib headers (`stdint.h` etc. fail to resolve) and there's no separate
   newlib formula. The `gcc-arm-embedded` cask has the real toolchain but its
   installer needs `sudo`, which isn't available non-interactively. Workaround:
   `brew install --cask gcc-arm-embedded` (let the sudo install fail/cancel —
   this just populates the download cache), then extract the cached `.pkg`
   without sudo:
   ```
   pkgutil --expand <cached .pkg> expanded
   cd expanded && cat Payload | gzip -dc | (cd ../.arm-gnu-toolchain && cpio -id)
   ```
   Add `.arm-gnu-toolchain/bin` to `PATH` before running `pebble build`.

5. **Emulator (`pebble install --emulator basalt`) does not work on macOS
   arm64.** It needs `qemu-pebble`, a custom QEMU fork with no arm64 binary
   and no maintained build path (the only Homebrew formula,
   `pebble/pebble-sdk/pebble-qemu`, only has Intel bottles from 2016 and is
   broken with current Homebrew). Don't try to install the emulator on this
   platform — go straight to phone install/sideload instead.

Putting it together, a full build on this machine:
```
source .pebble-venv/bin/activate
export PATH="$PWD/.arm-gnu-toolchain/bin:$PATH"
pebble build
pebble install --phone <phone-ip>   # owner's phone: 192.168.1.216 (may change with DHCP)
```

### Installing to a physical watch

- **Sideload** (no dev mode needed): get `build/*.pbw` onto the phone
  (AirDrop/email/etc.) and tap it — the Pebble/Rebble app installs it over
  Bluetooth.
- **`pebble install --phone <ip>`**: requires Developer Connection enabled
  in the phone app. On the Rebble Android app this is two nested settings,
  not one: overflow menu → Settings → enable "Developer Mode" first, *then*
  a "Developer Connection" entry appears — open it and toggle it on to see
  the Server IP. On iOS, this feature doesn't exist at all (sandboxing
  restrictions); sideloading is the only option there.

If none of this tooling is available in a given environment, say so rather
than claiming a build was verified.

## Conventions

- Keep `messageKeys` in `package.json` in sync with the `dict['EVENT_..._TITLE'/'_TIME']`
  keys sent from `src/pkjs/index.js` and the keys read in `src/c/calendar-face.c`.
- The plugin API used in `index.js` (`Pebble.subscribeToSource`,
  category `calendar`/item `event`) is experimental (mobile app v1.14.0+,
  requires enabling "Use experimental plugins" in the Pebble app's debug
  settings). Don't assume it behaves like a stable, documented API.
- This targets sideloading/personal use, not an app store submission.
