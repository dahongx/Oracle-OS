#include "drivers/as5600_encoder.hpp"

#include <chrono>
#include <thread>

namespace drivers {

static constexpr uint8_t AS5600_ADDR = 0x36;

As5600Encoder::As5600Encoder(const Config &config)
    : logger_({.tag = "AS5600", .level = config.log_level}) {
  i2c_ = std::make_unique<espp::I2c>(espp::I2c::Config{
      .port = config.port,
      .sda_io_num = config.sda,
      .scl_io_num = config.scl,
      // 关键:bit-bang(~100kHz、边沿余量大)能探到 AS5600 的 ACK,但 espp 硬件 I2C 默认 400kHz
      // 太快,配上偏弱的上拉,新版驱动会判定总线状态非法(ESP_ERR_INVALID_STATE)。
      // 所以把这条总线降到 50kHz,并开内部上拉,给弱上拉留足建立时间。
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .clk_speed = 50 * 1000,
  });

  std::error_code ec;
  auto device = i2c_->add_device<uint8_t>(
      {.device_address = espp::As5600::DEFAULT_ADDRESS,
       .timeout_ms = static_cast<int>(i2c_->config().timeout_ms),
       .scl_speed_hz = i2c_->config().clk_speed,
       .log_level = espp::Logger::Verbosity::WARN},
      ec);
  if (!device) {
    logger_.error("AS5600 I2C 初始化失败: {}", ec.message());
    return;
  }

  // 速度做一点低通滤波,减少 AS5600 本身噪声的影响
  static constexpr float filter_cutoff_hz = 4.0f;
  static constexpr float update_period_s = 0.01f;
  static espp::ButterworthFilter<2, espp::BiquadFilterDf2> velocity_filter(
      {.normalized_cutoff_frequency = 2.0f * filter_cutoff_hz * update_period_s});

  dev_ = std::make_unique<espp::As5600>(espp::As5600::Config{
      .write_then_read = espp::make_i2c_addressed_write_then_read(device),
      .velocity_filter = [](float raw) -> float { return velocity_filter.update(raw); },
      .update_period = std::chrono::duration<float>(update_period_s),
      .log_level = espp::Logger::Verbosity::WARN,
  });
  logger_.info("AS5600 初始化完成 (SDA=GPIO{} SCL=GPIO{})", (int)config.sda, (int)config.scl);
}

uint16_t As5600Encoder::read_reg16(uint8_t reg_hi) {
  uint8_t hi = 0, lo = 0;
  i2c_->read_at_register(AS5600_ADDR, reg_hi, &hi, 1);
  i2c_->read_at_register(AS5600_ADDR, reg_hi + 1, &lo, 1);
  return ((uint16_t)(hi & 0x0F) << 8) | lo; // 12bit
}

// STATUS: bit5 MD=磁铁检测到, bit4 ML=磁场太弱(太远), bit3 MH=磁场太强(太近)。
// AGC: 3.3V 供电范围 0~128,理想居中(~64)——偏高=磁场弱,偏低=磁场强。
void As5600Encoder::diagnose_magnet() {
  if (!i2c_)
    return;
  uint8_t status = 0, agc = 0, conf_hi = 0, conf_lo = 0, zmco = 0;
  i2c_->read_at_register(AS5600_ADDR, 0x0B, &status, 1);
  i2c_->read_at_register(AS5600_ADDR, 0x1A, &agc, 1);
  i2c_->read_at_register(AS5600_ADDR, 0x07, &conf_hi, 1);
  i2c_->read_at_register(AS5600_ADDR, 0x08, &conf_lo, 1);
  i2c_->read_at_register(AS5600_ADDR, 0x00, &zmco, 1);
  uint16_t magnitude = read_reg16(0x1B);
  uint16_t raw_angle = read_reg16(0x0C);
  uint16_t angle = read_reg16(0x0E);

  bool md = status & (1 << 5);
  bool ml = status & (1 << 4);
  bool mh = status & (1 << 3);

  logger_.info("======== AS5600 磁铁 & 寄存器诊断 ========");
  logger_.info("STATUS=0x{:02X}  MD磁铁检测={}  ML磁场太弱={}  MH磁场太强={}", status, md, ml, mh);
  logger_.info("AGC自动增益={} (3.3V下范围0~128,理想居中~64)", agc);
  logger_.info("MAGNITUDE磁场幅度={} (配合 AGC 看气隙是否合适)", magnitude);
  logger_.info("RAW_ANGLE={} ({:.1f}deg)  ANGLE={} ({:.1f}deg)", raw_angle,
               raw_angle * 360.0f / 4096.0f, angle, angle * 360.0f / 4096.0f);
  logger_.info("CONF=0x{:02X}{:02X}  ZMCO已烧写次数={}", conf_hi, conf_lo, zmco & 0x03);

  if (!md)
    logger_.warn("  => 未检测到磁铁! 检查对轴磁铁装好没、正对芯片中心没");
  else if (mh)
    logger_.warn("  => 磁场太强(磁铁太近),建议加大气隙");
  else if (ml)
    logger_.warn("  => 磁场太弱(磁铁太远/偏心),建议减小气隙或对中");
  else
    logger_.info("  => 磁铁位置正常。AGC 越接近 64 越理想");
  logger_.info("==========================================");
}

void As5600Encoder::diagnose_noise() {
  if (!i2c_)
    return;
  constexpr int N = 50;
  uint16_t vmin = 0xFFFF, vmax = 0;
  uint32_t sum = 0;
  logger_.info("---- AS5600 噪声测试: 请【保持旋钮静止】,连续采样 {} 次 ----", N);
  for (int i = 0; i < N; i++) {
    uint16_t raw = read_reg16(0x0C);
    if (raw < vmin) vmin = raw;
    if (raw > vmax) vmax = raw;
    sum += raw;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  float avg = (float)sum / N;
  int span = (int)vmax - (int)vmin;
  logger_.info("噪声结果: 均值={:.1f} 最小={} 最大={} 峰峰抖动={} counts (~{:.3f}deg)", avg, vmin,
               vmax, span, span * 360.0f / 4096.0f);
  if (span <= 2)
    logger_.info("  => 抖动很小,磁场干净,气隙/对中良好");
  else if (span <= 8)
    logger_.info("  => 抖动中等,可用");
  else
    logger_.warn("  => 抖动偏大! 可能磁铁偏心/气隙不当/附近有金属或干扰");
}

} // namespace drivers
