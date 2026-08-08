#pragma once
// ============================================================================
// 硬件诊断场景 (HardwareDiagScene)
// ----------------------------------------------------------------------------
// 用于验证所有硬件功能是否正常:
//   - AS5600磁编码器: 实时显示角度/转速/磁铁状态
//   - 按键: 显示按下状态和长按进度
//   - 震动: 5个测试按钮
//   - 电池: 显示电压和电量
//   - 屏幕: 显示分辨率和配置
//
// 通过SystemHooks访问硬件,不直接依赖bsp/drivers
// ============================================================================
#include "ui/scene.hpp"
#include "ui/theme.hpp"
#include "app/system_hooks.hpp"

namespace ui {

class HardwareDiagScene : public Scene {
public:
  HardwareDiagScene() = default;

  void on_enter(lv_obj_t *root) override;
  SceneId on_event(Event ev, float strength) override;
  SceneId on_tick(uint32_t elapsed_ms) override;
  const char *name() const override { return "HardwareDiag"; }

private:
  void update_haptic_selection();
  void play_haptic_test(int index);

  // UI元素
  lv_obj_t *root_{nullptr};
  lv_obj_t *title_{nullptr};

  // 编码器显示
  lv_obj_t *encoder_label_{nullptr};
  lv_obj_t *angle_label_{nullptr};
  lv_obj_t *rpm_label_{nullptr};
  lv_obj_t *magnet_label_{nullptr};

  // 按键显示
  lv_obj_t *button_label_{nullptr};
  lv_obj_t *button_status_{nullptr};

  // 震动测试
  lv_obj_t *haptic_label_{nullptr};
  lv_obj_t *haptic_hint_{nullptr};
  int haptic_select_{0}; // 当前选中的震动测试 0-4

  // 电池显示
  lv_obj_t *battery_label_{nullptr};
  lv_obj_t *battery_voltage_{nullptr};

  // 屏幕信息
  lv_obj_t *screen_label_{nullptr};

  uint32_t last_update_ms_{0};
};

} // namespace ui
