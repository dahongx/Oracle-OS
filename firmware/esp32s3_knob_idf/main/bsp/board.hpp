#pragma once
// ============================================================================
// 板级支持层 (BSP - Board)
// ----------------------------------------------------------------------------
// 唯一"知道器件接在哪个引脚、按什么顺序上电"的地方。按 pins.hpp 构建所有驱动,
// 对上层只暴露驱动接口指针(IInputSource* / IHaptic* / 各驱动对象)。
// 上层(app/ui)只拿接口,不碰引脚,也不碰 espp 底层。
// ============================================================================
#include <memory>

#include "logger.hpp"

#include "drivers/as5600_encoder.hpp"
#include "drivers/battery.hpp"
#include "drivers/button_input.hpp"
#include "drivers/display_panel.hpp"
#include "drivers/haptic.hpp"
#include "drivers/input.hpp"
#include "drivers/power.hpp"

namespace bsp {

// 板级持有的所有驱动。生命周期 = 整个程序。
class Board {
public:
  Board();

  // ---- 驱动接口访问(给 app / ui 层用)----
  drivers::Power &power() { return *power_; }
  drivers::Drv2605Haptic &haptic() { return *haptic_; }
  drivers::Battery &battery() { return *battery_; }
  drivers::DisplayPanel &display() { return *display_; }
  drivers::SerialSource &serial_source() { return serial_source_; }
  drivers::As5600Encoder *encoder() { return encoder_.get(); } // 可能为空(磁铁没调好)

  // 按键回调在构造后由 app 注入(需要用到 app 层的服务)
  void set_button_callbacks(std::function<void()> on_short, std::function<void()> on_double,
                            std::function<void()> on_long,
                            std::function<void()> on_power,
                            std::function<void(float)> on_hold_progress = {});
  void poll_button() {
    if (button_)
      button_->poll();
  }

private:
  espp::Logger logger_;

  std::unique_ptr<drivers::Power> power_;
  std::unique_ptr<drivers::Drv2605Haptic> haptic_;
  std::unique_ptr<drivers::As5600Encoder> encoder_;
  std::unique_ptr<drivers::Battery> battery_;
  std::unique_ptr<drivers::DisplayPanel> display_;
  std::unique_ptr<drivers::ButtonInput> button_;

  drivers::SerialSource serial_source_; // 串口模拟源(常驻)

  std::function<void()> on_short_;
  std::function<void()> on_double_;
  std::function<void()> on_long_;
  std::function<void()> on_power_;
  std::function<void(float)> on_hold_progress_;
};

} // namespace bsp
