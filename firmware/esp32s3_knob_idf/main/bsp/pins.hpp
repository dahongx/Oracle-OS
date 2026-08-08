#pragma once
// ============================================================================
// 板级引脚定义 (BSP - Pin Map)
// ----------------------------------------------------------------------------
// 全项目唯一"知道器件接在哪个 GPIO"的地方。驱动层不写死引脚,由 Board 从这里取值
// 传进各驱动的 Config。换板子/改硬件只动这一个文件。
//
// 所有引脚均已按原理图(doc/Oracle OS.pdf)像素级核对确认,不是猜测值。
// ============================================================================
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <hal/adc_types.h>

namespace bsp::pins {

// ---- AS5600 磁编码器 (独立 I2C 总线 0) ----
static constexpr gpio_num_t AS5600_SDA = GPIO_NUM_1;
static constexpr gpio_num_t AS5600_SCL = GPIO_NUM_2;

// ---- DRV2605L 触觉马达驱动 (独立 I2C 总线 1;EN 需拉高芯片才应答) ----
static constexpr gpio_num_t DRV_SDA = GPIO_NUM_8;
static constexpr gpio_num_t DRV_SCL = GPIO_NUM_9;
static constexpr gpio_num_t DRV_EN = GPIO_NUM_10;

// ---- 2.8" SPI LCD (ST7789V2, 写only, 无触摸) ----
static constexpr gpio_num_t LCD_MOSI = GPIO_NUM_21;
static constexpr gpio_num_t LCD_SCLK = GPIO_NUM_14;
static constexpr gpio_num_t LCD_CS = GPIO_NUM_13;
static constexpr gpio_num_t LCD_DC = GPIO_NUM_12;
static constexpr gpio_num_t LCD_RST = GPIO_NUM_47;
static constexpr gpio_num_t LCD_BLK = GPIO_NUM_11;

// ---- 按键 / 电源自锁 / 电池采样 ----
static constexpr gpio_num_t KEY = GPIO_NUM_39;       // 按下为低(板上 R16 10K 上拉)
static constexpr gpio_num_t POWER_CTL = GPIO_NUM_40; // 高=保持供电自锁,低=断电
static constexpr adc_unit_t BAT_ADC_UNIT = ADC_UNIT_1;
static constexpr adc_channel_t BAT_ADC_CHANNEL = ADC_CHANNEL_3; // GPIO4,100K/100K 分压

} // namespace bsp::pins
