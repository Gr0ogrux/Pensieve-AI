# Firmware

`xiao_pendant_pushtotalk.ino` — the deep-sleep, push-to-talk version.
This is what actually runs on the pendant.

## Board setup (Arduino IDE)

1. Install the **"esp32" board package by Espressif Systems** via
   Boards Manager (not "Arduino ESP32 Boards" — there are two similarly
   named packages, you want Espressif's).
2. Select **Tools > Board > XIAO_ESP32S3**
3. Set **Tools > PSRAM > Enabled** (or "OPI PSRAM" depending on your
   core version) — required, the recording buffer lives in PSRAM
4. Upload speed of 921600 is fine (the default for this board)

## Before uploading

Open the sketch and fill in these three lines near the top:

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* SERVER_URL    = "http://YOUR_PC_LOCAL_IP:5000/upload";
```

`SERVER_URL` is the local IP address of whichever PC is running the
receiver script (see `../server/`), found via `ipconfig` (Windows) or
`ifconfig`/`ip addr` (Mac/Linux). It has to be the PC's IP on your
*local* network (e.g. `192.168.1.x`), not a public address.

## How it behaves

- Press and hold the button (wired to pin D0) to record, release to stop
- Recordings longer than 60 seconds are automatically cut off (safety cap)
- Presses shorter than ~0.5s are discarded as accidental taps
- After recording, it tries to sync over WiFi (10s timeout) — if no
  WiFi is in range, the file just waits on the SD card and gets picked
  up on the next successful sync
- It also wakes up on its own once an hour to try syncing even without
  a button press
- Files that fail to upload twice are deleted automatically (almost
  always an accidental/garbage recording, not something worth
  retrying forever)
- Synced recordings are archived on the SD card and auto-deleted after
  7 days (configurable via `SENT_FILE_RETENTION_DAYS`)

## The deep sleep tradeoff (read this before you're confused by it)

This version puts the chip into deep sleep between actions rather than
staying fully awake — this is what gets you multi-day battery life
instead of roughly one day. The tradeoff: deep sleep is closer to a
full reboot than a pause. Every time it wakes (button press or hourly
timer), it's genuinely re-running `setup()` from scratch. That costs a
small delay before recording can start, and it's possible for the very
first instant of speech to be clipped while the mic finishes
initializing. If that's more annoying in practice than the battery
life is worth to you, an always-on (no deep sleep) version is a much
simpler modification — remove the `goToSleep()` call and the wake-cause
branching in `setup()`, and let `loop()` poll the button directly instead.

## Timezone

The NTP time sync (used for tracking how old archived files are)
defaults to US Eastern (`EST5EDT,M3.2.0,M11.1.0`). If you're elsewhere,
change the `TZ_STRING` constant — search "POSIX TZ string" for your
region's format.

## Wiring

See `../docs/WIRING.md` for the button and battery connections.
