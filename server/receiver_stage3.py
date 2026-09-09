"""
Pendant recording receiver — Stage 3 (final): full pipeline

Recording arrives -> transcribed locally (faster-whisper) -> Gemini reads
the transcript and figures out whether it's an event, a reminder, or a
note, and pulls out the relevant details -> creates the actual Google
Calendar event or Google Task automatically.

SETUP:
    python -m pip install flask faster-whisper google-generativeai google-api-python-client google-auth-httplib2 google-auth-oauthlib

BEFORE RUNNING:
    1. Place credentials.json (from Google Cloud Console) in this same folder.
    2. Set your Gemini API key below, or as an environment variable GEMINI_API_KEY.

FIRST RUN:
    A browser window will pop up asking you to log into Google and approve
    Calendar/Tasks access (this is the OAuth consent screen we set up).
    After approving once, a token.json file is saved here so it won't ask
    again on future runs (until the token expires, which Google auto-
    refreshes quietly in the background).
"""

import json
import os
from pathlib import Path
from datetime import datetime, timedelta

from flask import Flask, request
from faster_whisper import WhisperModel
import google.generativeai as genai

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build

# ---- FILL THIS IN (or set the GEMINI_API_KEY environment variable) ----
GEMINI_API_KEY = os.environ.get("GEMINI_API_KEY", "YOUR_GEMINI_API_KEY_HERE")
# -------------------------------------------------------------------------

SCOPES = [
    "https://www.googleapis.com/auth/calendar",
    "https://www.googleapis.com/auth/tasks",
]
CREDENTIALS_FILE = "credentials.json"
TOKEN_FILE = "token.json"

RECORDINGS_DIR = Path("recordings")
RECORDINGS_DIR.mkdir(exist_ok=True)

app = Flask(__name__)

# ---- Transcription model (loaded once) ----
print("Loading transcription model...")
whisper_model = WhisperModel("small", device="cpu", compute_type="int8")
print("Transcription model loaded.")

# ---- Gemini setup ----
genai.configure(api_key=GEMINI_API_KEY)
gemini_model = genai.GenerativeModel("gemini-3.6-flash")

# ---- Google Calendar/Tasks auth ----
def get_google_creds():
    creds = None
    if os.path.exists(TOKEN_FILE):
        creds = Credentials.from_authorized_user_file(TOKEN_FILE, SCOPES)
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file(CREDENTIALS_FILE, SCOPES)
            creds = flow.run_local_server(port=0)
        with open(TOKEN_FILE, "w") as f:
            f.write(creds.to_json())
    return creds

print("Authenticating with Google (a browser window may open)...")
google_creds = get_google_creds()
calendar_service = build("calendar", "v3", credentials=google_creds)
tasks_service = build("tasks", "v1", credentials=google_creds)
print("Google authentication ready.")


def transcribe(wav_path: Path) -> str:
    segments, info = whisper_model.transcribe(str(wav_path))
    return " ".join(s.text.strip() for s in segments).strip()


def classify_with_gemini(transcript: str) -> dict:
    """Ask Gemini to turn the transcript into structured data."""
    now = datetime.now()
    prompt = f"""You are parsing a short voice note transcript into a structured action.
The current date and time is: {now.strftime('%A, %Y-%m-%d %H:%M')}

Transcript: "{transcript}"

Decide if this is one of:
- "event": something with a specific date/time on the calendar
- "task": a reminder or to-do, may or may not have a due date/time
- "note": just information to remember, no action or date needed

Respond with ONLY valid JSON, no other text, in this exact shape:
{{
  "type": "event" | "task" | "note",
  "title": "short title",
  "date": "YYYY-MM-DD" or null,
  "time": "HH:MM" (24hr) or null,
  "details": "any extra context from the transcript, or empty string"
}}

If a relative date is mentioned (e.g. "tomorrow", "next Tuesday"), resolve
it to an actual date based on the current date given above."""

    response = gemini_model.generate_content(prompt)
    text = response.text.strip()
    # Gemini sometimes wraps JSON in ```json ... ``` -- strip that if present
    if text.startswith("```"):
        text = text.split("```")[1]
        if text.startswith("json"):
            text = text[4:]
    return json.loads(text.strip())


def create_calendar_event(parsed: dict):
    date = parsed.get("date") or datetime.now().strftime("%Y-%m-%d")
    time_str = parsed.get("time") or "09:00"
    start_dt = datetime.strptime(f"{date} {time_str}", "%Y-%m-%d %H:%M")
    end_dt = start_dt + timedelta(hours=1)

    event = {
        "summary": parsed["title"],
        "description": parsed.get("details", ""),
        "start": {"dateTime": start_dt.isoformat(), "timeZone": "America/New_York"},
        "end": {"dateTime": end_dt.isoformat(), "timeZone": "America/New_York"},
    }
    result = calendar_service.events().insert(calendarId="primary", body=event).execute()
    return result.get("htmlLink")


def create_task(parsed: dict):
    task = {"title": parsed["title"], "notes": parsed.get("details", "")}
    if parsed.get("date"):
        # Tasks API wants an RFC3339 timestamp; time-of-day is ignored by Google Tasks UI
        due_dt = datetime.strptime(parsed["date"], "%Y-%m-%d")
        task["due"] = due_dt.isoformat() + "Z"
    result = tasks_service.tasks().insert(tasklist="@default", body=task).execute()
    return result.get("id")


@app.route("/upload", methods=["POST"])
def upload():
    data = request.get_data()
    if not data:
        return "No data received", 400

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    wav_path = RECORDINGS_DIR / f"recording_{timestamp}.wav"
    wav_path.write_bytes(data)
    print(f"[+] Saved {wav_path} ({len(data)} bytes)")

    print("[*] Transcribing...")
    transcript = transcribe(wav_path)
    print(f"[+] Transcript: \"{transcript}\"")
    (wav_path.with_suffix(".txt")).write_text(transcript, encoding="utf-8")

    if not transcript:
        return "Saved, but transcript was empty -- nothing to act on.", 200

    print("[*] Asking Gemini to classify...")
    try:
        parsed = classify_with_gemini(transcript)
        print(f"[+] Parsed: {parsed}")
    except Exception as e:
        print(f"[-] Gemini parsing failed: {e}")
        return f"Saved and transcribed, but classification failed: {e}", 200

    try:
        if parsed["type"] == "event":
            link = create_calendar_event(parsed)
            print(f"[+] Calendar event created: {link}")
            result_msg = f"Created calendar event: {parsed['title']}"
        elif parsed["type"] == "task":
            task_id = create_task(parsed)
            print(f"[+] Task created: {task_id}")
            result_msg = f"Created task: {parsed['title']}"
        else:
            # "note" -- store as a task with no due date, acts as a simple note list
            task_id = create_task(parsed)
            print(f"[+] Note saved as task: {task_id}")
            result_msg = f"Saved note: {parsed['title']}"
    except Exception as e:
        print(f"[-] Google API call failed: {e}")
        return f"Transcribed and parsed, but Google action failed: {e}", 200

    return result_msg, 200


if __name__ == "__main__":
    print("Pendant receiver listening on port 5000...")
    print("Recordings will be saved to:", RECORDINGS_DIR.resolve())
    app.run(host="0.0.0.0", port=5000)
