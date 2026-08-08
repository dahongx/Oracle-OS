/*
 * ESP32-S3 硬件自检固件 - LCD 原始 SPI 通信测试(不依赖任何显示库)
 * -----------------------------------------------------------------
 * 目的: 在还没配置 TFT_eSPI 之类的显示库之前,先用最原始的方式发一个
 *       "Read Display ID"(命令 0x04)命令,看屏幕控制器有没有响应。
 *       这样能把"SPI 接线/时序对不对"和"显示库配置对不对"这两件事分开排查。
 *
 * 根据数据手册,这块 2.8寸屏是 ST7789V 或 ILI9341V 二选一(看具体贴片的是哪颗芯片),
 * 4-line SPI 接口,信号名: VCC GND CS RESET DC SDI(MOSI) SCK LED SDO(MISO)
 *
 * 引脚已根据原理图(MCU.SchDoc)核实,LCD 走的是软件 SPI(几个 GPIO 都不是 S3 的硬件
 * SPI 默认脚,但 ESP32 的 SPI 外设可以通过 IO Matrix 路由到任意 GPIO,不影响使用):
 *   LCD_CS=GPIO9  LCD_DC=GPIO10  LCD_SCL(SCK)=GPIO11  LCD_SDA(MOSI)=GPIO12
 *   LCD_RST=GPIO13  LCD_BLK=GPIO14
 * MISO(SDO) 原理图里没有接到 MCU,只读ID时也读不到真实数据,填 -1。
 */

#include <Arduino.h>
#include <SPI.h>

// ================== 已根据原理图核实的实际 GPIO ==================
#define PIN_LCD_CS    9    // LCD_CS
#define PIN_LCD_DC    10   // LCD_DC
#define PIN_LCD_SCK   11   // LCD_SCL (SPI 时钟)
#define PIN_LCD_MOSI  12   // LCD_SDA (SPI MOSI)
#define PIN_LCD_RST   13   // LCD_RST
#define PIN_LCD_BLK   14   // LCD_BLK (背光)
#define PIN_LCD_MISO  -1   // 原理图未接,读ID返回值仅供参考
// ===================================================================

SPIClass *lcdSPI = nullptr;

void lcdWriteCmd(uint8_t cmd) {
  digitalWrite(PIN_LCD_DC, LOW);
  digitalWrite(PIN_LCD_CS, LOW);
  lcdSPI->transfer(cmd);
  digitalWrite(PIN_LCD_CS, HIGH);
}

// 读 N 个字节(读之前会先发 1 个 dummy clock,很多屏幕控制器读寄存器的通用要求)
void lcdReadBytes(uint8_t cmd, uint8_t *buf, int n) {
  digitalWrite(PIN_LCD_DC, LOW);
  digitalWrite(PIN_LCD_CS, LOW);
  lcdSPI->transfer(cmd);
  digitalWrite(PIN_LCD_DC, HIGH);
  for (int i = 0; i < n; i++) {
    buf[i] = lcdSPI->transfer(0x00);
  }
  digitalWrite(PIN_LCD_CS, HIGH);
}

void hardwareReset() {
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(10);
  digitalWrite(PIN_LCD_RST, LOW);
  delay(20);
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(150); // 复位后控制器需要时间完成内部初始化
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  delay(200);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" ESP32-S3 硬件自检 - LCD 原始 SPI 通信测试");
  Serial.println("========================================");

  pinMode(PIN_LCD_CS, OUTPUT);
  pinMode(PIN_LCD_DC, OUTPUT);
  pinMode(PIN_LCD_RST, OUTPUT);
  pinMode(PIN_LCD_BLK, OUTPUT);
  digitalWrite(PIN_LCD_CS, HIGH);
  digitalWrite(PIN_LCD_BLK, HIGH); // 先把背光点亮,不管有没有画面,至少能看到背光亮不亮

  lcdSPI = new SPIClass(HSPI);
  lcdSPI->begin(PIN_LCD_SCK, PIN_LCD_MISO, PIN_LCD_MOSI, PIN_LCD_CS);
  lcdSPI->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // 先用低速 1MHz 保守测试

  Serial.println("背光已点亮,请肉眼确认屏幕背光是否亮起(说明 BLK 网络和供电没问题)。");
  Serial.println("正在做硬件复位...");
  hardwareReset();

  uint8_t id[4] = {0};
  lcdReadBytes(0x04, id, 4); // Read Display ID (RDDID)
  Serial.printf("Read Display ID (0x04) 返回: %02X %02X %02X %02X\n",
                id[0], id[1], id[2], id[3]);

  uint8_t pwrMode[2] = {0};
  lcdReadBytes(0x0A, pwrMode, 2); // Read Display Power Mode
  Serial.printf("Read Power Mode (0x0A) 返回: %02X %02X\n", pwrMode[0], pwrMode[1]);

  Serial.println("----------------------------------------");
  bool allZero = (id[1] == 0 && id[2] == 0 && id[3] == 0);
  bool allFF   = (id[1] == 0xFF && id[2] == 0xFF && id[3] == 0xFF);
  if (allZero || allFF) {
    Serial.println("!! 返回全 0x00 或全 0xFF,大概率是 SPI 接线有问题:");
    Serial.println("   - 检查 CS/DC/SCK/MOSI/RST 是否真的接到了上面配置的 GPIO");
    Serial.println("   - 检查屏幕供电(VCC/GND)是否正常");
    Serial.println("   - 部分山寨屏幕本身不支持这条读命令,即使接线是对的也可能读不到,"
                    "这种情况请直接跳到 esp32s3_lcd_fill_test 用画面判断");
  } else {
    Serial.println("收到了非全零/非全FF的响应,说明 SPI 通信链路大概率是通的。");
    Serial.println("具体 ID 值和数据手册对不对得上不重要(很多山寨屏对不上),");
    Serial.println("接下来可以去跑 esp32s3_lcd_fill_test 做真正的点亮测试。");
  }
  Serial.println("========================================");
}

void loop() {
  delay(1000);
}
