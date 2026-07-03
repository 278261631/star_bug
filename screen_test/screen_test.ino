/*
 * ESP32-S3-Touch-LCD-1.47 屏幕测试程序
 * 
 * 使用前请先在 Arduino IDE 中安装库:
 *   工具 -> 管理库 -> 搜索 "GFX Library for Arduino" -> 安装 (作者: Moon On Our Nation)
 * 
 * 开发板选择:
 *   工具 -> 开发板 -> ESP32 -> Waveshare ESP32-S3-LCD-1.47
 * 
 * 关键设置:
 *   USB Mode: USB-OTG (TinyUSB)
 *   USB CDC On Boot: Enabled
 *   PSRAM: Enabled
 *   Flash Size: 16MB
 */

#include <Arduino_GFX_Library.h>

#define TFT_DC   45
#define TFT_CS   21
#define TFT_SCK  38
#define TFT_MOSI 39
#define TFT_RST  47
#define TFT_BL   46

#define TFT_WIDTH  172
#define TFT_HEIGHT 320

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, TFT_RST, 4, false,
    TFT_WIDTH, TFT_HEIGHT,
    34, 0, 34, 0);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3-Touch-LCD-1.47 Screen Test");

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(RGB565_WHITE);
  delay(500);

  Serial.println("Test 1: Color fill");
  gfx->fillScreen(RGB565_RED);
  delay(600);
  gfx->fillScreen(RGB565_GREEN);
  delay(600);
  gfx->fillScreen(RGB565_BLUE);
  delay(600);
  gfx->fillScreen(RGB565_BLACK);
  delay(400);

  Serial.println("Test 2: Color bars");
  uint16_t colors[] = {
    RGB565_RED, RGB565_ORANGE, RGB565_YELLOW, RGB565_GREEN,
    RGB565_CYAN, RGB565_BLUE, RGB565_MAGENTA, RGB565_WHITE
  };
  int bar_h = TFT_HEIGHT / 8;
  for (int i = 0; i < 8; i++) {
    gfx->fillRect(0, i * bar_h, TFT_WIDTH, bar_h, colors[i]);
  }
  delay(1500);

  Serial.println("Test 3: Text");
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 20);
  gfx->println("ESP32-S3");
  gfx->setCursor(10, 50);
  gfx->println("Touch LCD");
  gfx->setCursor(10, 80);
  gfx->println("1.47 inch");
  gfx->setTextSize(1);
  gfx->setCursor(10, 120);
  gfx->println("Waveshare");
  gfx->setCursor(10, 140);
  gfx->println("172x320 IPS");
  delay(1500);

  Serial.println("Test 4: Shapes");
  gfx->fillScreen(RGB565_NAVY);
  gfx->fillRect(10, 10, 60, 40, RGB565_RED);
  gfx->fillRect(90, 10, 60, 40, RGB565_GREEN);
  gfx->fillRect(10, 60, 60, 40, RGB565_BLUE);
  gfx->fillRect(90, 60, 60, 40, RGB565_YELLOW);
  gfx->drawRect(10, 120, 60, 40, RGB565_WHITE);
  gfx->drawRect(90, 120, 60, 40, RGB565_CYAN);
  gfx->fillCircle(40, 200, 20, RGB565_MAGENTA);
  gfx->fillCircle(110, 200, 20, RGB565_ORANGE);
  gfx->drawCircle(40, 260, 20, RGB565_WHITE);
  gfx->drawCircle(110, 260, 20, RGB565_WHITE);
  delay(2000);

  Serial.println("Test 5: Gradient");
  for (int y = 0; y < TFT_HEIGHT; y++) {
    uint8_t r = map(y, 0, TFT_HEIGHT, 0, 255);
    uint8_t g = map(y, 0, TFT_HEIGHT, 255, 0);
    uint8_t b = 128;
    gfx->drawFastHLine(0, y, TFT_WIDTH, gfx->color565(r, g, b));
  }
  delay(1500);

  Serial.println("Test 6: Random pixels");
  gfx->fillScreen(RGB565_BLACK);
  for (int i = 0; i < 500; i++) {
    int x = random(TFT_WIDTH);
    int y = random(TFT_HEIGHT);
    gfx->drawPixel(x, y, gfx->color565(random(256), random(256), random(256)));
  }
  delay(1500);

  Serial.println("All tests completed!");
}

void loop() {
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, TFT_HEIGHT / 2 - 20);
  gfx->println("Screen OK!");
  delay(2000);

  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_GREEN);
  gfx->setTextSize(2);
  gfx->setCursor(20, TFT_HEIGHT / 2 - 50);
  gfx->println("172 x 320");
  gfx->setCursor(10, TFT_HEIGHT / 2 - 10);
  gfx->println("IPS LCD");
  delay(2000);
}
