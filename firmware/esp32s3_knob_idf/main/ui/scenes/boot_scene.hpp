#pragma once

#include <cmath>

#include "ui/scene.hpp"
#include "ui/cat_sprites.hpp"
#include "ui/spin_clover.hpp"
#include "ui/theme.hpp"

namespace ui {

class BootScene : public Scene {
public:
  static constexpr uint32_t FADE_MS = 900;
  static constexpr uint32_t HOLD_MS = 3800;

  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);

    static constexpr int stars[][2] = {
        {-82, -92}, {78, -82}, {-96, 10}, {91, 28}};
    for (auto &p : stars) {
      auto *star = theme::make_label(root, "+", theme::font_normal(), theme::gold());
      lv_obj_align(star, LV_ALIGN_CENTER, p[0], p[1]);
      lv_obj_set_style_text_opa(star, LV_OPA_50, 0);
    }

    clover_.build(root, 62);
    lv_obj_align(clover_.root(), LV_ALIGN_CENTER, 0, -70);
    clover_.set_glow(0.0f);
    clover_.set_scale(0.82f);
    lv_obj_set_style_opa(clover_.root(), LV_OPA_TRANSP, 0);

    mascot_ = cat_sprite::draw(root, cat_sprite::Pose::CATCH, 82);
    lv_obj_align(mascot_, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_opa(mascot_, LV_OPA_TRANSP, 0);

    title_ = theme::make_label(root, "ORACLE OS", theme::font_large(),
                               theme::gold_bright());
    lv_obj_align(title_, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_text_opa(title_, LV_OPA_TRANSP, 0);

    subtitle_ = theme::make_label(root, "掌心仪式挂件", theme::font_normal(),
                                  theme::clover_bright());
    lv_obj_align(subtitle_, LV_ALIGN_CENTER, 0, 90);
    lv_obj_set_style_text_opa(subtitle_, LV_OPA_TRANSP, 0);
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    float t = elapsed_ms >= FADE_MS ? 1.0f : (float)elapsed_ms / FADE_MS;
    // 柔和淡入 + 很轻的落位，不再使用任何圆形进度或能量条。
    float eased = 1.0f - (1.0f - t) * (1.0f - t);
    lv_opa_t opa = (lv_opa_t)(eased * 255.0f);
    if (clover_.root()) {
      lv_obj_set_style_opa(clover_.root(), opa, 0);
      clover_.set_scale(0.82f + eased * 0.18f);
      clover_.set_angle(sinf(elapsed_ms / 700.0f) * 2.0f);
    }
    if (title_) lv_obj_set_style_text_opa(title_, opa, 0);
    if (mascot_) lv_obj_set_style_opa(mascot_, opa, 0);
    if (subtitle_) {
      float sub_t = elapsed_ms < 300 ? 0.0f :
                    (elapsed_ms >= FADE_MS + 300 ? 1.0f :
                     (float)(elapsed_ms - 300) / FADE_MS);
      lv_obj_set_style_text_opa(subtitle_, (lv_opa_t)(sub_t * 255.0f), 0);
    }
    return elapsed_ms >= HOLD_MS ? SceneId::REPLACE_HOME : SceneId::NONE;
  }

  SceneId on_event(Event ev) override {
    if (ev == Event::PRESS || ev == Event::ROTATE_CW)
      return SceneId::REPLACE_HOME;
    return SceneId::NONE;
  }

  const char *name() const override { return "Boot"; }

private:
  SpinClover clover_;
  lv_obj_t *title_{nullptr};
  lv_obj_t *subtitle_{nullptr};
  lv_obj_t *mascot_{nullptr};
};

} // namespace ui
