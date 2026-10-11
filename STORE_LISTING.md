# App Store Listing

Draft copy and a submission checklist.

## Listing copy

**Name:** Calendar

**Short description (one line):**
> Time at a glance, with your next 3 calendar events underneath.

**Long description:**
> A simple watchface: the time up top, and your next 3 upcoming calendar
> events underneath (time + title for each). Events are read directly
> from the calendars you already have synced to the Pebble app -- there's
> nothing to configure, no account to connect, and no URL to paste in.
>
> Requires enabling the Pebble app's experimental plugin feature
> (Settings → Debug → Show debug options → "Use experimental plugins")
> and having at least one calendar enabled in the Pebble app's Calendar
> screen.

**Category:** Daily / Watchfaces

**Supported platforms:** Aplite, Basalt, Chalk, Diorite, Emery, Flint,
Gabbro (everything in `targetPlatforms` in `package.json`).

**Permissions requested:** Calendar (to read your upcoming events via the
Pebble app's calendar plugin -- nothing leaves your phone; this app makes
no network requests of its own).

**Support contact:** samuelt12@gmail.com

**Developer/company name shown on the listing:** samuelt12 (matches
`"author"` in `package.json`).

## Known risk to disclose in reviewer notes

This watchface's calendar data comes from the Pebble mobile app's
**experimental plugin API**, which the platform's own docs say not to
ship to the app store yet because it's unstable and may change. Worth
calling out explicitly in whatever "notes for the reviewer" field the
submission form offers, so it isn't a surprise during review -- and worth
knowing that a future app update could change or break this before you
get a chance to update the watchface.

## Assets

- App icon (25x25 PNG, launcher icon): `resources/images/icon.png` --
  already wired into `package.json` via the `menuIcon` resource.
- Store listing icon (larger marketing icon): generated at 144x144, sent
  to you separately -- confirm the exact size the portal asks for at
  upload time, and re-export from `resources/images/icon.png` if it needs
  to be bigger.
- Screenshots: **not included** -- these need to come from a real watch
  or emulator actually showing your events (with the experimental plugin
  flag on and a calendar enabled), one per platform shape you want to
  list (rectangular: Aplite/Basalt/Diorite/Emery/Flint; round: Chalk/
  Gabbro). `pebble screenshot` can capture these from the emulator, or
  photograph/screenshot a real watch.

## Submission checklist

These steps need your own accounts/hardware and can't be done from here:

1. **Build a release `.pbw`:** `pebble build` (needs the Pebble/Rebble
   SDK installed locally -- `pebble sdk install latest`) or build via
   CloudPebble.
2. **Decide which store:** the official Core Devices Pebble appstore
   (via the Developer Dashboard / `pebble publish`) and/or the community
   Rebble store (rebble.io/submit, dev-portal.rebble.io) are separate
   destinations with separate listings -- pick one or both.
3. **Register the app / create the listing**, using the UUID already in
   `package.json` (`1f7f7c53-f504-4653-960f-910447e3d532`) -- that UUID
   needs to stay fixed for all future versions of this app.
4. **Capture screenshots** (see above).
5. **Upload the `.pbw`**, icon, and screenshots, and submit for review,
   mentioning the experimental-plugin dependency in the reviewer notes.

I couldn't verify the exact current submission UI/steps from here (the
SDK/docs domains are blocked from this sandbox) -- the portal itself will
walk you through specifics, but the list above is what it will ask for.
