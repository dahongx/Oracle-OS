/*
 * ESP32-S3 硬件自检固件 - I2C 总线扫描 + AS5600 角度读取 + 按键(KEY) + 电池电压(ADC)
 * -----------------------------------------------------------------
 * 引脚已经根据你提供的原理图(Oracle OS.pdf / MCU.SchDoc、BSP.SchDoc、POWER.SchDoc)
 * 核实过,不再是猜测的默认值:
 *
 *   AS5600  : SDA = GPIO1  (AS_SDA)   SCL = GPIO2  (AS_SCL)   —— 独立的一条 I2C 总线
 *   DRV2605L: SDA = GPIO3  (M_SDA)    SCL = GPIO8  (M_SCL)    —— 另一条独立的 I2C 总线
 *             (注意 AS5600 和 DRV2605L 不共用总线,是两组不同的 GPIO!)
 *   KEY(按键读)       = GPIO39,原理图里默认高电平(R16 10K 上拉 3V3),按下 BUTTON 时被拉低
 *   BAT_ADC(电池电压) = GPIO4,经过 R3/R4(100K/100K)等比分压,实际电池电压 ≈ ADC读数电压 x 2
 *
 * 本工程只扫 AS5600 所在的总线(GPIO1/2),顺便也扫一下 DRV2605L 所在的总线(GPIO3/8)
 * 一起确认,这样两颗 I2C 芯片是否在线可以一次性看到。
 * DRV2605L 的震动效果测试请看 esp32s3_motor_test 工程(那边需要额外把 M_EN=GPIO46 拉高)。
 */

#include <Arduino.h>
#include <Wire.h>

// ================== 已根据原理图核实的实际 GPIO ==================
#define PIN_AS5600_SDA   1    // AS_SDA
#define PIN_AS5600_SCL   2    // AS_SCL
#define PIN_DRV_SDA      3    // M_SDA
#define PIN_DRV_SCL      8    // M_SCL
#define PIN_KEY          39   // KEY (按下时为低电平)
#define PIN_BAT_ADC      4    // BAT_ADC (电阻分压,读数需要 x2 才是电池实际电压)
// ===================================================================

#define AS5600_ADDR   0x36   // AS5600 固定 I2C 地址
#define AS5600_REG_RAW_ANGLE_H 0x0C
#define DRV2605L_ADDR 0x5A   // DRV2605L 固定 I2C 地址

TwoWire I2C_AS5600 = TwoWire(0);
TwoWire I2C_DRV    = TwoWire(1);

bool scanOneBus(TwoWire &bus, const char *busName, uint8_t expectAddr, const char *expectName) {
  Serial.printf("---- 扫描 %s (SDA/SCL 见上面打印) ----\n", busName);
  bool foundExpected = false;
  int found = 0;
  for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
    bus.beginTransmission(addr);
    if (bus.endTransmission() == 0) {
      Serial.printf("  发现设备 -> 0x%02X", addr);
      if (addr == expectAddr) {
        Serial.printf("   (符合 %s 地址)", expectName);
        foundExpected = true;
      }
      Serial.println();
      found++;
    }
  }
  if (found == 0) {
    Serial.println("  !! 没有扫到任何设备,请检查该总线的 SDA/SCL 接线、上拉电阻、供电 !!");
  }
  return foundExpected;
}

bool as5600Present = false;

uint16_t readAS5600RawAngle() {
  I2C_AS5600.beginTransmission(AS5600_ADDR);
  I2C_AS5600.write(AS5600_REG_RAW_ANGLE_H);
  if (I2C_AS5600.endTransmission(false) != 0) return 0xFFFF;
  I2C_AS5600.requestFrom((uint8_t)AS5600_ADDR, (uint8_t)2);
  if (I2C_AS5600.available() < 2) return 0xFFFF;
  uint8_t hi = I2C_AS5600.read();
  uint8_t lo = I2C_AS5600.read();
  return ((uint16_t)(hi & 0x0F) << 8) | lo; // 12bit, 0~4095
}

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  delay(200);

  pinMode(PIN_KEY, INPUT); // 板上已有 R16 硬件上拉,不需要再开内部上拉

  I2C_AS5600.begin(PIN_AS5600_SDA, PIN_AS5600_SCL, 100000);
  I2C_DRV.begin(PIN_DRV_SDA, PIN_DRV_SCL, 100000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" ESP32-S3 硬件自检 - I2C 扫描 / AS5600 / KEY / 电池电压");
  Serial.printf(" AS5600  总线: SDA=GPIO%d  SCL=GPIO%d\n", PIN_AS5600_SDA, PIN_AS5600_SCL);
  Serial.printf(" DRV2605 总线: SDA=GPIO%d  SCL=GPIO%d\n", PIN_DRV_SDA, PIN_DRV_SCL);
  Serial.printf(" KEY 引脚: GPIO%d   BAT_ADC 引脚: GPIO%d\n", PIN_KEY, PIN_BAT_ADC);
  Serial.println("========================================");

  as5600Present = scanOneBus(I2C_AS5600, "AS5600 总线 (GPIO1/2)", AS5600_ADDR, "AS5600");
  bool drvPresent = scanOneBus(I2C_DRV, "DRV2605L 总线 (GPIO3/8)", DRV2605L_ADDR, "DRV2605L");

  Serial.printf("AS5600  是否在线: %s\n", as5600Present ? "是" : "否");
  Serial.printf("DRV2605L是否在线: %s (只扫描到,不代表马达会转,注意 DRV2605L 需要把 M_EN=GPIO46 拉高才会应答,"
                "如果这里显示否,先去 esp32s3_motor_test 工程确认)\n", drvPresent ? "是" : "否");
  Serial.println("----------------------------------------");
}

unsigned long lastPrint = 0;

void loop() {
  if (millis() - lastPrint >= 300) {
    lastPrint = millis();

    int keyLevel = digitalRead(PIN_KEY);
    int adcRaw = analogRead(PIN_BAT_ADC);
    float adcVolt = adcRaw * 3.3f / 4095.0f;
    float batVolt = adcVolt * 2.0f; // R3/R4 等比分压(100K/100K),实际电压是采样点的 2 倍

    if (as5600Present) {
      uint16_t raw = readAS5600RawAngle();
      if (raw == 0xFFFF) {
        Serial.printf("[%8lu ms] AS5600 读取失败(掉线?)  KEY=%d(0=按下)  BAT=%.2fV\n",
                      millis(), keyLevel, batVolt);
      } else {
        float deg = raw * 360.0f / 4096.0f;
        Serial.printf("[%8lu ms] AS5600 raw=%4u angle=%6.1f deg  KEY=%d(0=按下)  BAT=%.2fV  (转动磁铁看角度是否跟着变)\n",
                      millis(), raw, deg, keyLevel, batVolt);
      }
    } else {
      Serial.printf("[%8lu ms] AS5600 未在线,跳过角度读取  KEY=%d(0=按下)  BAT=%.2fV\n",
                    millis(), keyLevel, batVolt);
    }
  }
}
