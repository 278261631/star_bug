# ESP32-S3-Touch-LCD-1.47 开发参考

## 硬件信息

| 项目 | 参数 |
|------|------|
| 主控 | ESP32-S3R8 (Xtensa LX7 双核, 240MHz) |
| Flash | 16MB |
| PSRAM | 8MB (OPI) |
| 屏幕 | 1.47inch IPS LCD, 172x320, 262K 色 |
| 驱动芯片 | JD9853 (兼容 ST7789 指令集) |
| 触控芯片 | AXS5106L (I2C) |

### 显示 SPI 引脚

| 功能 | GPIO |
|------|------|
| DC | 45 |
| CS | 21 |
| SCK | 38 |
| MOSI | 39 |
| RST | 47 |
| BL (背光) | 46 |

### 其他引脚 (pins_arduino.h)

| 功能 | GPIO |
|------|------|
| TX | 43 |
| RX | 44 |
| SDA | 8 |
| SCL | 9 |
| RGB LED | 38 (注意:与屏幕 SCK 共用,择一使用) |

## 烧录方法

### 方式一: Arduino IDE (GUI)

1. **安装库**: 工具 -> 管理库 -> 搜索 `GFX Library for Arduino` -> 安装
2. **选择开发板**: 工具 -> 开发板 -> ESP32 -> `Waveshare ESP32-S3-LCD-1.47`
3. **关键设置**（工具菜单）:
   - USB CDC On Boot: `Enabled`
   - PSRAM: `Enabled`
   - Flash Size: `16MB`
   - Partition Scheme: `Default 4MB with spiffs` 或 `16MB Flash (3MB APP/9.9MB FATFS)`
   - USB Mode: `Hardware CDC and JTAG` (默认)
4. **选择端口**: 对应 COM 口
5. 点击 **上传** 按钮

### 方式二: arduino-cli (命令行)

```powershell
# 安装库 (仅首次)
arduino-cli lib install "GFX Library for Arduino"

# 编译
arduino-cli compile --fqbn "esp32:esp32:waveshare_esp32_s3_lcd_147:CDCOnBoot=cdc" <项目路径>

# 上传
arduino-cli upload --fqbn "esp32:esp32:waveshare_esp32_s3_lcd_147:CDCOnBoot=cdc" --port COM<端口号> <项目路径>
```

**arduino-cli 安装位置**: `C:\Users\Administrator\AppData\Local\Microsoft\WindowsApps\arduino-cli.exe`

**ESP32 包路径**: `C:\Users\Administrator\AppData\Local\Arduino15\packages\esp32\hardware\esp32\3.3.0`

**FQBN**: `esp32:esp32:waveshare_esp32_s3_lcd_147`

## 屏幕驱动代码模板

```cpp
#include <Arduino_GFX_Library.h>

#define TFT_DC   45
#define TFT_CS   21
#define TFT_SCK  38
#define TFT_MOSI 39
#define TFT_RST  47
#define TFT_BL   46

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI);
Arduino_GFX *gfx = new Arduino_ST7789(
    bus, TFT_RST, 4 /* rotation */, false /* IPS */,
    172 /* width */, 320 /* height */,
    34 /* col_offset1 */, 0 /* row_offset1 */,
    34 /* col_offset2 */, 0 /* row_offset2 */);

void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  gfx->begin();
  gfx->fillScreen(RGB565_BLACK);
}
```

### rotation 参数说明

- `0`: 默认 (文字会左右镜像)
- `4`: 正常方向 (MX=1, 修复镜像)

颜色常量使用 `RGB565_` 前缀，如 `RGB565_RED`, `RGB565_GREEN`, `RGB565_BLACK`, `RGB565_WHITE` 等。

## 已知问题 & 注意事项

### 1. 串口监视器问题
Windows 打开 COM 口时会触发 DTR/RTS 信号，导致 ESP32-S3 进入下载模式 (boot:0x22)，**无法正常读取程序串口输出**。这是 ESP32 开发板的常见问题，不影响程序运行。如果必须查看串口输出，需使用支持不触发 DTR/RTS 的串口工具。

### 2. 重烧录失败
如果上传失败，按住 BOOT 键，按一下 RESET（或重新上电），然后松开 BOOT，让芯片进入下载模式后再烧录。

### 3. 屏幕镜像
rotation=0 时文字左右镜像，这是因为 JD9853 与 ST7789 的列地址方向不同。解决方案是设置 rotation=4（MX=1 翻转列地址）。

### 4. GPIO38 冲突
GPIO38 同时用于屏幕 SCK 和板载 RGB LED (`PIN_RGB_LED`)，两者不能同时使用。

## 资源链接

- [Waveshare Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.47)
- [Arduino_GFX 库](https://github.com/moononournation/Arduino_GFX)
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [Demo 下载](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.47/ESP32-S3-Touch-LCD-1.47-Demo.zip)
- [原理图](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.47/ESP32-S3-Touch-LCD-1.47-Schematic.pdf)
