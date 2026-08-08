/*
 * ESP32-S3 硬件自检固件 - LCD 点亮测试(TFT_eSPI)
 * -----------------------------------------------------------------
 * 目的: 循环填充几种纯色 + 显示文字 + 画一个不断变化的图形,
 *       用肉眼彻底确认屏幕能不能正常显示画面。
 *
 * 引脚配置在 platformio.ini 的 build_flags 里(TFT_eSPI 的标准用法,
 * 不需要改库文件),这里的代码本身不需要关心具体引脚。
 *
 * 用法: 先烧 lcd_st7789 环境;如果花屏/白屏/颜色不对,再改烧 lcd_ili9341 环境
 * (VSCode 左下角或 PlatformIO 图标栏可以切换 env,或者命令行:
 *    pio run -e lcd_st7789 -t upload
 *    pio run -e lcd_ili9341 -t upload )
 */

#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  delay(200);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" ESP32-S3 硬件自检 - LCD 点亮测试(TFT_eSPI)");
  Serial.println("========================================");

  tft.init();
  tft.setRotation(1); // 横屏,不习惯的话改成 0~3 试试

  Serial.println("tft.init() 完成,开始颜色填充测试...");
}

const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_WHITE, TFT_BLACK};
const char *colorNames[] = {"RED", "GREEN", "BLUE", "WHITE", "BLACK"};
const int numColors = sizeof(colors) / sizeof(colors[0]);
int colorIdx = 0;

unsigned long lastSwitch = 0;
uint32_t frame = 0;

void loop() {
  if (millis() - lastSwitch > 1000) {
    lastSwitch = millis();

    tft.fillScreen(colors[colorIdx]);
    Serial.printf("[%8lu ms] 填充颜色: %s  (屏幕应该整个变成这个颜色)\n",
                  millis(), colorNames[colorIdx]);
    colorIdx = (colorIdx + 1) % numColors;

    // 文字颜色跟着背景色反一下,保证在任何背景色下都看得清
    tft.setTextColor(TFT_YELLOW, colors[(colorIdx + numColors - 1) % numColors]);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("ESP32-S3 LCD OK");
    tft.setCursor(10, 40);
    tft.printf("frame #%lu", (unsigned long)frame);

    // 画一个随时间移动的圆,进一步确认刷新是实时的、不是卡死的静态画面
    int cx = 60 + (frame * 13) % (tft.width() - 120);
    int cy = tft.height() / 2 + 40;
    tft.fillCircle(cx, cy, 15, TFT_ORANGE);

    frame++;
  }
}
