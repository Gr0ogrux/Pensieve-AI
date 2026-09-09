# Receiver / Server

`receiver_stage3.py` runs on a PC on your home network (ideally one
that's on most of the time). It receives recordings from the pendant,
transcribes them locally, classifies them with a small AI call, and
creates the resulting Calendar event / Task in your Google account.

This is the most involved part of the whole build — budget maybe an
hour for first-time setup, most of it spent in Google's account setup
screens rather than actual coding.

## 1. Install Python dependencies

```
python -m pip install flask faster-whisper google-generativeai google-api-python-client google-auth-httplib2 google-auth-oauthlib
```

(`google-generativeai` is technically deprecated in favor of
`google-genai` — it still works fine as of this writing, but if you're
setting this up fresh and want to future-proof it, consider porting
the Gemini calls to the newer package.)

## 2. Set up Google Calendar / Tasks access

You need a small Google Cloud project with OAuth credentials so the
script can create events/tasks on your behalf. Free, no credit card
required for this part:

1. Go to [console.cloud.google.com](https://console.cloud.google.com),
   create a new project
2. Enable the **Google Calendar API** and **Google Tasks API**
   (APIs & Services > Library, search and enable each)
3. Set up the OAuth consent screen (APIs & Services > OAuth consent
   screen, or "Google Auth Platform" in newer console UIs) — choose
   **External**, fill in basic app info, and add your own Google
   account under **Test users** (this matters — without it, you won't
   be able to authorize your own script)
4. Under **Credentials**, create an **OAuth client ID**, application
   type **Desktop app**
5. Download the credentials JSON file, rename it to `credentials.json`,
   and place it in this `server/` folder

**`credentials.json` should never be committed to a public repo.** A
`.gitignore` in this repo already excludes it, along with the
`token.json` file that gets created after your first successful login.

## 3. Get a Gemini API key

1. Go to [aistudio.google.com](https://aistudio.google.com), sign in
2. Click **Get API key** > **Create API key** (you can tie it to the
   same project from step 2, or a separate one — doesn't matter)
3. Set it as an environment variable rather than pasting it into the
   script:
   - Windows (Command Prompt): `set GEMINI_API_KEY=your_key_here`
     (do this in the same window before running the script, or set it
     permanently via System Properties > Environment Variables)
   - Mac/Linux: `export GEMINI_API_KEY=your_key_here`

The free tier has fairly low rate limits (a handful of requests per
minute, ~20/day at time of writing) — fine for light testing, but
you'll likely want to enable billing once you're using this daily. The
actual cost is negligible: each classification call costs a fraction
of a cent, so even heavy use runs well under $1/month. See
[ai.google.dev/gemini-api/docs/rate-limits](https://ai.google.dev/gemini-api/docs/rate-limits)
for current numbers.

## 4. Find this PC's local IP and run it

```
ipconfig          (Windows — look for "IPv4 Address")
ifconfig          (Mac/Linux)
```

Use that address in the firmware's `SERVER_URL` (see `../firmware/README.md`).

```
python receiver_stage3.py
```

First run downloads the transcription model (a few hundred MB, one-time)
and opens a browser window for you to approve Google account access.
After that, it just runs.

## 5. Keep it running

For this to work day-to-day, the script needs to actually be running
whenever the pendant tries to sync. Options, roughly in order of
robustness:

- Simplest: a `.bat` file (Windows) that runs it, dropped into your
  Startup folder (`Win+R` > `shell:startup`) — starts automatically
  when you log in, but not before
- More robust: a Task Scheduler task set to run at system boot
  regardless of login state

## Customizing the classification

The prompt that decides event vs. task vs. note (and extracts
dates/times) is in the `classify_with_gemini()` function — plain
English, easy to adjust if you want different categories or behavior.

## Timezone

Calendar events are created with a hardcoded timezone
(`America/New_York` in `create_calendar_event()`) — change this to your
own if different.
