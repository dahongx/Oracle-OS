#pragma once
// ============================================================================
// 串口命令控制台 (App Layer - Serial Console)
// ----------------------------------------------------------------------------
// 从 stdin(USB-Serial/JTAG 控制台)读单字符命令,驱动【串口模拟输入源】,
// 并可在真实霍尔 / 串口模拟之间切换。霍尔物理含义 = 磁转子的角度/转速/方向,
// 这里用键盘字符模拟这两个量。
//
// 命令:
//   a/d = 逆/顺时针转一格    A/D = 快转5格
//   f/s = 快/慢转速          0 = 停转    z = 角度清零
//   H = 切真实霍尔  S = 切串口模拟  ? = 帮助
// ============================================================================
#include <functional>
#include <memory>

#include "logger.hpp"
#include "task.hpp"

#include "drivers/input.hpp"

namespace app {

class SerialConsole {
public:
  struct Config {
    drivers::SerialSource *sim;      // 被驱动的串口模拟源
    float detent_step_deg{15.0f};
    // 切换到真实霍尔 / 串口模拟的回调(由 app 接到 KnobService::set_source)
    std::function<void()> on_switch_to_hall{};
    std::function<void()> on_switch_to_sim{};
    std::function<void()> on_shutdown{}; // 关机(命令 Q)
    std::function<void()> on_press{};    // 短按=确认/进入(命令 p)
    std::function<void()> on_back{};     // 长按=返回/取消(命令 b)
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::INFO};
  };

  explicit SerialConsole(const Config &config);
  void start();

  // 供按键/其他模块复用:模拟顺时针转一格
  void sim_step_cw();
  void print_help();

private:
  bool poll(std::mutex &m, std::condition_variable &cv);
  void handle_char(int c);

  Config cfg_;
  espp::Logger logger_;
  std::unique_ptr<espp::Task> task_;
};

} // namespace app
