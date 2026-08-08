#pragma once
// ============================================================================
// 设置 (Settings)
// ----------------------------------------------------------------------------
// 旋转在若干设置项间切换焦点,短按调整/执行:
//   Brightness  : 短按循环 4 档亮度
//   Haptic      : 短按循环 4 档触觉强度(并试震一下)
//   Input       : 短按在 霍尔 / 串口模拟 间切换(霍尔不可用时提示)
//   Power Off   : 短按关机
// 长按返回。硬件动作通过 app::SystemHooks 调用,UI 不直接依赖驱动。
// ============================================================================
#include "ui/scene.hpp"
#include "ui/theme.hpp"
#include "app/system_hooks.hpp"

namespace ui {

class SettingsScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);

    lv_obj_t *title = theme::make_label(root, "设置", theme::font_normal(), theme::text_dim());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    static const char *names[NUM] = {"亮度", "触觉", "输入", "关机"};
    for (int i = 0; i < NUM; i++) {
      rows_[i] = theme::make_label(root, names[i], theme::font_normal(), theme::gold_dim());
      lv_obj_align(rows_[i], LV_ALIGN_CENTER, -40, i * 24 - 34);
      vals_[i] = theme::make_label(root, "", theme::font_normal(), theme::text_dim());
      lv_obj_align(vals_[i], LV_ALIGN_CENTER, 70, i * 24 - 34);
    }

    hint_ = theme::make_label(root, "转动移动 · 按下更改", theme::font_normal(), theme::gold());
    lv_obj_align(hint_, LV_ALIGN_BOTTOM_MID, 0, -14);

    // 初值
    bright_ = 3; // 0..3
    apply_brightness();
    refresh_vals();
    refresh_focus();
  }

  SceneId on_event(Event ev) override {
    switch (ev) {
    case Event::POWER:
      return SceneId::NONE;
    case Event::ROTATE_CW:
      sel_ = (sel_ + 1) % NUM;
      refresh_focus();
      return SceneId::NONE;
    case Event::ROTATE_CCW:
      sel_ = (sel_ - 1 + NUM) % NUM;
      refresh_focus();
      return SceneId::NONE;
    case Event::PRESS:
      activate();
      return SceneId::NONE;
    case Event::BACK:
      return SceneId::BACK;
    }
    return SceneId::NONE;
  }

  const char *name() const override { return "Settings"; }

private:
  static constexpr int NUM = 4;

  void apply_brightness() {
    float b = 0.25f * (bright_ + 1); // 0.25 / 0.5 / 0.75 / 1.0
    auto &h = app::SystemHooks::instance();
    if (h.set_brightness)
      h.set_brightness(b);
  }

  void activate() {
    auto &h = app::SystemHooks::instance();
    switch (sel_) {
    case 0: // Brightness
      bright_ = (bright_ + 1) % 4;
      apply_brightness();
      break;
    case 1: // Haptic
      haptic_ = (haptic_ + 1) % 4;
      if (h.set_haptic_strength)
        h.set_haptic_strength(0.25f * (haptic_ + 1));
      if (h.test_haptic)
        h.test_haptic();
      break;
    case 2: // Input
      if (h.hall_available && h.hall_available()) {
        bool toHall = !(h.is_using_hall && h.is_using_hall());
        if (h.use_hall)
          h.use_hall(toHall);
      } else {
        lv_label_set_text(hint_, "霍尔未就绪 · 磁铁待调");
      }
      break;
    case 3: // Power Off
      if (h.shutdown)
        h.shutdown();
      break;
    }
    refresh_vals();
  }

  void refresh_vals() {
    static const char *lvl[4] = {"25%", "50%", "75%", "100%"};
    lv_label_set_text(vals_[0], lvl[bright_]);
    lv_label_set_text(vals_[1], lvl[haptic_]);
    auto &h = app::SystemHooks::instance();
    bool hall = h.is_using_hall && h.is_using_hall();
    lv_label_set_text(vals_[2], hall ? "霍尔" : "模拟");
    lv_label_set_text(vals_[3], ">");
  }

  void refresh_focus() {
    for (int i = 0; i < NUM; i++)
      lv_obj_set_style_text_color(rows_[i], i == sel_ ? theme::gold_bright() : theme::gold_dim(), 0);
  }

  lv_obj_t *rows_[NUM]{};
  lv_obj_t *vals_[NUM]{};
  lv_obj_t *hint_{nullptr};
  int sel_{0};
  int bright_{3};
  int haptic_{3};
};

} // namespace ui
