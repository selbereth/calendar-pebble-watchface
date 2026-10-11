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
  already wired into `package.json` via the `menuIcon` resource. For a
  watchface, this bundled icon is what the store listing uses too; `pebble
  publish` doesn't take a separate marketing-icon upload for this app type
  (that path in the tool is watchapp-only), so the 144x144 version sent
  earlier isn't actually needed here.
- Screenshots: **not included** -- these need to come from a real watch
  or emulator actually showing your events (with the experimental plugin
  flag on and a calendar enabled), one per platform shape you want to
  list (rectangular: Aplite/Basalt/Diorite/Emery/Flint; round: Chalk/
  Gabbro), named `<platform>_whatever.png` (e.g. `basalt_1.png`). Required
  for the very first publish -- see below.

## Submission checklist

Publishing now goes through `.github/workflows/publish-pebble-appstore.yml`
(see README.md "Publishing via GitHub Actions" for the full one-time setup
and per-release steps). Short version:

1. **One-time:** `pebble login` locally, copy the saved `id_token` into a
   `PEBBLE_FIREBASE_ID_TOKEN` GitHub Actions secret.
2. **Each release:** refresh that secret (the token expires in ~1 hour),
   bump `"version"` in `package.json`, then run the workflow from the
   Actions tab with release notes and (for the first run) screenshots.
3. That's it -- `pebble publish` registers the app on its first run (using
   the UUID already in `package.json`,
   `1f7f7c53-f504-4653-960f-910447e3d532`, which must stay fixed for all
   future versions) and uploads subsequent releases to it after that.

Two things worth remembering, spelled out in the README: every successful
run **publishes live immediately** (no draft mode, despite what the CLI's
flags imply), and I could not run this workflow myself to confirm it works
end-to-end -- the SDK/appstore domains are blocked from the sandbox this
was built in, so treat the first real run as a test.

If you'd rather target the community Rebble store (rebble.io/submit,
dev-portal.rebble.io) instead of the default Core Devices one, that's a
separate destination with its own listing -- the workflow's `api_base`
input can point at it if you have that URL, otherwise it's a manual
upload there.
