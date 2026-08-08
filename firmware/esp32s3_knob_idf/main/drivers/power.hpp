#pragma once
// ============================================================================
// 电源管理驱动 (Driver Layer - Power latch)
// ----------------------------------------------------------------------------
// 电池供电的自锁电路:POWER_CTL 保持高电平才不断电;拉低则释放自锁 => 关机。
// 注意:USB 供电会绕过自锁电路,拉低 POWER_CTL 不会真正断电(仅电池供电时有效)。
// ============================================================================
#include <driver/gpio.h>

#include "logger.hpp"

namespace drivers {

class Power {
public:
  struct Config {
    gpio_num_t power_ctl;
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::WARN};
  };

  explicit Power(const Config &config)
      : cfg_(config), logger_({.tag = "Power", .level = config.log_level}) {
    gpio_config_t conf{
        .pin_bit_mask = 1ULL << cfg_.power_ctl,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&conf);
    latch_on(); // 上电即保持供电
  }

  // 保持供电自锁(高电平)
  void latch_on() { gpio_set_level(cfg_.power_ctl, 1); }

  // 关机:拉低 POWER_CTL 释放自锁。电池供电时系统立即断电;USB 供电无效。
  void shutdown() {
    logger_.warn("关机: 拉低 POWER_CTL(GPIO{}) 释放电池自锁", (int)cfg_.power_ctl);
    gpio_set_level(cfg_.power_ctl, 0);
  }

private:
  Config cfg_;
  espp::Logger logger_;
};

} // namespace drivers
