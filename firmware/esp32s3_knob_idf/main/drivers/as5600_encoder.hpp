#pragma once
// ============================================================================
// AS5600 磁编码器驱动 (Driver Layer - real Hall input)
// ----------------------------------------------------------------------------
// 封装 espp::As5600 + 它专用的 I2C 总线,实现 IInputSource 接口。
// 对应用层只暴露 degrees()/mech_degrees()/rpm();I2C 时序、速度滤波、磁铁诊断
// 全封装在内部。磁铁装好后,Board 用它替换 SerialSource 即可。
// ============================================================================
#include <memory>

#include "as5600.hpp"
#include "butterworth_filter.hpp"
#include "i2c.hpp"
#include "logger.hpp"

#include "drivers/input.hpp"

namespace drivers {

class As5600Encoder : public IInputSource {
public:
  struct Config {
    i2c_port_t port;
    gpio_num_t sda;
    gpio_num_t scl;
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::WARN};
  };

  explicit As5600Encoder(const Config &config);

  bool ok() const { return dev_ != nullptr; }

  // ---- IInputSource ----
  float degrees() override { return dev_ ? dev_->get_degrees() : 0.0f; }
  float mech_degrees() override { return dev_ ? dev_->get_mechanical_degrees() : 0.0f; }
  float rpm() override { return dev_ ? dev_->get_rpm() : 0.0f; }
  const char *name() override { return "AS5600(霍尔)"; }

  // 磁铁位置诊断(STATUS/AGC/MAGNITUDE)与静止噪声测试——焊好后确认气隙/对中用。
  void diagnose_magnet();
  void diagnose_noise();

private:
  uint16_t read_reg16(uint8_t reg_hi);

  espp::Logger logger_;
  std::unique_ptr<espp::I2c> i2c_;
  std::unique_ptr<espp::As5600> dev_;
};

} // namespace drivers
