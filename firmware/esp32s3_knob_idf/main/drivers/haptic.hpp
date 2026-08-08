#pragma once
// ============================================================================
// 触觉反馈驱动 (Driver Layer - Haptic)
// ----------------------------------------------------------------------------
// 应用层只调 click()/strong()/buzz(ms),不关心 DRV2605L 的寄存器时序。
// LRA 开环全幅驱动那套硬啃出来的参数(235Hz 谐振点 / CLAMPV=0xFF)全封在实现里。
// ============================================================================
#include <cstdint>
#include <memory>

#include "drv2605.hpp"
#include "i2c.hpp"
#include "logger.hpp"

namespace drivers {

// 触觉接口:对应用层来说触觉就是几种"敲击"语义。
class IHaptic {
public:
  virtual ~IHaptic() = default;
  virtual void click() = 0;            // 一格的轻敲(旋钮过格)
  virtual void strong() = 0;           // 强敲(确认/进入)
  virtual void buzz(int duration_ms) = 0; // 自定时长的持续震(提示/关机)
};

// DRV2605L + LRA 实现。拥有自己的 I2C 总线 + EN 引脚。
class Drv2605Haptic : public IHaptic {
public:
  struct Config {
    i2c_port_t port;
    gpio_num_t sda;
    gpio_num_t scl;
    gpio_num_t en; // 使能脚,必须先拉高芯片才应答 I2C
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::WARN};
  };

  explicit Drv2605Haptic(const Config &config);

  bool ok() const { return dev_ != nullptr; }

  // ---- IHaptic ----
  void click() override { play_openloop(100); } // 上升时间 Max 50ms,给足起振
  void strong() override { play_openloop(150); }
  void buzz(int duration_ms) override { play_openloop(duration_ms); }

  // 触觉强度 [0,1]:缩放开环钳位电压(CLAMPV)。1.0=最强(0xFF),0=几乎无感。
  void set_strength(float s) {
    if (s < 0.0f) s = 0.0f;
    if (s > 1.0f) s = 1.0f;
    strength_ = s;
  }
  float strength() const { return strength_; }

  // 播放DRV2605L内置波形(1-123)
  void play(uint8_t waveform_id);

private:
  void write_reg(uint8_t reg, uint8_t val);
  // 开环 RTP 全幅驱动:追求最强震感(马达贴塑料外壳内,外壳衰减,要榨到最大)。
  void play_openloop(int duration_ms);

  espp::Logger logger_;
  std::unique_ptr<espp::I2c> i2c_;
  std::unique_ptr<espp::Drv2605> dev_;
  float strength_{1.0f}; // 开环钳位电压缩放 [0,1]
};

} // namespace drivers
