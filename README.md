<img width="824" height="825" alt="pensieve_device" src="https://github.com/user-attachments/assets/68c9b5d1-7f52-4b39-95d0-b60bf554cee6" />

# **Pensieve AI** — a wearable voice assistant built from scratch

A push-to-talk pendant that records what you say, transcribes it locally,
figures out whether it's a reminder, calendar event, or just a note, and
files it into Google Calendar / Tasks automatically - no phone app, no
subscription, no cloud company's assistant, just a $30-ish pile of parts
and open firmware.

Press the button, say something, let go. That's the entire interaction.

## What it actually does

1. Records while the button is held (up to 60s), then stops
2. Saves to a microSD card - recordings are never lost even with no WiFi
3. Opportunistically syncs over WiFi whenever it's in range (on button
   press, or automatically once an hour even if you haven't used it)
4. A receiver script on a PC transcribes the audio locally (private,
   free - no audio ever leaves your network)
5. A small AI call classifies the transcript into an event / task / note
   and extracts the relevant details (dates, times, titles)
6. That gets pushed straight into Google Calendar or Google Tasks
7. Runs in deep sleep between uses for real multi-day battery life

## Why build this instead of buying one

Commercial AI pendants exist (Bee, Fieldy, etc.) but they're subscription
products with someone else's cloud in the middle. This is the same idea,
built from parts you can buy today, with the audio-to-transcript step
happening entirely on your own hardware, and the code fully open so you
can see (and change) exactly what it's doing with what you say.

## Hardware

| Part | Notes |
|---|---|
| Seeed XIAO ESP32-S3 Sense | The board - has the mic, camera (unused here), and microSD slot built in |
| LiPo battery, 3.7V, ~1000mAh, JST-PH1.25 connector | Solders directly to the board's BAT+/BAT- pads (no connector on the board itself) |
| Momentary tactile push button | 6x6mm style, 4-leg (2 electrical pairs) |
| 4x 4mm x 1mm neodymium magnets | Holds the case closed |
| Necklace cord | Threads through the printed bail |
| A 3D printer | PETG recommended for the final print, PLA fine for test-fitting |

Total cost is roughly $25-35 depending on where you source parts.

## Repo structure

- **`firmware/`** — the Arduino sketch that runs on the pendant itself
- **`case/`** — the 3D-printable case (parametric OpenSCAD source + ready-to-print STLs)
- **`server/`** — the Python script that runs on a PC, receives recordings, transcribes and classifies them, and creates the Calendar/Tasks entries
- **`docs/`** — supplementary notes (wiring, troubleshooting)

Each folder has its own README with setup specifics.

## Quick start

1. **Print the case** — see `case/README.md`
2. **Wire the button and battery** — see `docs/WIRING.md`
3. **Flash the firmware** — see `firmware/README.md`
4. **Set up the receiver** — see `server/README.md` (this is the more
   involved part: Python environment, a free Google Cloud project for
   Calendar/Tasks access, and a Gemini API key)
5. Press the button, say something, check your Google Tasks a few
   seconds later

## Known limitations / honest caveats

- **Battery life is genuinely limited by the ESP32-S3's always-on idle
  draw** if you don't use the deep-sleep firmware — expect closer to
  "charge nightly like a phone" than "charge weekly." The deep-sleep
  version in this repo addresses this, at the cost of a small delay
  (and possible clipped first syllable) each time you press the button,
  since the chip does a real reboot on every wake.
- **The XIAO ESP32-S3 has no dedicated JST battery connector** — only
  bare solder pads. Soldering directly to a LiPo is genuinely more
  dangerous than a plug-in connector; read `docs/WIRING.md` before
  attempting it.
- **This was built and debugged interactively over a single very long
  session**, including working through several real hardware/firmware
  interaction bugs (documented in `docs/` for anyone hitting the same
  walls). It works, but it's a hobbyist build, not a polished product.

## License

MIT — do whatever you want with this, attribution appreciated but not required.

---

*This project's code — firmware, case design, and server — was developed
with assistance from Google Gemini and Anthropic Claude.*
