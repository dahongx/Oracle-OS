#include "ui/scenes/hardware_diag_scene.hpp"
#include <cstdio>

namespace ui {

void HardwareDiagScene::on_enter(lv_obj_t *root) {
  theme::apply_bg(root);
  root_ = root;

  // 标题
  title_ = theme::make_label(root, "硬件诊断", theme::font_normal(), theme::gold_bright());
  lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, 8);

  // Two compact columns keep every diagnostic value inside the 240 px panel.
  // The previous single column ended below y=240.
  // === 编码器区域 ===
  encoder_label_ = theme::make_label(root, "编码器:", theme::font_normal(), theme::gold());
  lv_obj_align(encoder_label_, LV_ALIGN_TOP_LEFT, 8, 32);

  angle_label_ = theme::make_label(root, "角度: --", theme::font_normal(), theme::text_dim());
  lv_obj_align(angle_label_, LV_ALIGN_TOP_LEFT, 8, 52);

  rpm_label_ = theme::make_label(root, "转速: --", theme::font_normal(), theme::text_dim());
  lv_obj_align(rpm_label_, LV_ALIGN_TOP_LEFT, 8, 72);

  magnet_label_ = theme::make_label(root, "磁铁: --", theme::font_normal(), theme::text_dim());
  lv_obj_align(magnet_label_, LV_ALIGN_TOP_LEFT, 8, 92);

  // === 按键区域 ===
  button_label_ = theme::make_label(root, "按键:", theme::font_normal(), theme::gold());
  lv_obj_align(button_label_, LV_ALIGN_TOP_LEFT, 8, 118);

  button_status_ = theme::make_label(root, "状态: 正常", theme::font_normal(), theme::text_dim());
  lv_obj_align(button_status_, LV_ALIGN_TOP_LEFT, 8, 138);

  // === 震动测试区域 ===
  haptic_label_ = theme::make_label(root, "震动测试 (旋转选择):", theme::font_normal(), theme::gold());
  lv_obj_set_width(haptic_label_, 106);
  lv_obj_align(haptic_label_, LV_ALIGN_TOP_LEFT, 126, 32);

  haptic_hint_ = theme::make_label(root, "> [1] TICK (轻)", theme::font_normal(), theme::gold_bright());
  lv_obj_set_width(haptic_hint_, 106);
  lv_obj_align(haptic_hint_, LV_ALIGN_TOP_LEFT, 126, 72);

  // === 电池区域 ===
  battery_label_ = theme::make_label(root, "电池:", theme::font_normal(), theme::gold());
  lv_obj_align(battery_label_, LV_ALIGN_TOP_LEFT, 126, 122);

  battery_voltage_ = theme::make_label(root, "电压: --V", theme::font_normal(), theme::text_dim());
  lv_obj_align(battery_voltage_, LV_ALIGN_TOP_LEFT, 126, 144);

  // === 屏幕信息 ===
  screen_label_ = theme::make_label(root, "屏幕: 240×240 RGB565", theme::font_normal(), theme::text_dim());
  lv_obj_set_width(screen_label_, 106);
  lv_obj_align(screen_label_, LV_ALIGN_TOP_LEFT, 126, 174);

  // 底部提示
  lv_obj_t *hint = theme::make_label(root, "短按测试震动 · 长按返回", theme::font_normal(), theme::gold());
  lv_obj_set_width(hint, 230);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -5);
}

SceneId HardwareDiagScene::on_event(Event ev, float strength) {
  switch (ev) {
  case Event::POWER:
    return SceneId::NONE;
  case Event::ROTATE_CW:
    haptic_select_ = (haptic_select_ + 1) % 5;
    update_haptic_selection();
    return SceneId::NONE;

  case Event::ROTATE_CCW:
    haptic_select_ = (haptic_select_ - 1 + 5) % 5;
    update_haptic_selection();
    return SceneId::NONE;

  case Event::PRESS:
    // 短按播放当前选中的震动
    play_haptic_test(haptic_select_);
    return SceneId::NONE;

  case Event::BACK:
    return SceneId::BACK;
  }
  return SceneId::NONE;
}

SceneId HardwareDiagScene::on_tick(uint32_t elapsed_ms) {
  // 每100ms更新一次数据
  if (elapsed_ms - last_update_ms_ < 100) {
    return SceneId::NONE;
  }
  last_update_ms_ = elapsed_ms;

  auto &hooks = app::SystemHooks::instance();

  // 更新编码器数据
  if (hooks.get_encoder_angle && hooks.get_encoder_rpm && hooks.get_encoder_ok) {
    char buf[64];

    // 角度
    float angle = hooks.get_encoder_angle();
    snprintf(buf, sizeof(buf), "角度: %.1f°", angle);
    lv_label_set_text(angle_label_, buf);

    // 转速
    float rpm = hooks.get_encoder_rpm();
    snprintf(buf, sizeof(buf), "转速: %.1f rpm", rpm);
    lv_label_set_text(rpm_label_, buf);

    // 磁铁状态
    bool ok = hooks.get_encoder_ok();
    lv_label_set_text(magnet_label_, ok ? "磁铁: 正常" : "磁铁: 未检测到");
  } else {
    lv_label_set_text(angle_label_, "角度: hooks未设置");
    lv_label_set_text(rpm_label_, "转速: --");
    lv_label_set_text(magnet_label_, "磁铁: --");
  }

  // 更新电池电压
  if (hooks.get_battery_voltage) {
    float voltage = hooks.get_battery_voltage();
    char buf[64];
    snprintf(buf, sizeof(buf), "电压: %.2fV", voltage);
    lv_label_set_text(battery_voltage_, buf);
  } else {
    lv_label_set_text(battery_voltage_, "电压: hooks未设置");
  }

  return SceneId::NONE;
}

void HardwareDiagScene::update_haptic_selection() {
  const char *names[] = {
    "> [1] TICK (轻)",
    "> [2] ENTER (中低)",
    "> [3] REVEAL (中)",
    "> [4] CANCEL (双击)",
    "> [5] POWER (长)",
  };
  lv_label_set_text(haptic_hint_, names[haptic_select_]);
}

void HardwareDiagScene::play_haptic_test(int index) {
  // 临时波形ID (DRV2605L内置波形)
  // 后续需要在HapticManager中定义标准波形
  const uint8_t waveforms[] = {
    1,   // TICK - Sharp Click 100%
    10,  // ENTER - Soft Bump 100%
    47,  // REVEAL - Long Double Sharp Click Strong 1 100%
    48,  // CANCEL - Long Double Sharp Click Strong 2 100%
    49,  // POWER - Long Double Sharp Click Strong 3 100%
  };

  auto &hooks = app::SystemHooks::instance();
  if (hooks.play_haptic_waveform && index >= 0 && index < 5) {
    hooks.play_haptic_waveform(waveforms[index]);
  }
}

} // namespace ui
