#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <M5Unified.h>

static const char* SETUP_SSID = "FART-MACHINE-SETUP";
static const char* SETUP_PASS = "prutt1234";
static constexpr uint32_t SAMPLE_RATE = 16000;
static constexpr size_t MAX_SAMPLES = SAMPLE_RATE * 3;
static int16_t audioBuffer[MAX_SAMPLES];

WebServer server(80);
Preferences prefs;
String savedSsid, savedPass;
bool setupMode = false;
String statusLine = "Starting...";
uint32_t lastDisplay = 0;

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="sv"><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>FART MACHINE ADV</title>
<style>
body{margin:0;background:#111820;color:#f4f7fa;font-family:Arial,sans-serif;text-align:center}
main{max-width:520px;margin:auto;padding:22px}
h1{font-size:30px;margin:12px 0 4px} .sub{color:#9eacb8;margin-bottom:22px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:12px}
button{border:0;border-radius:16px;padding:22px 8px;font-size:20px;font-weight:bold;background:#263746;color:white;min-height:90px}
button:active{transform:scale(.97);background:#3a596d}
small{display:block;font-size:12px;font-weight:normal;color:#b8c7d2;margin-top:7px}
.note{color:#9eacb8;font-size:13px;margin-top:22px}
</style></head><body><main>
<div style="font-size:54px">💨</div><h1>FART MACHINE ADV</h1><div class="sub">Cardputer-Adv remote control</div>
<div class="grid">
<button onclick="fire('classic')">FART<small>Classic</small></button>
<button onclick="fire('mega')">MEGA FART<small>Long blast</small></button>
<button onclick="fire('random')">RANDOM<small>Surprise me</small></button>
<button onclick="fire('burst')">BURST<small>Three in a row</small></button>
</div>
<p id="msg" class="note">Ready to fire.</p>
<p class="note">Ansluten via ditt hem-Wi-Fi.</p>
</main><script>
async function fire(t){let m=document.getElementById('msg');m.textContent='💨 Firing...';
try{let r=await fetch('/play?type='+encodeURIComponent(t));m.textContent=await r.text();}
catch(e){m.textContent='Kunde inte nå Cardputern.';}}
</script></body></html>
)HTML";

const char SETUP_PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="sv"><head><meta name="viewport" content="width=device-width,initial-scale=1">
<title>FART MACHINE Wi-Fi</title><style>
body{font-family:Arial;background:#111820;color:#fff;padding:24px;max-width:480px;margin:auto}
input{box-sizing:border-box;width:100%;padding:13px;margin:8px 0 16px;border-radius:9px;border:0;font-size:16px}
button{width:100%;padding:15px;border:0;border-radius:10px;background:#3b8b63;color:white;font-size:18px;font-weight:bold}
</style></head><body><h1>💨 FART MACHINE</h1><h2>Wi-Fi setup</h2>
<p>Ange namnet och lösenordet till ditt hem-Wi-Fi.</p>
<form method="POST" action="/save">
<label>Wi-Fi-namn (SSID)</label><input name="ssid" required maxlength="32">
<label>Wi-Fi-lösenord</label><input name="pass" type="password" maxlength="64">
<button type="submit">Spara och anslut</button></form></body></html>
)HTML";

void showStatus(const String& line) {
  statusLine = line;
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(8, 8);
  M5.Display.println("FART MACHINE");
  M5.Display.setTextSize(1);
  M5.Display.println();
  M5.Display.println(line);
  M5.Display.println();
  if (setupMode) {
    M5.Display.println("Wi-Fi setup AP:");
    M5.Display.println(SETUP_SSID);
    M5.Display.println("Password: prutt1234");
    M5.Display.println("Open: 192.168.4.1");
  } else {
    M5.Display.println("Home Wi-Fi connected");
    M5.Display.println(WiFi.localIP().toString());
    M5.Display.println();
    M5.Display.println("Open IP in your phone");
  }
}

uint32_t rngState = 0xA53C91E7;
float rand01() {
  rngState = 1664525UL * rngState + 1013904223UL;
  return (float)((rngState >> 8) & 0xFFFFFF) / 16777215.0f;
}

// Procedurally synthesized, original comic sound effects. No external audio files required.
void makeFart(int kind, float durationSeconds) {
  size_t count = min((size_t)(SAMPLE_RATE * durationSeconds), MAX_SAMPLES);
  float phase = 0.0f;
  float phase2 = 0.0f;
  float baseFreq = (kind == 1) ? 58.0f : (kind == 2 ? 43.0f : 72.0f);
  rngState = 0xA53C91E7u + kind * 9871u + millis();
  for (size_t i = 0; i < count; ++i) {
    float t = (float)i / SAMPLE_RATE;
    float progress = (float)i / (float)count;
    float envelope = sinf(3.1415926f * powf(progress, 0.72f));
    envelope = powf(max(0.0f, envelope), 0.65f);
    float wobble = 1.0f + 0.34f * sinf(2.0f * 3.1415926f * (kind == 2 ? 4.2f : 7.0f) * t);
    float freq = baseFreq * wobble + 13.0f * sinf(t * 17.0f);
    phase += 2.0f * 3.1415926f * freq / SAMPLE_RATE;
    phase2 += 2.0f * 3.1415926f * (freq * 1.47f) / SAMPLE_RATE;
    float buzz = sinf(phase) * 0.62f + sinf(phase2) * 0.18f;
    float noise = (rand01() * 2.0f - 1.0f);
    float rasp = noise * (kind == 2 ? 0.48f : 0.30f);
    float pulses = 0.72f + 0.28f * sinf(2.0f * 3.1415926f * (kind == 3 ? 9.0f : 3.3f) * t);
    float sample = (buzz + rasp) * envelope * pulses;
    if (kind == 3) {
      // Burst: three distinct short blasts separated by gaps.
      float local = fmodf(t, 0.42f);
      float gate = (local < 0.27f) ? 1.0f : 0.0f;
      sample *= gate;
    }
    if (kind == 4) {
      // Random style: a brighter, more sputtery tone.
      sample = (0.45f * sinf(phase * 1.8f) + 0.55f * noise) * envelope;
    }
    int32_t v = (int32_t)(sample * 21000.0f);
    audioBuffer[i] = (int16_t)constrain(v, -30000, 30000);
  }
  M5.Speaker.stop();
  M5.Speaker.playRaw(audioBuffer, count, SAMPLE_RATE, false, 1, 0);
}

void handleRoot() {
  if (setupMode) server.send_P(200, "text/html; charset=utf-8", SETUP_PAGE);
  else server.send_P(200, "text/html; charset=utf-8", PAGE);
}

void handlePlay() {
  if (setupMode) { server.send(503, "text/plain", "Wi-Fi is not configured yet."); return; }
  String type = server.arg("type");
  if (type == "classic") { makeFart(1, 1.25f); server.send(200, "text/plain", "Classic fart fired!"); }
  else if (type == "mega") { makeFart(2, 2.8f); server.send(200, "text/plain", "MEGA FART fired!"); }
  else if (type == "burst") { makeFart(3, 2.3f); server.send(200, "text/plain", "Burst fired!"); }
  else { makeFart(4, 1.7f); server.send(200, "text/plain", "Random fart fired!"); }
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.isEmpty()) { server.send(400, "text/plain", "SSID saknas."); return; }
  prefs.begin("fartwifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  server.send(200, "text/html; charset=utf-8",
    "<html><meta name='viewport' content='width=device-width,initial-scale=1'><body style='font-family:Arial;padding:24px'><h2>💨 Sparat!</h2><p>Cardputern startar om och ansluter till ditt Wi-Fi.</p></body></html>");
  delay(900);
  ESP.restart();
}

void startSetupAP() {
  setupMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SETUP_SSID, SETUP_PASS);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  showStatus("Wi-Fi setup active");
}

bool connectHomeWiFi() {
  prefs.begin("fartwifi", true);
  savedSsid = prefs.getString("ssid", "");
  savedPass = prefs.getString("pass", "");
  prefs.end();
  if (savedSsid.isEmpty()) return false;

  setupMode = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());
  showStatus("Connecting to " + savedSsid);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 18000) {
    M5.update();
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) return false;

  server.on("/", HTTP_GET, handleRoot);
  server.on("/play", HTTP_GET, handlePlay);
  server.begin();
  showStatus("Ready! " + WiFi.localIP().toString());
  return true;
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Speaker.begin();
  M5.Speaker.setVolume(210);
  delay(300);

  if (!connectHomeWiFi()) startSetupAP();
}

void loop() {
  M5.update();
  server.handleClient();

  // Cardputer keyboard shortcut: press Enter/Space to fire a random sound.
  if (M5.BtnA.wasPressed()) makeFart(4, 1.6f);

  if (!setupMode && WiFi.status() != WL_CONNECTED) {
    server.stop();
    if (!connectHomeWiFi()) startSetupAP();
  }
  delay(2);
}
