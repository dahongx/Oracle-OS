#pragma once
// ============================================================================
// 电池电压驱动 (Driver Layer - Battery)
// ----------------------------------------------------------------------------
// 应用层只调 voltage() 拿实际电池电压(伏);ADC 通道/分压换算封在内部。
// 板上 R3/R4 各 100K 等比分压 => 实际电压 = 采样电压 x2。
// ============================================================================
#include <memory>
#include <optional>

#include "oneshot_adc.hpp"

namespace drivers {

class Battery {
public:
  struct Config {
    adc_unit_t unit;
    adc_channel_t channel;
    float divider_ratio{2.0f}; // 分压反算系数(100K/100K => x2)
  };

  explicit Battery(const Config &config) : cfg_(config) {
    adc_ = std::make_unique<espp::OneshotAdc>(espp::OneshotAdc::Config{
        .unit = cfg_.unit,
        .channels = {{.unit = cfg_.unit,
                      .channel = cfg_.channel,
                      .attenuation = ADC_ATTEN_DB_12}},
    });
  }

  // 返回实际电池电压(伏);读失败返回 std::nullopt。
  std::optional<float> voltage() {
    auto mv = adc_->read_mv(
        {.unit = cfg_.unit, .channel = cfg_.channel, .attenuation = ADC_ATTEN_DB_12});
    if (!mv.has_value())
      return std::nullopt;
    return (mv.value() / 1000.0f) * cfg_.divider_ratio;
  }

private:
  Config cfg_;
  std::unique_ptr<espp::OneshotAdc> adc_;
};

} // namespace drivers
