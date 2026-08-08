/*
 * ESP32-S3 硬件自检固件 - DRV2605L + 线性马达 震动测试
 * -----------------------------------------------------------------
 * 引脚已根据原理图(BSP.SchDoc)核实:
 *   M_SDA = GPIO3, M_SCL = GPIO8  —— DRV2605L 专用的一条 I2C 总线(和 AS5600 是分开的)
 *   M_EN  = GPIO46               —— 接 DRV2605L 的 EN 脚,必须先拉高使能芯片,
 *                                    否则这颗 I2C 地址(0x5A)扫不到、也不会有任何输出!
 *   IN_TRIG 脚在硬件上直接接了 GND(没有接到 MCU),所以只能用 I2C 内部触发(setMode INTTRIG + go()),
 *   不能用外部触发方式。
 *
 * 目的: 确认 DRV2605L 芯片 I2C 通信正常 + M+/M- 到线性马达这条驱动链路是通的、马达没坏。
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// ================== 已根据原理图核实的实际 GPIO ==================
#define PIN_I2C_SDA   3    // M_SDA
#define PIN_I2C_SCL   8    // M_SCL
#define PIN_DRV_EN    46   // M_EN,必须拉高使能 DRV2605L
// ===================================================================

Adafruit_DRV2605 drv;
bool drvOk = false;

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000) delay(10);
  delay(200);

  pinMode(PIN_DRV_EN, OUTPUT);
  digitalWrite(PIN_DRV_EN, HIGH); // 先使能 DRV2605L,再做任何 I2C 操作
  delay(10);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" ESP32-S3 硬件自检 - DRV2605L 触觉马达测试");
  Serial.printf(" I2C 引脚: SDA=GPIO%d SCL=GPIO%d   EN=GPIO%d(已拉高)\n",
                PIN_I2C_SDA, PIN_I2C_SCL, PIN_DRV_EN);
  Serial.println("========================================");

  drvOk = drv.begin();
  if (!drvOk) {
    Serial.println("!! 没有找到 DRV2605L (0x5A),请检查: EN 是否真的拉高、I2C 接线/供电 !!");
    return;
  }
  Serial.println("DRV2605L 已找到,开始配置...");

  drv.selectLibrary(1);
  drv.useLRA();          // 线性马达(LRA)。如果实际是转子马达(ERM),改成 drv.useERM();
  drv.setMode(DRV2605_MODE_INTTRIG); // IN_TRIG 硬件脚接了 GND 没接 MCU,只能用内部触发: go()

  Serial.println("配置完成,进入循环播放测试。");
  Serial.println("如果马达有感觉/能听到轻微震动声,说明驱动链路正常。");
  Serial.println("----------------------------------------");
}

// 挑几个手感区别明显的内置效果轮流播放,方便凭感觉判断马达有没有反应
const uint8_t effects[] = {
  1,   // Strong Click - 100%
  14,  // Strong Buzz
  47,  // Buzz 1 - 100%
  70,  // Transition Ramp Down Long Smooth 1 - 100 to 0%
};
const int numEffects = sizeof(effects) / sizeof(effects[0]);
int idx = 0;
unsigned long lastFire = 0;

void loop() {
  if (!drvOk) {
    // 没找到芯片就每 2 秒重试一次,方便你一边插排线一边看有没有连上
    if (millis() - lastFire > 2000) {
      lastFire = millis();
      digitalWrite(PIN_DRV_EN, HIGH);
      drvOk = drv.begin();
      Serial.println(drvOk ? "DRV2605L 现在找到了!" : "仍未找到 DRV2605L...");
      if (drvOk) {
        drv.selectLibrary(1);
        drv.useLRA();
        drv.setMode(DRV2605_MODE_INTTRIG);
      }
    }
    return;
  }

  if (millis() - lastFire > 1500) {
    lastFire = millis();
    uint8_t e = effects[idx];
    Serial.printf("[%8lu ms] 播放 effect #%u ...\n", millis(), e);
    drv.setWaveform(0, e);
    drv.setWaveform(1, 0); // 结束标记
    drv.go();
    idx = (idx + 1) % numEffects;
  }
}
