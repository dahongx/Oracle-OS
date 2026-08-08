#include "app/knob_service.hpp"

#include <cmath>

using namespace std::chrono_literals;

namespace app {

KnobService::KnobService(const Config &config)
    : cfg_(config), logger_({.tag = "KnobService", .level = config.log_level}),
      source_(config.initial_source) {}

void KnobService::set_source(drivers::IInputSource *source) {
  if (!source)
    return;
  source_.store(source);
  logger_.info("==== 当前旋钮输入源: {} ====", source->name());
}

void KnobService::start() {
  task_ = espp::Task::make_unique({
      .callback = [this](std::mutex &m, std::condition_variable &cv) -> bool {
        return tick(m, cv);
      },
      .task_config = {.name = "Knob Logic", .stack_size_bytes = 6144, .priority = 10},
  });
  task_->start();
}

bool KnobService::tick(std::mutex &m, std::condition_variable &cv) {
  auto *src = source_.load();
  if (src) {
    float degrees = src->degrees();      // 累计角度(可超过360)
    int angle_0_360 = ((int)std::lround(degrees) % 360 + 360) % 360;
    if (cfg_.on_angle)
      cfg_.on_angle(degrees, angle_0_360);

    float boundary = std::round(degrees / cfg_.detent_step_deg) * cfg_.detent_step_deg;
    if (std::abs(boundary - last_detent_boundary_) >= cfg_.detent_step_deg - 0.01f) {
      int direction = (boundary > last_detent_boundary_) ? +1 : -1;
      last_detent_boundary_ = boundary;
      int count = (int)std::lround(boundary / cfg_.detent_step_deg);
      detent_count_.store(count);
      if (cfg_.on_detent)
        cfg_.on_detent(count);
      if (cfg_.on_step) {
        // 强度 = 由转速映射:|rpm| 越大强度越高。慢速(~静止)≈1,快转封顶 ~4。
        float rpm = std::abs(src->rpm());
        float strength = 1.0f + rpm / 40.0f; // 40rpm -> +1
        if (strength > 4.0f)
          strength = 4.0f;
        cfg_.on_step(direction, strength);
      }
      logger_.info("[{}] >>> 跨过格位! 当前格位={} (每 {:.0f}deg 一格)", src->name(), count,
                   cfg_.detent_step_deg);
      if (cfg_.haptic)
        cfg_.haptic->click();
    }
  }

  std::unique_lock<std::mutex> lk(m);
  cv.wait_for(lk, 20ms);
  return false; // 一直跑
}

} // namespace app
