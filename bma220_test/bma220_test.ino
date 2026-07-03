/*
 * ESP32-S3-Touch-LCD-1.47 BMA220 加速度计状态显示
 *
 * BMA220: Bosch 三轴数字加速度计 (I2C 地址 0x0A)
 * 数据精度: 6-bit/轴, 默认量程 ±2g (56 LSB/g, 零偏=32)
 *
 * 硬件连接:
 *   SDA -> GPIO8
 *   SCL -> GPIO9
 */

#include <Wire.h>
#include <Arduino_GFX_Library.h>

#define TFT_DC   45
#define TFT_CS   21
#define TFT_SCK  38
#define TFT_MOSI 39
#define TFT_RST  47
#define TFT_BL   46

#define TFT_WIDTH  172
#define TFT_HEIGHT 320

#define I2C_SDA 42
#define I2C_SCL 41

/* BMA220 I2C 地址 (7-bit) */
#define BMA220_ADDR 0x0A

/* BMA220 寄存器 */
#define BMA220_CHIP_ID     0x00
#define BMA220_REVISION    0x01
#define BMA220_ACC_X       0x02
#define BMA220_ACC_Y       0x03
#define BMA220_ACC_Z       0x04
#define BMA220_BW_RANGE    0x14
#define BMA220_SOFTRESET   0x15
#define BMA220_SUSPEND     0x16

#define EXPECTED_CHIP_ID 0xDD

/* 各量程灵敏度 (LSB/g) */
const float SENSITIVITY[] = { 56.0f, 28.0f, 14.0f, 7.0f };
const float RANGE_MAX[]   = { 2.0f,  4.0f,  8.0f,  16.0f };

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, TFT_RST, 4, false,
    TFT_WIDTH, TFT_HEIGHT,
    34, 0, 34, 0);

bool   sensorOk = false;
uint8_t chipId  = 0;
uint8_t rev     = 0;
uint8_t range   = 0;
uint8_t bw      = 0;

float x_g = 0, y_g = 0, z_g = 0;

/* ---------- BMA220 底层 I2C 读写 ---------- */

bool bma220Read(uint8_t reg, uint8_t *data, uint8_t len = 1) {
  Wire.beginTransmission(BMA220_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) return false;
  Wire.requestFrom(BMA220_ADDR, len);
  for (uint8_t i = 0; i < len; i++) {
    if (Wire.available()) data[i] = Wire.read();
    else return false;
  }
  return true;
}

bool bma220Write(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(BMA220_ADDR);
  Wire.write(reg);
  Wire.write(data);
  return Wire.endTransmission() == 0;
}

/* ---------- BMA220 初始化 ---------- */

bool bma220Init() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  /* 软复位 */
  bma220Write(BMA220_SOFTRESET, 0xB6);
  delay(10);

  /* 读取芯片 ID */
  if (!bma220Read(BMA220_CHIP_ID, &chipId)) return false;
  if (chipId != EXPECTED_CHIP_ID) return false;

  /* 读取版本号 */
  bma220Read(BMA220_REVISION, &rev);

  /* 读取带宽和量程 */
  uint8_t bwRange;
  bma220Read(BMA220_BW_RANGE, &bwRange);
  range = bwRange & 0x03;
  bw    = (bwRange >> 2) & 0x03;

  /* 退出挂起模式 */
  bma220Write(BMA220_SUSPEND, 0x00);

  return true;
}

/* 将原始数据转换为 g 值 (零偏 = 32) */
float rawToG(uint8_t raw) {
  return ((int16_t)raw - 32) / SENSITIVITY[range];
}

/* ---------- 绘制函数 ---------- */

void drawBar(int y, const char *label, float value, uint16_t color) {
  int barW = TFT_WIDTH - 24;
  int barH = 14;
  int barX = 12;
  int barY = y + 2;
  int mid   = barX + barW / 2;

  /* 标签 + 数值 */
  gfx->setTextColor(color);
  gfx->setCursor(4, y);
  gfx->printf("%s:%+7.3f g", label, value);

  /* 背景条 */
  gfx->drawRect(barX, barY, barW, barH, gfx->color565(48, 48, 48));

  /* 中心零线 */
  gfx->drawFastVLine(mid, barY - 2, barH + 4, RGB565_WHITE);

  /* 填充条 */
  float maxG  = RANGE_MAX[range];
  float clamped = constrain(value, -maxG, maxG);
  int fillW = (int)(fabsf(clamped) / maxG * (barW / 2));
  if (clamped >= 0) {
    gfx->fillRect(mid, barY + 2, fillW, barH - 4, color);
  } else {
    gfx->fillRect(mid - fillW, barY + 2, fillW, barH - 4, color);
  }
}

void drawInfoBar(int y) {
  const char *bwNames[] = { "32Hz", "64Hz", "128Hz", "256Hz" };
  gfx->setTextColor(RGB565_WHITE);
  gfx->setCursor(4, y);
  gfx->printf("ID:0x%02X Rev:%02X  BW:%s  R:%+dG",
              chipId, rev, bwNames[bw], (int)RANGE_MAX[range]);
}

void drawHeader() {
  gfx->fillRect(0, 0, TFT_WIDTH, 28, gfx->color565(8, 8, 24));
  gfx->setTextColor(RGB565_CYAN);
  gfx->setTextSize(1);
  gfx->setCursor(4, 4);
  gfx->print("BMA220 Accel Monitor");
  gfx->drawFastHLine(0, 28, TFT_WIDTH, gfx->color565(0, 64, 128));
}

/* ---------- Arduino 入口 ---------- */

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);

  /* 初始化提示 */
  gfx->setTextColor(RGB565_WHITE);
  gfx->setCursor(4, 4);
  gfx->print("BMA220 Init...");

  sensorOk = bma220Init();

  if (sensorOk) {
    Serial.println("BMA220 OK!");
    Serial.printf("  Chip ID : 0x%02X\n", chipId);
    Serial.printf("  Revision: 0x%02X\n", rev);
    Serial.printf("  Range   : +-%d g\n", (int)RANGE_MAX[range]);
    Serial.printf("  BW      : %s\n", bw == 3 ? "256Hz" : bw == 2 ? "128Hz" : bw == 1 ? "64Hz" : "32Hz");

    gfx->setTextColor(RGB565_GREEN);
    gfx->setCursor(4, 20);
    gfx->printf("OK! ID:0x%02X +-%dG", chipId, (int)RANGE_MAX[range]);
    delay(1500);
  } else {
    Serial.println("BMA220 NOT FOUND!");
    gfx->setTextColor(RGB565_RED);
    gfx->setCursor(4, 20);
    gfx->print("BMA220 FAIL!");
    delay(3000);
  }

  gfx->fillScreen(RGB565_BLACK);
}

void loop() {
  if (!sensorOk) {
    gfx->fillScreen(RGB565_BLACK);
    gfx->setTextColor(RGB565_RED);
    gfx->setTextSize(2);
    gfx->setCursor(10, TFT_HEIGHT / 2 - 20);
    gfx->println("BMA220");
    gfx->setCursor(30, TFT_HEIGHT / 2 + 10);
    gfx->println("NOT FOUND");
    delay(2000);
    return;
  }

  /* 读取三轴加速度 */
  uint8_t rawX, rawY, rawZ;
  bma220Read(BMA220_ACC_X, &rawX);
  bma220Read(BMA220_ACC_Y, &rawY);
  bma220Read(BMA220_ACC_Z, &rawZ);

  x_g = rawToG(rawX);
  y_g = rawToG(rawY);
  z_g = rawToG(rawZ);

  /* 绘制屏幕 */
  gfx->fillScreen(RGB565_BLACK);
  drawHeader();

  drawBar(52,  "X", x_g, RGB565_RED);
  drawBar(105, "Y", y_g, RGB565_GREEN);
  drawBar(158, "Z", z_g, RGB565_BLUE);

  drawInfoBar(215);

  /* 底部辅助信息 */
  gfx->setTextColor(RGB565_DARKGREY);
  gfx->setCursor(4, 235);
  gfx->printf("Raw: X=%02X Y=%02X Z=%02X", rawX, rawY, rawZ);

  /* 加速度矢量大小 */
  float mag = sqrtf(x_g * x_g + y_g * y_g + z_g * z_g);
  gfx->setCursor(4, 250);
  gfx->printf("|a| = %.3f g", mag);

  /* 串口输出 */
  Serial.printf("X:%+7.3f  Y:%+7.3f  Z:%+7.3f  |a|:%.3f g\n", x_g, y_g, z_g, mag);

  delay(200);
}
