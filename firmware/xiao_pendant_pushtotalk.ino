/*
 * XIAO ESP32-S3 Sense — "AI Pendant" recorder, DEEP SLEEP version
 *
 * Press and hold the button to record, release to stop (up to
 * MAX_RECORD_SECS as a safety cap). Between uses, the chip is in deep
 * sleep -- the lowest-power state available -- rather than sitting
 * fully awake waiting for a button press. It wakes on either:
 *   - the button being pressed, or
 *   - an hourly timer, to opportunistically sync over WiFi even if
 *     the button hasn't been touched
 *
 * IMPORTANT BEHAVIOR CHANGE FROM THE ALWAYS-ON VERSION:
 * Deep sleep is closer to a full power-off than a pause. Every wake
 * is a real reboot -- setup() runs again from scratch each time,
 * re-initializing the mic, SD card, etc. This costs a small amount of
 * time (typically a few hundred ms) before recording can actually
 * start, meaning the very first instant of speech right as you press
 * the button could be clipped while the chip finishes waking up. This
 * is the real tradeoff for the big battery life improvement -- worth
 * being aware of when you test it.
 *
 * WIRING: one leg of the button to pin D0, the other leg to any GND
 * pin on the board. No resistor needed -- we use the internal pull-up
 * (reconfigured for the RTC domain specifically before each sleep, so
 * it holds reliably while the chip is off).
 *
 * BEFORE UPLOADING:
 *   1. Fill in WIFI_SSID, WIFI_PASSWORD, SERVER_URL below.
 *   2. Tools > PSRAM > Enabled
 *   3. Tools > Board > XIAO_ESP32S3 (board package 2.0.14)
 */

#include "driver/i2s.h"
#include "driver/rtc_io.h"
#include "FS.h"
#include "SD.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "time.h"
#include "esp_sleep.h"

// ---- FILL THESE IN ----
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* SERVER_URL    = "http://YOUR_PC_LOCAL_IP:5000/upload"; // e.g. http://192.168.1.50:5000/upload
// ------------------------

#define SD_CS_PIN       21
#define I2S_MIC_CLK     42
#define I2S_MIC_DATA    41
#define SAMPLE_RATE     16000
#define MAX_RECORD_SECS 60      // safety cap -- recording stops here even if button stays held
#define I2S_PORT        I2S_NUM_0
#define BUTTON_PIN      1       // D0 on the XIAO -- other leg of button goes to GND -- confirmed RTC-capable on ESP32-S3
#define WIFI_TIMEOUT_MS 10000

// ---- Periodic sync + retention settings ----
const int SYNC_INTERVAL_SEC = 3600;               // wake on its own to sync at least this often
const int SENT_FILE_RETENTION_DAYS = 7;           // auto-delete archived recordings older than this
const int MAX_UPLOAD_ATTEMPTS = 2;                // give up and delete after this many failures --
                                                    // almost always an accidental button tap

// ---- NTP time (the board has no real-time clock of its own -- this is
// fetched over the network each time WiFi connects, so file ages can be
// tracked in real calendar days rather than just relative uptime) ----
const char* NTP_SERVER = "pool.ntp.org";
const char* TZ_STRING = "EST5EDT,M3.2.0,M11.1.0"; // US Eastern, handles DST automatically
bool g_timeIsValid = false;

// Survives deep sleep (unlike normal variables) -- purely a diagnostic
// counter so the Serial log shows how many wake cycles have happened.
RTC_DATA_ATTR int bootCount = 0;

uint8_t *g_fileBuf = nullptr;
const uint32_t MAX_SAMPLES = SAMPLE_RATE * MAX_RECORD_SECS;

void writeWavHeaderToBuf(uint8_t *header, uint32_t dataSize, uint32_t sampleRate) {
  uint32_t totalDataLen = dataSize + 36;
  uint32_t byteRate = sampleRate * 1 * sizeof(int16_t);
  header[0]='R'; header[1]='I'; header[2]='F'; header[3]='F';
  header[4]=(byte)(totalDataLen); header[5]=(byte)(totalDataLen>>8);
  header[6]=(byte)(totalDataLen>>16); header[7]=(byte)(totalDataLen>>24);
  header[8]='W'; header[9]='A'; header[10]='V'; header[11]='E';
  header[12]='f'; header[13]='m'; header[14]='t'; header[15]=' ';
  header[16]=16; header[17]=0; header[18]=0; header[19]=0;
  header[20]=1; header[21]=0;
  header[22]=1; header[23]=0;
  header[24]=(byte)(sampleRate); header[25]=(byte)(sampleRate>>8);
  header[26]=(byte)(sampleRate>>16); header[27]=(byte)(sampleRate>>24);
  header[28]=(byte)(byteRate); header[29]=(byte)(byteRate>>8);
  header[30]=(byte)(byteRate>>16); header[31]=(byte)(byteRate>>24);
  header[32]=2; header[33]=0;
  header[34]=16; header[35]=0;
  header[36]='d'; header[37]='a'; header[38]='t'; header[39]='a';
  header[40]=(byte)(dataSize); header[41]=(byte)(dataSize>>8);
  header[42]=(byte)(dataSize>>16); header[43]=(byte)(dataSize>>24);
}

uint32_t getNextCounter() {
  uint32_t n = 1;
  if (SD.exists("/counter.txt")) {
    File f = SD.open("/counter.txt", FILE_READ);
    if (f) { n = f.parseInt(); f.close(); }
  }
  File f = SD.open("/counter.txt", FILE_WRITE);
  if (f) { f.print(n + 1); f.close(); }
  return n;
}

bool uploadFile(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  size_t size = f.size();
  uint8_t *buf = (uint8_t*) ps_malloc(size);
  if (!buf) { f.close(); return false; }
  f.read(buf, size);
  f.close();

  HTTPClient http;
  http.begin(SERVER_URL);
  http.setTimeout(20000);
  http.addHeader("Content-Type", "audio/wav");
  int code = http.POST(buf, size);
  free(buf);
  http.end();

  Serial.printf("  Upload %s -> HTTP %d\n", path, code);
  delay(300);
  return (code == 200);
}

bool syncNTPTime() {
  configTzTime(TZ_STRING, NTP_SERVER);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) {
    Serial.printf("[+] Time synced: %04d-%02d-%02d %02d:%02d:%02d\n",
      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return true;
  }
  Serial.println("[!] NTP time sync failed -- skipping file-age cleanup this round.");
  return false;
}

void cleanupOldSentFiles() {
  if (!g_timeIsValid) return;

  time_t now = time(nullptr);
  const time_t maxAgeSecs = (time_t)SENT_FILE_RETENTION_DAYS * 24UL * 3600UL;

  File dir = SD.open("/recordings/sent");
  if (!dir) return;
  File entry;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      entry.close();
      int sep = name.indexOf('_');
      if (sep > 0) {
        String epochStr = name.substring(0, sep);
        bool allDigits = epochStr.length() > 0;
        for (unsigned int i = 0; i < epochStr.length(); i++) {
          if (!isDigit(epochStr[i])) { allDigits = false; break; }
        }
        if (allDigits) {
          time_t fileTime = (time_t) epochStr.toInt();
          if (now - fileTime > maxAgeSecs) {
            String fullPath = String("/recordings/sent/") + name;
            if (SD.remove(fullPath)) {
              Serial.printf("[*] Deleted old archived file (%.1f days old): %s\n",
                (now - fileTime) / 86400.0, name.c_str());
            }
          }
        }
      }
    } else {
      entry.close();
    }
  }
  dir.close();
}

void syncPendingRecordings() {
  File dir = SD.open("/recordings");
  if (!dir) return;
  File entry;
  while ((entry = dir.openNextFile())) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      String fullPath = String("/recordings/") + name;
      entry.close();

      int priorFailures = 0;
      int failMarker = name.indexOf("__fail");
      if (failMarker >= 0) {
        int dotPos = name.indexOf('.', failMarker);
        String countStr = name.substring(failMarker + 6, dotPos > 0 ? dotPos : name.length());
        priorFailures = countStr.toInt();
      }

      Serial.printf("[*] Syncing pending file: %s (prior failures: %d)\n", fullPath.c_str(), priorFailures);

      if (uploadFile(fullPath.c_str())) {
        String sentPath;
        if (g_timeIsValid) {
          sentPath = String("/recordings/sent/") + String((unsigned long)time(nullptr)) + "_" + name;
        } else {
          sentPath = String("/recordings/sent/") + name;
        }
        if (SD.rename(fullPath, sentPath)) {
          Serial.println("  -> uploaded and archived.");
        } else {
          Serial.println("  -> uploaded OK, but archiving failed. Removing local copy to prevent a duplicate re-upload.");
          SD.remove(fullPath);
        }
      } else {
        int newFailCount = priorFailures + 1;
        if (newFailCount >= MAX_UPLOAD_ATTEMPTS) {
          Serial.printf("  -> upload failed %d time(s), giving up and deleting.\n", newFailCount);
          SD.remove(fullPath);
        } else {
          String baseName = (failMarker >= 0) ? name.substring(0, failMarker) : name;
          String stem, ext;
          int dotIdx = baseName.lastIndexOf('.');
          if (dotIdx >= 0) {
            stem = baseName.substring(0, dotIdx);
            ext = baseName.substring(dotIdx);
          } else {
            stem = baseName;
            ext = "";
          }
          String newPath = String("/recordings/") + stem + "__fail" + String(newFailCount) + ext;
          SD.rename(fullPath, newPath);
          Serial.printf("  -> upload failed (attempt %d of %d), will retry next time.\n", newFailCount, MAX_UPLOAD_ATTEMPTS);
        }
      }
    } else {
      entry.close();
    }
  }
  dir.close();
}

bool initI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_PIN_NO_CHANGE,
    .ws_io_num = I2S_MIC_CLK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_DATA
  };
  return (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) == ESP_OK &&
          i2s_set_pin(I2S_PORT, &pin_config) == ESP_OK);
}

// Handles a full sync cycle: connects to WiFi, syncs real time via
// NTP, uploads anything pending, cleans up old archived files.
// Assumes I2S is NOT installed when called (either never started this
// wake cycle, or already torn down) -- unlike the always-on version,
// there's no need to restore the mic afterward since we're about to
// sleep either way.
void syncNow() {
  Serial.printf("[*] Trying WiFi (timeout %ds)...\n", WIFI_TIMEOUT_MS/1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[+] WiFi connected (%s).\n", WiFi.localIP().toString().c_str());
    delay(500); // let the socket stack settle before a large transfer

    g_timeIsValid = syncNTPTime();

    Serial.println("[*] Syncing...");
    syncPendingRecordings();
    cleanupOldSentFiles();

    WiFi.disconnect(true);
  } else {
    Serial.printf("[!] WiFi failed. Status code: %d\n", WiFi.status());
    Serial.println("[!]   0=IDLE 1=NO_SSID_AVAIL 4=CONNECT_FAILED 6=DISCONNECTED");
    Serial.println("[*] No WiFi in range -- recording stays on SD, will sync later.");
  }
}

void recordAndSave() {
  int16_t *audioBuf = (int16_t *)(g_fileBuf + 44);
  uint32_t samplesGot = 0;
  size_t bytesRead = 0;

  Serial.println(">>> RECORDING (release button to stop) <<<");

  while (digitalRead(BUTTON_PIN) == LOW && samplesGot < MAX_SAMPLES) {
    size_t want = (MAX_SAMPLES - samplesGot) * sizeof(int16_t);
    if (want > 4096) want = 4096;
    esp_err_t res = i2s_read(I2S_PORT, (void*)(audioBuf + samplesGot), want, &bytesRead, pdMS_TO_TICKS(100));
    if (res == ESP_OK && bytesRead > 0) {
      samplesGot += bytesRead / sizeof(int16_t);
    }
  }

  float secs = (float)samplesGot / SAMPLE_RATE;
  Serial.printf("[+] Recording complete! %.1f seconds captured.\n", secs);

  // Done with the mic for this wake cycle -- free it before touching
  // WiFi, same lesson learned from the always-on version.
  i2s_driver_uninstall(I2S_PORT);

  if (samplesGot < SAMPLE_RATE / 2) {
    Serial.println("[*] Too short, discarding.");
    return;
  }

  uint32_t dataBytes = samplesGot * sizeof(int16_t);
  writeWavHeaderToBuf(g_fileBuf, dataBytes, SAMPLE_RATE);

  uint32_t n = getNextCounter();
  char path[40];
  snprintf(path, sizeof(path), "/recordings/rec%04u.wav", n);
  File audioFile = SD.open(path, FILE_WRITE);
  if (audioFile) {
    audioFile.write(g_fileBuf, dataBytes + 44);
    audioFile.close();
    Serial.printf("[+] Saved %s to SD card.\n", path);
  } else {
    Serial.println("[-] Failed to write file to SD!");
    return;
  }

  syncNow();
}

// Configures both wake sources and puts the chip into deep sleep.
// Never returns -- the next thing that happens is setup() running
// again from the top, as if the board had just been reset.
void goToSleep() {
  if (g_fileBuf) { free(g_fileBuf); g_fileBuf = nullptr; }

  // The regular pinMode(INPUT_PULLUP) pull-up only works while the
  // chip is awake. The RTC domain needs its own pull configuration to
  // hold the pin reliably HIGH while asleep, so a press produces a
  // clean LOW-going edge for the wake logic to catch.
  rtc_gpio_pullup_en((gpio_num_t)BUTTON_PIN);
  rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0); // wake on LOW (button pressed)
  esp_sleep_enable_timer_wakeup((uint64_t)SYNC_INTERVAL_SEC * 1000000ULL);

  Serial.printf("[*] Going to sleep. Will wake on button press or in %d minutes.\n", SYNC_INTERVAL_SEC / 60);
  Serial.flush();
  delay(50);
  esp_deep_sleep_start();
}

void setup() {
  unsigned long serialStart = millis();
  Serial.begin(115200);
  // Bounded wait, NOT "while(!Serial)" -- on battery alone with no USB
  // host attached, Serial never becomes true, and an unbounded wait
  // here would hang forever on every single wake cycle. This is the
  // one landmine that would otherwise make deep sleep useless for
  // real standalone (battery-only) operation.
  while (!Serial && millis() - serialStart < 2000) { delay(10); }

  esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
  bootCount++;

  Serial.println("\n--- AI Pendant: deep-sleep push-to-talk ---");
  Serial.printf("[*] Boot #%d, wakeup reason: %d\n", bootCount, (int)wakeupReason);
  Serial.println("[*]   0=power-on/reset  2=EXT0(button)  4=timer");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (SD.begin(SD_CS_PIN)) {
    if (!SD.exists("/recordings")) SD.mkdir("/recordings");
    if (!SD.exists("/recordings/sent")) SD.mkdir("/recordings/sent");
    Serial.println("[+] SD card ready.");

    if (wakeupReason == ESP_SLEEP_WAKEUP_EXT0) {
      Serial.println("[*] Woke up: button pressed.");
      g_fileBuf = (uint8_t *) ps_malloc(MAX_SAMPLES * sizeof(int16_t) + 44);
      if (!g_fileBuf) {
        Serial.println("[-] Failed to allocate recording buffer (check PSRAM enabled).");
      } else if (!initI2S()) {
        Serial.println("[-] I2S init failed!");
      } else {
        Serial.println("[+] Microphone ready.");
        recordAndSave(); // records, saves, and syncs -- all in one call
      }
    } else if (wakeupReason == ESP_SLEEP_WAKEUP_TIMER) {
      Serial.println("[*] Woke up: hourly timer.");
      syncNow();
    } else {
      Serial.println("[*] Fresh power-on (not from deep sleep). Doing an initial sync...");
      syncNow();
    }
  } else {
    Serial.println("[-] SD Mount Failed! Will retry next wake.");
  }

  goToSleep();
}

void loop() {
  // Never reached -- setup() always ends by calling esp_deep_sleep_start(),
  // which resets the chip rather than returning here. Arduino still
  // requires loop() to exist as a function.
}
