#include "drivers/haptic.hpp"

#include <chrono>
#include <thread>

#include <driver/gpio.h>

namespace drivers {

static constexpr uint8_t DRV_ADDR = 0x5A;

Drv2605Haptic::Drv2605Haptic(const Config &config)
    : logger_({.tag = "Haptic", .level = config.log_level}) {
  // EN 必须先拉高,DRV2605L 才会应答 I2C(否则扫不到 0x5A)
  gpio_config_t en_conf{
      .pin_bit_mask = 1ULL << config.en,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&en_conf);
  gpio_set_level(config.en, 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  i2c_ = std::make_unique<espp::I2c>(espp::I2c::Config{
      .port = config.port,
      .sda_io_num = config.sda,
      .scl_io_num = config.scl,
  });

  std::error_code ec;
  auto device = i2c_->add_device<uint8_t>(
      {.device_address = espp::Drv2605::DEFAULT_ADDRESS,
       .timeout_ms = static_cast<int>(i2c_->config().timeout_ms),
       .scl_speed_hz = i2c_->config().clk_speed,
       .log_level = espp::Logger::Verbosity::WARN},
      ec);
  if (!device) {
    logger_.error("DRV2605L I2C 初始化失败: {}", ec.message());
    return;
  }

  dev_ = std::make_unique<espp::Drv2605>(espp::Drv2605::Config{
      .device_address = espp::Drv2605::DEFAULT_ADDRESS,
      .write = espp::make_i2c_addressed_write(device),
      .read_register = espp::make_i2c_addressed_read_register(device),
      .motor_type = espp::Drv2605::MotorType::LRA, // 板子上是线性马达
      .log_level = espp::Logger::Verbosity::WARN,
  });
  dev_->select_library(espp::Drv2605::Library::LRA, ec);
  if (ec) {
    logger_.error("DRV2605L select_library 失败: {}", ec.message());
    dev_.reset();
    return;
  }

  // LRA 必须先自动校准:芯片用校准结果确定 RATEDV / CLAMPV,不校准直接播放会几乎无感。
  // 马达 LD0832AA 谐振 235±10Hz;drive_time 按真实谐振算,偏差大会大幅衰减。
  espp::Drv2605::LraCalibrationSettings lra_cal{};
  lra_cal.rated_voltage = 255;
  lra_cal.overdrive_clamp = 255;
  lra_cal.drive_time = espp::Drv2605::lra_freq_to_drive_time(235.0f);
  espp::Drv2605::CalibratedData cal_data;
  if (!dev_->calibrate(lra_cal, cal_data, ec))
    logger_.error("DRV2605L 自动校准失败: {}", ec.message());
  else
    logger_.info("DRV2605L 自动校准完成");

  logger_.info("DRV2605L 初始化完成 (SDA=GPIO{} SCL=GPIO{} EN=GPIO{})", (int)config.sda,
               (int)config.scl, (int)config.en);
}

void Drv2605Haptic::write_reg(uint8_t reg, uint8_t val) {
  if (!i2c_)
    return;
  uint8_t buf[2] = {reg, val};
  i2c_->write(DRV_ADDR, buf, 2);
}

// 榨最大振幅的三个关键(都已设置):
//  1. CLAMPV=0xFF:驱动电压顶到芯片供电上限
//  2. OL_LRA_PERIOD(0x20)=0x2B:开环驱动频率强制=235Hz(谐振点)。开环下芯片不会自动
//     跟踪谐振,必须手动设到 F0;count=(1/235)/98.46us≈43=0x2B。这是开环下最关键一步。
//  3. duration 足够长(>80ms):规格书上升时间 Max 50ms,给足起振时间。
// 播完归零并恢复闭环状态,不影响下次调用。
void Drv2605Haptic::play_openloop(int duration_ms) {
  if (!i2c_)
    return;
  constexpr uint8_t REG_MODE = 0x01;
  constexpr uint8_t REG_RTPIN = 0x02;
  constexpr uint8_t REG_CLAMPV = 0x17;
  constexpr uint8_t REG_OL_LRA_PERIOD = 0x20;
  constexpr uint8_t REG_CONTROL3 = 0x1D;

  // 钳位电压按强度缩放:strength=1 -> 0xFF(最强),越小越弱。最低保留 0x30 避免完全无感。
  uint8_t clampv = (uint8_t)(0x30 + (int)((0xFF - 0x30) * strength_));
  write_reg(REG_OL_LRA_PERIOD, 0x2B); // 开环驱动周期 = 235Hz(谐振点),最关键
  write_reg(REG_CONTROL3, 0x09);      // LRA 开环 + RTP 无符号数据格式
  write_reg(REG_CLAMPV, clampv);      // 过驱动钳位电压(开环下直接决定幅度)
  write_reg(REG_MODE, 0x05);          // RTP 实时播放 + 退出待机
  write_reg(REG_RTPIN, 0xFF);         // 满量程全幅驱动

  std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));

  write_reg(REG_RTPIN, 0x00);    // 停止驱动
  write_reg(REG_CONTROL3, 0xA0); // 复位默认(回闭环)
  write_reg(REG_MODE, 0x00);     // MODE 回 INTTRIG,和 espp 驱动缓存一致
}

// 播放DRV2605L内置波形库中的波形
void Drv2605Haptic::play(uint8_t waveform_id) {
  if (!dev_)
    return;

  std::error_code ec;
  // 设置波形到slot 0
  if (!dev_->set_waveform(0, static_cast<espp::Drv2605::Waveform>(waveform_id), ec)) {
    logger_.warn("设置波形 {} 失败: {}", waveform_id, ec.message());
    return;
  }
  // 设置slot 1为END标记
  if (!dev_->set_waveform(1, espp::Drv2605::Waveform::END, ec)) {
    logger_.warn("设置波形END标记失败: {}", ec.message());
    return;
  }
  // 开始播放
  dev_->start(ec);
  if (ec) {
    logger_.warn("播放波形 {} 失败: {}", waveform_id, ec.message());
  }
}

} // namespace drivers
