#pragma once
// ============================================================================
// 按键驱动 (Driver Layer - Button)
// ----------------------------------------------------------------------------
// 把 GPIO 中断的"按下/松开边沿"翻译成应用层关心的两个语义事件:
//   on_short_press : 按下并在阈值内松开
//   on_long_press  : 按住超过 long_press_ms(用于长按关机)
// 长按需要轮询判断(中断只有边沿),所以提供 poll(),由某个周期任务定期调用。
// ============================================================================
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

#include <esp_timer.h>

#include "button.hpp"
#include "logger.hpp"

namespace drivers {

class ButtonInput {
public:
  struct Config {
    gpio_num_t gpio;
    uint32_t long_press_ms{3000};
    uint32_t power_press_ms{10000};
    uint32_t double_click_ms{360};
    uint32_t debounce_ms{40}; // 去抖窗口:同一根线上相邻边沿间隔小于此值视为抖动
    std::function<void()> on_short_press{};
    std::function<void()> on_double_press{};
    std::function<void()> on_long_press{};
    std::function<void()> on_power_press{};
    // 长按进度回调:按住期间以 [0,1] 报告进度(0=刚按下,1=达到长按阈值)。
    // 松手未达阈值时会以 progress=0 回调一次,让 UI 收起进度环。用于长按进度环可视化。
    std::function<void(float progress)> on_hold_progress{};
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::WARN};
  };

  explicit ButtonInput(const Config &config)
      : cfg_(config), logger_({.tag = "Button", .level = config.log_level}) {
    button_ = std::make_unique<espp::Button>(espp::Button::Config{
        .name = "KEY",
        .interrupt_config =
            {
                .gpio_num = cfg_.gpio,
                .callback = [this](const espp::Interrupt::Event &event) { on_edge(event.active); },
                .active_level = espp::Interrupt::ActiveLevel::LOW, // 板上 R16 上拉,按下拉低
                .interrupt_type = espp::Interrupt::Type::ANY_EDGE,
                .pullup_enabled = false, // 板上已有硬件上拉(R16 10K)
                .pulldown_enabled = false,
            },
        .task_config = {.name = "KEY task", .stack_size_bytes = 4096},
    });
  }

  // 周期调用:检测"按住已超过阈值"以触发长按(只触发一次),并在按住期间报告进度。
  void poll() {
    uint32_t start = press_start_ms_.load();
    if (start == 0) {
      uint32_t pending = pending_click_ms_.load();
      if (pending != 0 && now_ms() - pending >= cfg_.double_click_ms) {
        pending_click_ms_.store(0);
        logger_.debug("单击");
        if (cfg_.on_short_press)
          cfg_.on_short_press();
      }
      // 未按下:若上一轮还在报进度,补一个 0 让 UI 收起进度环
      if (hold_reported_ && cfg_.on_hold_progress) {
        cfg_.on_hold_progress(0.0f);
      }
      hold_reported_ = false;
      return;
    }
    uint32_t held = now_ms() - start;
    if (!power_fired_.load()) {
      float progress = (float)held / (float)cfg_.power_press_ms;
      if (progress > 1.0f)
        progress = 1.0f;
      if (cfg_.on_hold_progress) {
        cfg_.on_hold_progress(progress);
        hold_reported_ = true;
      }
      if (!long_fired_.load() && held >= cfg_.long_press_ms) {
        long_fired_.store(true);
        pending_click_ms_.store(0);
        logger_.info("长按 {} ms -> on_long_press", cfg_.long_press_ms);
        if (cfg_.on_long_press)
          cfg_.on_long_press();
      }
      if (held >= cfg_.power_press_ms) {
        power_fired_.store(true);
        if (cfg_.on_hold_progress)
          cfg_.on_hold_progress(0.0f);
        hold_reported_ = false;
        logger_.info("长按 {} ms -> on_power_press", cfg_.power_press_ms);
        if (cfg_.on_power_press)
          cfg_.on_power_press();
      }
    }
  }

private:
  static uint32_t now_ms() { return (uint32_t)(esp_timer_get_time() / 1000); }

  // 统一边沿处理:去抖 + 逻辑状态跟踪。espp::Button 是裸 ANY_EDGE 中断、没有去抖,
  // 机械触点抖动会产生一串假边沿。这里用两道防线滤掉:
  //   1) 时间去抖:与上一个【被接受】的边沿间隔 < debounce_ms 的,直接丢弃(抖动尖峰)
  //   2) 逻辑状态:只在去抖后的电平真正翻转时才当作一次按下/松开,重复同态边沿忽略
  // 这样"长按触发后松手的抖动"不会被误判成一次新的短按。
  void on_edge(bool active) {
    uint32_t t = now_ms();
    if (t - last_edge_ms_ < cfg_.debounce_ms)
      return; // 抖动,丢弃
    last_edge_ms_ = t;
    if (active == logical_pressed_)
      return; // 电平没真正翻转,忽略
    logical_pressed_ = active;
    if (active)
      on_press();
    else
      on_release();
  }

  void on_press() {
    press_start_ms_.store(now_ms());
    long_fired_.store(false);
    power_fired_.store(false);
  }

  void on_release() {
    uint32_t start = press_start_ms_.exchange(0);
    if (start == 0)
      return;
    if (long_fired_.load())
      return; // 长按已处理,松开不再触发短按
    uint32_t t = now_ms();
    uint32_t pending = pending_click_ms_.load();
    if (pending != 0 && t - pending <= cfg_.double_click_ms) {
      pending_click_ms_.store(0);
      logger_.debug("双击");
      if (cfg_.on_double_press)
        cfg_.on_double_press();
    } else {
      pending_click_ms_.store(t);
    }
  }

  Config cfg_;
  espp::Logger logger_;
  std::unique_ptr<espp::Button> button_;
  volatile uint32_t last_edge_ms_{0}; // 上一个被接受的边沿时刻(去抖用)
  bool logical_pressed_{false};       // 去抖后的逻辑按下状态
  bool hold_reported_{false};         // 上一轮是否报过长按进度(用于松手时收起进度环)
  std::atomic<uint32_t> press_start_ms_{0};
  std::atomic<bool> long_fired_{false};
  std::atomic<bool> power_fired_{false};
  std::atomic<uint32_t> pending_click_ms_{0};
};

} // namespace drivers
