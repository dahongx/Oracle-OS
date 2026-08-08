#pragma once
// ============================================================================
// 情绪罗盘 (Mood Compass)
// ----------------------------------------------------------------------------
// 旋转定位情绪 -> 罗盘指针在 6 个方位刻度间转动,当前方位刻度点高亮 ->
// 短按锁定,中心显示情绪词 + 底部一句平衡建议。长按返回。视觉:黑金罗盘,克制。
// ============================================================================
#include <cmath>

#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class MoodScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);
    root_ = root;

    lv_obj_t *title = theme::make_label(root, "情绪", theme::font_normal(), theme::text_dim());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    // 罗盘外圈
    ring_ = lv_obj_create(root);
    lv_obj_set_size(ring_, RADIUS * 2, RADIUS * 2);
    lv_obj_align(ring_, LV_ALIGN_CENTER, 0, -6);
    lv_obj_set_style_radius(ring_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring_, 1, 0);
    lv_obj_set_style_border_color(ring_, theme::gold_dim(), 0);
    lv_obj_clear_flag(ring_, LV_OBJ_FLAG_SCROLLABLE);

    // 6 个方位刻度点
    for (int i = 0; i < NUM; i++) {
      ticks_[i] = lv_obj_create(root);
      lv_obj_set_size(ticks_[i], 5, 5);
      lv_obj_set_style_radius(ticks_[i], LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_bg_color(ticks_[i], theme::gold_dim(), 0);
      lv_obj_set_style_bg_opa(ticks_[i], LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(ticks_[i], 0, 0);
      float a = angle_of(i);
      int x = (int)(std::cos(a) * RADIUS);
      int y = (int)(std::sin(a) * RADIUS);
      lv_obj_align(ticks_[i], LV_ALIGN_CENTER, x, y - 6);
    }

    // 指针(中心指向当前方位的一个亮点)
    needle_ = lv_obj_create(root);
    lv_obj_set_size(needle_, 9, 9);
    lv_obj_set_style_radius(needle_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(needle_, theme::gold_bright(), 0);
    lv_obj_set_style_bg_opa(needle_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(needle_, 0, 0);

    mood_ = theme::make_label(root, "", theme::font_large(), theme::gold_bright());
    lv_obj_align(mood_, LV_ALIGN_CENTER, 0, -6);

    hint_ = theme::make_label(root, "转动感应 · 按下锁定", theme::font_normal(), theme::gold());
    lv_obj_align(hint_, LV_ALIGN_BOTTOM_MID, 0, -16);

    update_compass();
  }

  SceneId on_event(Event ev) override {
    switch (ev) {
    case Event::POWER:
      return SceneId::NONE;
    case Event::ROTATE_CW:
      sector_ = (sector_ + 1) % NUM;
      locked_ = false;
      lv_label_set_text(mood_, "");
      lv_label_set_text(hint_, "转动感应 · 按下锁定");
      update_compass();
      return SceneId::NONE;
    case Event::ROTATE_CCW:
      sector_ = (sector_ - 1 + NUM) % NUM;
      locked_ = false;
      lv_label_set_text(mood_, "");
      lv_label_set_text(hint_, "转动感应 · 按下锁定");
      update_compass();
      return SceneId::NONE;
    case Event::PRESS:
      locked_ = true;
      lv_label_set_text(mood_, MOODS[sector_]);
      lv_label_set_text(hint_, ADVICE[sector_]);
      return SceneId::NONE;
    case Event::BACK:
      return SceneId::BACK;
    }
    return SceneId::NONE;
  }

  const char *name() const override { return "Mood"; }

private:
  static constexpr int RADIUS = 54;
  static constexpr int NUM = 6;
  static constexpr const char *MOODS[NUM] = {"平静", "紧绷", "明朗", "低落", "躁动", "舒展"};
  static constexpr const char *ADVICE[NUM] = {"安住此刻", "缓缓呼气", "与人分享",
                                              "温柔以待", "沉稳落地", "向前一步"};

  float angle_of(int i) const {
    return (float)i / NUM * 2.0f * (float)M_PI - (float)M_PI / 2.0f; // 从正上方起
  }

  void update_compass() {
    // 高亮当前扇区刻度,其余暗;指针移到当前方位内圈
    for (int i = 0; i < NUM; i++)
      lv_obj_set_style_bg_color(ticks_[i], i == sector_ ? theme::gold_bright() : theme::gold_dim(), 0);
    float a = angle_of(sector_);
    int x = (int)(std::cos(a) * (RADIUS - 16));
    int y = (int)(std::sin(a) * (RADIUS - 16));
    lv_obj_align(needle_, LV_ALIGN_CENTER, x, y - 6);
  }

  lv_obj_t *root_{nullptr};
  lv_obj_t *ring_{nullptr};
  lv_obj_t *ticks_[NUM]{};
  lv_obj_t *needle_{nullptr};
  lv_obj_t *mood_{nullptr};
  lv_obj_t *hint_{nullptr};
  int sector_{0};
  bool locked_{false};
};

} // namespace ui
