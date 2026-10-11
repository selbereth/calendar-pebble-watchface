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

The plugin API is still experimental/unstable per its own docs ("don't ship
to the app store yet"), so submitting this to the store carries the risk of
rejection now or breakage later if the API changes. See `STORE_LISTING.md`
for the submission checklist and that risk written out in full.

## Building

This is a standard `package.json`-based Pebble project:

```
pebble build
pebble install --phone <phone-ip>   # or --emulator basalt
```

Requires the Pebble/Rebble SDK tooling (`pebble-tool`).

## Publishing via GitHub Actions

`.github/workflows/publish-pebble-appstore.yml` runs `pebble publish` for
you. It's **manual-only** (`workflow_dispatch`), on purpose -- there's no
safe way to run it unattended:

- **It publishes live immediately on success.** The installed `pebble-tool`
  (v5.0.40) hard-codes `isPublished`/`visible` to `true` in its upload
  requests regardless of any flag -- there is currently no draft/staging
  option, despite what the CLI's own `--help` text implies. The workflow
  requires typing `PUBLISH` into a confirmation input to run, as the only
  safety net available.
- **The auth token expires in about an hour**, so it can't be set once and
  forgotten -- see setup below.
- **I could not run this workflow end-to-end myself** -- the SDK/appstore
  domains (`sdk.repebble.com`, `appstore-api.repebble.com`) are blocked from
  the sandbox this was built in. Do a careful first run and watch the logs.

### One-time setup

1. Install `pebble-tool` locally (`pip install pebble-tool`) and run
   `pebble login` -- this opens a browser to sign in (GitHub OAuth).
2. Find the token it saved:
   - Linux: `~/.local/share/pebble-sdk/oauth_firebase/firebase_oauth_storage.json`
   - macOS: `~/Library/Application Support/Pebble SDK/oauth_firebase/firebase_oauth_storage.json`
   - Copy the `id_token` field's value.
3. In the GitHub repo: **Settings → Secrets and variables → Actions → New
   repository secret**, name it `PEBBLE_FIREBASE_ID_TOKEN`, paste the token.

### Each time you publish

1. Repeat step 2-3 above to refresh the secret -- the token is only good for
   about an hour, so do this right before running the workflow, not hours
   ahead of time.
2. Bump `"version"` in `package.json` and commit/push that.
3. Go to **Actions → Publish to Pebble Appstore → Run workflow** and fill in:
   - `confirm`: literally `PUBLISH`.
   - `release_notes`: what changed.
   - `screenshots`: **required for your very first publish** (the app
     doesn't exist on the store yet) unless you set `emulator_capture` to
     `true` -- space-separated repo-relative paths, filenames starting with
     the platform name (e.g. `screenshots/basalt_1.png`). Optional after the
     app exists.
   - `emulator_capture`: auto-captures screenshots from the Pebble emulator
     instead of using files. Unverified from this sandbox -- prefer
     supplying real screenshots if you can.
4. After it's live, visit the dashboard the job output links to if you want
   to edit the listing (category, longer description, etc.) beyond what the
   workflow sets.
