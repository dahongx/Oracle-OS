/*
 * ESP32-S3 硬件自检固件
 * -----------------------------------------
 * 目的: 只验证"USB 能否成功下载固件 + 主控芯片能否正常跑起来 + 能否通过串口打印",
 *       不涉及 LCD / AS5600 / DRV2605L 等外围模块,把最基础的一环先确认清楚。
 *
 * 现象:
 *   1) 用 PlatformIO 通过 USB 把本工程烧录进板子(全程不需要外部 USB转串口器件,
 *      走的是 ESP32-S3 芯片自带的 USB Serial/JTAG)。
 *   2) 烧录完成后打开串口监视器(115200 波特率),应能看到一段初始化信息,
 *      之后每秒打印一条心跳日志。
 *   3) 只要能看到这些打印,就说明: USB下载电路正常、主控芯片正常起振/运行、
 *      USB转串口(原生)链路正常。
 */

#include <Arduino.h>

void setup() {
  Serial.begin(115200);

  // 原生 USB CDC 需要等待电脑侧枚举完成、串口监视器打开,
  // 这里最多等 3 秒,超时也会继续往下走(避免监视器没及时打开导致卡死在这里)
  unsigned long waitStart = millis();
  while (!Serial && (millis() - waitStart) < 3000) {
    delay(10);
  }
  delay(200); // 给串口监视器留一点缓冲时间,避免开头几行被吞掉

  Serial.println();
  Serial.println("========================================");
  Serial.println("   ESP32-S3 硬件自检 - 串口通信测试固件");
  Serial.println("========================================");
  Serial.printf("Chip Model      : %s\n", ESP.getChipModel());
  Serial.printf("Chip Revision   : %d\n", ESP.getChipRevision());
  Serial.printf("CPU Freq (MHz)  : %d\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash Size (MB) : %u\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("Free Heap (B)   : %u\n", ESP.getFreeHeap());
  Serial.printf("SDK Version     : %s\n", ESP.getSdkVersion());
  Serial.println("----------------------------------------");
  Serial.println("初始化完成! USB 下载与串口通信正常。");
  Serial.println("========================================");
}

uint32_t heartbeat = 0;

void loop() {
  Serial.printf("[%8lu ms] heartbeat #%lu - ESP32-S3 running OK\n",
                (unsigned long)millis(), (unsigned long)heartbeat++);
  delay(1000);
}
