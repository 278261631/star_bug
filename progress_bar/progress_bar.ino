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

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, TFT_RST, 4, false,
    TFT_WIDTH, TFT_HEIGHT,
    34, 0, 34, 0);

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

void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);
}

void loop() {
  unsigned long now = millis();
  float progress = (float)(now % (CYCLE_SECONDS * 1000UL)) / (float)(CYCLE_SECONDS * 1000UL);
  float segProgress = progress * NUM_SEGMENTS;
  int colorIdx = (now / COLOR_INTERVAL) % THEME_COUNT;

  for (int i = 0; i < NUM_SEGMENTS; i++) {
    int x = startX + i * (SEG_WIDTH + SEG_GAP);
    float fill = constrain(segProgress - i, 0.0f, 1.0f);
    int fillHeight = (int)(fill * SEG_HEIGHT);

    gfx->drawRect(x, startY, SEG_WIDTH, SEG_HEIGHT, RGB565_WHITE);
    gfx->fillRect(x, startY + SEG_HEIGHT - fillHeight,
                  SEG_WIDTH, fillHeight, themes[colorIdx][i]);
  }

  gfx->setCursor(0, startY + SEG_HEIGHT + 12);
  gfx->setTextSize(1);

  static int lastColorIdx = -1;
  if (colorIdx != lastColorIdx) {
    lastColorIdx = colorIdx;
    gfx->fillRect(0, startY + SEG_HEIGHT + 10, TFT_WIDTH, 30, RGB565_BLACK);
  }

  gfx->setTextColor(themes[colorIdx][0]);
  gfx->print("Theme:");

  int pct = (int)(progress * 100);
  gfx->setTextColor(RGB565_WHITE);
  gfx->print(" ");
  gfx->print(pct);
  gfx->print("%");

  delay(100);
}
