#pragma once

#include <cmath>

#include "app/system_hooks.hpp"
#include "ui/cat_sprites.hpp"
#include "ui/ritual_clover.hpp"
#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class OrbitScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);

    auto *title = theme::make_label(root, "四叶呼吸", theme::font_normal(), theme::text());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    cue_ = theme::make_label(root, "跟着四叶草", theme::font_normal(), theme::text_dim());
    lv_obj_align(cue_, LV_ALIGN_TOP_MID, 0, 42);

    cat_tail_ = cat_sprite::draw(root, cat_sprite::Pose::BARE_TAIL, 78);
    lv_obj_align(cat_tail_, LV_ALIGN_CENTER, -34, 35);
    cat_sprite::set_pivot(cat_tail_, 14, 91);

    cat_ = cat_sprite::draw(root, cat_sprite::Pose::BREATHE, 104);
    lv_obj_align(cat_, LV_ALIGN_CENTER, -58, 33);

    clover_.build(root, 84, -76, 55, 36, -35);
    clover_.widget().set_glow(0.16f);

    status_ = theme::make_label(root, "准备一下", theme::font_large(),
                                theme::gold_bright());
    lv_obj_align(status_, LV_ALIGN_CENTER, 42, 62);

    for (int i = 0; i < 3; ++i) {
      dots_[i] = lv_obj_create(root);
      lv_obj_set_size(dots_[i], 7, 7);
      lv_obj_align(dots_[i], LV_ALIGN_BOTTOM_MID, (i - 1) * 18, -25);
      lv_obj_set_style_radius(dots_[i], LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_border_width(dots_[i], 0, 0);
      lv_obj_set_style_bg_color(dots_[i], theme::gold_dim(), 0);
      lv_obj_set_style_bg_opa(dots_[i], LV_OPA_COVER, 0);
    }
  }

  SceneId on_event(Event ev, float) override {
    if (ev == Event::BACK) {
      clover_.request_exit();
      returning_ = true;
    } else if (done_ &&
               (ev == Event::ROTATE_CW || ev == Event::ROTATE_CCW ||
                ev == Event::PRESS)) {
      restart(last_ms_);
    }
    return SceneId::NONE;
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    last_ms_ = elapsed_ms;
    clover_.tick(elapsed_ms);
    if (returning_ && clover_.exit_done()) return SceneId::BACK;
    cat_sprite::set_rotation(cat_tail_, sinf(elapsed_ms / 920.0f) * 1.5f);
    if (!clover_.active()) return SceneId::NONE;

    if (!started_) {
      started_ = true;
      breath_start_ms_ = elapsed_ms;
      lv_label_set_text(status_, "吸气");
    }
    if (done_) return SceneId::NONE;

    uint32_t run_ms = elapsed_ms - breath_start_ms_;
    int round = static_cast<int>(run_ms / CYCLE_MS);
    if (round >= 3) {
      finish();
      return SceneId::NONE;
    }

    for (int i = 0; i < 3; ++i)
      lv_obj_set_style_bg_color(dots_[i], i < round ? theme::clover_bright()
                                                    : theme::gold_dim(), 0);

    uint32_t within = run_ms % CYCLE_MS;
    int phase = 0;
    float scale = 0.84f;
    float glow = 0.12f;
    if (within < 4000) {
      phase = 0;
      float t = static_cast<float>(within) / 4000.0f;
      float smooth = 0.5f - 0.5f * cosf(t * 3.1415926f);
      scale = 0.84f + 0.28f * smooth;
      glow = 0.12f + 0.48f * smooth;
    } else if (within < 5000) {
      phase = 1;
      scale = 1.12f;
      glow = 0.60f;
    } else {
      phase = 2;
      float t = static_cast<float>(within - 5000) / 5000.0f;
      float smooth = 0.5f - 0.5f * cosf(t * 3.1415926f);
      scale = 1.12f - 0.28f * smooth;
      glow = 0.60f - 0.48f * smooth;
    }
    clover_.widget().set_scale(scale);
    clover_.widget().set_glow(glow);

    if (phase != phase_) {
      phase_ = phase;
      lv_label_set_text(status_, phase == 0 ? "吸气" : (phase == 1 ? "停一停" : "呼气"));
      if (app::SystemHooks::instance().test_haptic)
        app::SystemHooks::instance().test_haptic();
    }
    return SceneId::NONE;
  }

  const char *name() const override { return "CloverBreath"; }

private:
  static constexpr uint32_t CYCLE_MS = 10000;

  void finish() {
    done_ = true;
    for (auto *dot : dots_)
      lv_obj_set_style_bg_color(dot, theme::clover_bright(), 0);
    clover_.widget().set_scale(1.0f);
    clover_.widget().set_glow(0.30f);
    cat_sprite::set_pose(cat_, cat_sprite::Pose::RELAX);
    lv_label_set_text(cue_, "慢慢回来");
    lv_label_set_text(status_, "舒服一点");
    if (app::SystemHooks::instance().test_haptic)
      app::SystemHooks::instance().test_haptic();
  }

  void restart(uint32_t now_ms) {
    done_ = false;
    started_ = true;
    breath_start_ms_ = now_ms;
    phase_ = -1;
    cat_sprite::set_pose(cat_, cat_sprite::Pose::BREATHE);
    lv_label_set_text(cue_, "跟着四叶草");
    for (auto *dot : dots_)
      lv_obj_set_style_bg_color(dot, theme::gold_dim(), 0);
  }

  RitualClover clover_;
  lv_obj_t *cat_{nullptr};
  lv_obj_t *cat_tail_{nullptr};
  lv_obj_t *cue_{nullptr};
  lv_obj_t *status_{nullptr};
  lv_obj_t *dots_[3]{};
  uint32_t last_ms_{0};
  uint32_t breath_start_ms_{0};
  int phase_{-1};
  bool started_{false};
  bool done_{false};
  bool returning_{false};
};

} // namespace ui
