#pragma once
// ============================================================================
// 系统钩子 (SystemHooks) —— UI 层调用硬件动作的窄接口
// ----------------------------------------------------------------------------
// Settings 等场景需要调硬件(调亮度/触觉强度/关机/切输入源),但 UI 层不应直接依赖
// bsp/drivers。这里提供一个全局函数指针集合,由 app_main 在启动时填好,UI 场景只调它。
// 依赖方向仍然干净:UI -> SystemHooks(纯函数指针) <- app_main 注入。
// ============================================================================
#include <functional>

namespace app {

struct SystemHooks {
  std::function<void(float)> set_brightness{};   // [0,1]
  std::function<void(float)> set_haptic_strength{}; // [0,1]
  std::function<float()> get_haptic_strength{};
  std::function<void()> test_haptic{};           // 播一下当前强度的触觉
  std::function<void()> shutdown{};
  std::function<bool()> hall_available{};        // AS5600 是否可用
  std::function<void(bool)> use_hall{};          // true=切霍尔,false=切串口模拟
  std::function<bool()> is_using_hall{};

  // 硬件诊断接口
  std::function<float()> get_encoder_angle{};    // 获取编码器角度 0-360°
  std::function<float()> get_encoder_rpm{};      // 获取编码器转速 rpm
  std::function<bool()> get_encoder_ok{};        // 编码器是否正常
  std::function<float()> get_battery_voltage{};  // 获取电池电压
  std::function<void(int)> play_haptic_waveform{}; // 播放指定波形ID

  static SystemHooks &instance() {
    static SystemHooks h;
    return h;
  }
};

} // namespace app
