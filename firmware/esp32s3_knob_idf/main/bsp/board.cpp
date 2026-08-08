#include "bsp/board.hpp"
#include "bsp/pins.hpp"

namespace bsp {

Board::Board() : logger_({.tag = "Board", .level = espp::Logger::Verbosity::INFO}) {
  logger_.info("==== 构建板级驱动 ====");

  // 电源:最先上电自锁,保证电池供电时不掉电
  power_ = std::make_unique<drivers::Power>(drivers::Power::Config{.power_ctl = pins::POWER_CTL});

  // 触觉马达
  haptic_ = std::make_unique<drivers::Drv2605Haptic>(drivers::Drv2605Haptic::Config{
      .port = I2C_NUM_1, .sda = pins::DRV_SDA, .scl = pins::DRV_SCL, .en = pins::DRV_EN});

  // 磁编码器(真实霍尔);磁铁没调好前不作为默认输入源,但先建好并跑诊断
  encoder_ = std::make_unique<drivers::As5600Encoder>(drivers::As5600Encoder::Config{
      .port = I2C_NUM_0, .sda = pins::AS5600_SDA, .scl = pins::AS5600_SCL});
  if (encoder_->ok()) {
    encoder_->diagnose_magnet();
    encoder_->diagnose_noise();
  } else {
    logger_.warn("AS5600 未初始化成功,霍尔输入不可用(将只用串口模拟)");
    encoder_.reset();
  }

  // 电池采样
  battery_ = std::make_unique<drivers::Battery>(drivers::Battery::Config{
      .unit = pins::BAT_ADC_UNIT, .channel = pins::BAT_ADC_CHANNEL});

  // 显示屏
  display_ = std::make_unique<drivers::DisplayPanel>(drivers::DisplayPanel::Config{
      .mosi = pins::LCD_MOSI,
      .sclk = pins::LCD_SCLK,
      .cs = pins::LCD_CS,
      .dc = pins::LCD_DC,
      .rst = pins::LCD_RST,
      .blk = pins::LCD_BLK,
      .width = 240,
      .height = 240,
      .log_level = espp::Logger::Verbosity::INFO});

  logger_.info("==== 板级驱动构建完成 ====");
}

void Board::set_button_callbacks(std::function<void()> on_short, std::function<void()> on_double,
                                 std::function<void()> on_long,
                                 std::function<void()> on_power,
                                 std::function<void(float)> on_hold_progress) {
  on_short_ = std::move(on_short);
  on_double_ = std::move(on_double);
  on_long_ = std::move(on_long);
  on_power_ = std::move(on_power);
  on_hold_progress_ = std::move(on_hold_progress);
  button_ = std::make_unique<drivers::ButtonInput>(drivers::ButtonInput::Config{
      .gpio = pins::KEY,
      .long_press_ms = 3000,
      .power_press_ms = 10000,
      .on_short_press = [this]() { if (on_short_) on_short_(); },
      .on_double_press = [this]() { if (on_double_) on_double_(); },
      .on_long_press = [this]() { if (on_long_) on_long_(); },
      .on_power_press = [this]() { if (on_power_) on_power_(); },
      .on_hold_progress = [this](float p) { if (on_hold_progress_) on_hold_progress_(p); }});
}

} // namespace bsp
