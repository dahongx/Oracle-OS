#pragma once
// ============================================================================
// 旋钮应用服务 (App Layer - Knob Service)
// ----------------------------------------------------------------------------
// 纯业务逻辑,只依赖【驱动接口】(IInputSource / IHaptic),不碰任何寄存器或引脚:
//   - 周期读取当前输入源的角度/转速
//   - 检测虚拟"格位"(detent)跨越 -> 触发触觉 + 通过回调通知 UI
//   - 支持在【真实霍尔】和【串口模拟】输入源之间运行时切换
// UI 通过注册回调(on_angle / on_detent)接收更新,服务不依赖具体 UI 类。
// ============================================================================
#include <atomic>
#include <functional>
#include <memory>

#include "logger.hpp"
#include "task.hpp"

#include "drivers/haptic.hpp"
#include "drivers/input.hpp"

namespace app {

class KnobService {
public:
  struct Config {
    drivers::IInputSource *initial_source; // 初始输入源(通常是串口模拟)
    drivers::IHaptic *haptic;
    float detent_step_deg{15.0f};
    // UI 更新回调(线程安全由 UI 实现保证):
    std::function<void(float raw_deg, int deg_0_360)> on_angle{};
    std::function<void(int detent_count)> on_detent{};
    // 过格步进:direction=+1 顺时针/-1 逆时针;strength≈1 为慢速,快转时 >1
    // (由当前转速 rpm 映射,体现"速度越快能量越强")。UI 用它做滚动焦点+能量反馈。
    std::function<void(int direction, float strength)> on_step{};
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::INFO};
  };

  explicit KnobService(const Config &config);

  void start();

  // 运行时切换输入源(磁铁调好后切到真实霍尔)。传 nullptr 忽略。
  void set_source(drivers::IInputSource *source);
  drivers::IInputSource *source() const { return source_.load(); }

  int detent_count() const { return detent_count_.load(); }

private:
  bool tick(std::mutex &m, std::condition_variable &cv);

  Config cfg_;
  espp::Logger logger_;
  std::atomic<drivers::IInputSource *> source_;
  std::atomic<int> detent_count_{0};
  float last_detent_boundary_{0.0f};
  std::unique_ptr<espp::Task> task_;
};

} // namespace app
