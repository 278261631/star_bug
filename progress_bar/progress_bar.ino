#include <WiFi.h>
#include <WebServer.h>
#include <Arduino_GFX_Library.h>

#define TFT_DC   45
#define TFT_CS   21
#define TFT_SCK  38
#define TFT_MOSI 39
#define TFT_RST  47
#define TFT_BL   46

#define TFT_WIDTH  172
#define TFT_HEIGHT 320

#define NUM_SEGMENTS   6
#define SEG_WIDTH      22
#define SEG_GAP        4
#define SEG_HEIGHT     180
#define CYCLE_SECONDS  60
#define COLOR_INTERVAL 10000

#define BL_PWM_FREQ    5000
#define BL_PWM_RES     8

#define WIFI_SSID "ProgressBar-ESP32"

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, TFT_RST, 4, false,
    TFT_WIDTH, TFT_HEIGHT,
    34, 0, 34, 0);

WebServer server(80);

const uint16_t themes[][NUM_SEGMENTS] = {
  { RGB565_RED, RGB565_ORANGE, RGB565_YELLOW, RGB565_GREEN, RGB565_CYAN, RGB565_BLUE },
  { RGB565_MAGENTA, RGB565_PURPLE, RGB565_BLUE, RGB565_CYAN, RGB565_GREEN, RGB565_OLIVE },
  { RGB565_YELLOW, RGB565_ORANGE, RGB565_RED, RGB565_MAGENTA, RGB565_PURPLE, RGB565_BLUE },
  { RGB565_CYAN, RGB565_GREEN, RGB565_YELLOW, RGB565_ORANGE, RGB565_RED, RGB565_MAGENTA },
  { RGB565_WHITE, RGB565_CYAN, RGB565_GREEN, RGB565_YELLOW, RGB565_ORANGE, RGB565_RED },
  { RGB565_BLUE, RGB565_MAGENTA, RGB565_RED, RGB565_ORANGE, RGB565_YELLOW, RGB565_GREEN },
};
const int THEME_COUNT = sizeof(themes) / sizeof(themes[0]);

const int totalWidth = NUM_SEGMENTS * SEG_WIDTH + (NUM_SEGMENTS - 1) * SEG_GAP;
const int startX = (TFT_WIDTH - totalWidth) / 2;
const int startY = (TFT_HEIGHT - SEG_HEIGHT) / 2;

unsigned long cycleStart = 0;
uint8_t brightness = 255;
bool wifiStarted = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Control</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,system-ui,sans-serif;background:#1a1a2e;color:#eee;display:flex;justify-content:center;align-items:center;min-height:100vh;padding:20px}
.card{background:#16213e;border-radius:16px;padding:28px 24px;width:100%;max-width:360px;text-align:center;box-shadow:0 8px 32px rgba(0,0,0,.4)}
h1{font-size:20px;margin-bottom:24px;color:#e94560}
.row{display:flex;align-items:center;justify-content:space-between;margin-bottom:20px;gap:12px}
.row label{font-size:14px;color:#aaa;white-space:nowrap}
.row span{font-size:14px;color:#e94560;min-width:40px;text-align:right}
input[type=range]{-webkit-appearance:none;flex:1;height:8px;border-radius:4px;background:#0f3460;outline:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;border-radius:50%;background:#e94560;cursor:pointer;border:2px solid #fff}
.btn{width:100%;padding:14px;border:none;border-radius:10px;font-size:16px;font-weight:600;cursor:pointer;transition:all .2s}
.btn-reset{background:#e94560;color:#fff}
.btn-reset:hover{background:#c73652}
.btn-reset:active{transform:scale(.97)}
</style>
</head>
<body>
<div class="card">
<h1>Screen Control</h1>
<div class="row">
<label>Brightness</label>
<input type="range" id="bri" min="10" max="255" value="255" oninput="setBri(this.value)">
<span id="briVal">---</span>
</div>
<button class="btn btn-reset" onclick="reset()">Reset</button>
</div>
<script>
function setBri(v){fetch('/brightness?value='+v);document.getElementById('briVal').textContent=v}
function reset(){fetch('/reset')}
fetch('/status').then(r=>r.json()).then(d=>{
document.getElementById('bri').value=d.bri;
document.getElementById('briVal').textContent=d.bri;
})
</script>
</body>
</html>
)rawliteral";

void handleRoot() { server.send(200, "text/html", index_html); }
void handleNotFound() { server.send(404, "text/plain", "Not Found"); }

void handleReset() {
  cycleStart = millis();
  server.send(200, "text/plain", "OK");
}

void handleBrightness() {
  if (server.hasArg("value")) {
    brightness = server.arg("value").toInt();
    brightness = constrain(brightness, (uint8_t)10, (uint8_t)255);
    static bool ledcOk = false;
    if (!ledcOk) {
      ledcOk = ledcAttach(TFT_BL, BL_PWM_FREQ, BL_PWM_RES);
    }
    if (ledcOk) {
      ledcWrite(TFT_BL, brightness);
    } else {
      digitalWrite(TFT_BL, brightness > 128 ? HIGH : LOW);
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  char json[32];
  snprintf(json, sizeof(json), "{\"bri\":%d}", brightness);
  server.send(200, "application/json", json);
}

void startWiFi() {
  gfx->setCursor(0, 0);
  gfx->setTextColor(RGB565_YELLOW);
  gfx->print("WiFi...");

  delay(10);
  WiFi.mode(WIFI_AP);
  delay(10);
  WiFi.softAP(WIFI_SSID);
  delay(10);

  IPAddress ip = WiFi.softAPIP();

  server.on("/", handleRoot);
  server.on("/reset", handleReset);
  server.on("/brightness", handleBrightness);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();

  gfx->fillRect(0, 0, TFT_WIDTH, 20, RGB565_BLACK);
  gfx->setCursor(0, 0);
  gfx->setTextColor(RGB565_GREEN);
  gfx->print("AP:");
  gfx->print(WIFI_SSID);
  gfx->setCursor(0, 10);
  gfx->print("IP:");
  gfx->print(ip.toString());

  wifiStarted = true;
}

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);

  cycleStart = millis();
}

void draw(unsigned long now) {
  unsigned long elapsed = now - cycleStart;
  float progress = (float)(elapsed % (CYCLE_SECONDS * 1000UL)) / (float)(CYCLE_SECONDS * 1000UL);
  float segProgress = progress * NUM_SEGMENTS;
  int colorIdx = (elapsed / COLOR_INTERVAL) % THEME_COUNT;

  for (int i = 0; i < NUM_SEGMENTS; i++) {
    int x = startX + i * (SEG_WIDTH + SEG_GAP);
    float fill = constrain(segProgress - i, 0.0f, 1.0f);
    int fillHeight = (int)(fill * SEG_HEIGHT);
    int emptyHeight = SEG_HEIGHT - fillHeight;

    if (emptyHeight > 0) {
      gfx->fillRect(x, startY, SEG_WIDTH, emptyHeight, RGB565_BLACK);
    }
    if (fillHeight > 0) {
      gfx->fillRect(x, startY + emptyHeight, SEG_WIDTH, fillHeight, themes[colorIdx][i]);
    }
    gfx->drawRect(x, startY, SEG_WIDTH, SEG_HEIGHT, RGB565_WHITE);
  }

  gfx->setCursor(0, startY + SEG_HEIGHT + 12);
  gfx->setTextSize(1);

  if (!wifiStarted) {
    gfx->setTextColor(RGB565_YELLOW);
    gfx->print("WiFi starting...");
  } else {
    static int lastColorIdx = -1;
    if (colorIdx != lastColorIdx) {
      lastColorIdx = colorIdx;
      gfx->fillRect(0, startY + SEG_HEIGHT + 10, TFT_WIDTH, 30, RGB565_BLACK);
    }
    gfx->setTextColor(themes[colorIdx][0]);
    gfx->print("T:");
    gfx->print(colorIdx + 1);
    gfx->print(" BRI:");
    gfx->print(brightness);
  }

  int pct = (int)(progress * 100);
  gfx->setCursor(0, startY + SEG_HEIGHT + 26);
  gfx->setTextColor(RGB565_WHITE);
  gfx->print(pct);
  gfx->print("%");
}

void loop() {
  static bool wifiInitDone = false;

  if (!wifiInitDone) {
    if (millis() > 2000) {
      wifiInitDone = true;
      startWiFi();
    }
  }

  if (wifiStarted) {
    server.handleClient();
  }

  unsigned long now = millis();
  static unsigned long lastDraw = 0;
  if (now - lastDraw >= 100) {
    lastDraw = now;
    draw(now);
  }
}
