#pragma once

#include <cmath>

#include "app/system_hooks.hpp"
#include "ui/cat_sprites.hpp"
#include "ui/ritual_clover.hpp"
#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class QuickAskScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);

    auto *title = theme::make_label(root, "吹走厄运", theme::font_normal(), theme::text());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    cat_ = cat_sprite::draw(root, cat_sprite::Pose::BLOW, 108);
    lv_obj_align(cat_, LV_ALIGN_CENTER, -58, 17);

    clover_.build(root, 64, -87, -7, 40, -27);
    clover_.widget().set_glow(0.08f);

    for (int i = 0; i < 3; ++i) {
      wind_[i] = theme::make_label(root, "—", theme::font_normal(),
                                    theme::clover_bright());
      lv_obj_align(wind_[i], LV_ALIGN_CENTER, -4 + i * 11, 2 - i * 6);
      lv_obj_set_style_text_opa(wind_[i], LV_OPA_TRANSP, 0);
    }

    status_ = theme::make_label(root, "转动吹散", theme::font_normal(),
                                theme::gold_bright());
    lv_obj_align(status_, LV_ALIGN_BOTTOM_MID, 0, -28);
  }

  SceneId on_event(Event ev, float strength) override {
    if (ev == Event::BACK) {
      clover_.request_exit();
      returning_ = true;
      return SceneId::NONE;
    }
    if (!clover_.active() || clover_.exiting()) return SceneId::NONE;

    if (done_ &&
        (ev == Event::ROTATE_CW || ev == Event::ROTATE_CCW || ev == Event::PRESS)) {
      reset();
      return SceneId::NONE;
    }
    if (ev == Event::ROTATE_CW || ev == Event::ROTATE_CCW || ev == Event::PRESS) {
      float dir = ev == Event::ROTATE_CCW ? -1.0f : 1.0f;
      float impulse = ev == Event::PRESS ? 1.3f : (strength < 0.7f ? 0.7f : strength);
      velocity_ += dir * impulse * 340.0f;
      if (velocity_ > 1500.0f) velocity_ = 1500.0f;
      if (velocity_ < -1500.0f) velocity_ = -1500.0f;
      progress_ += 0.17f * impulse;
      if (progress_ > 1.0f) progress_ = 1.0f;
      gust_ms_ = last_ms_;
      has_gust_ = true;
      if (progress_ >= 1.0f) finish();
    }
    return SceneId::NONE;
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    float dt = static_cast<float>(elapsed_ms - last_ms_) / 1000.0f;
    if (dt < 0.0f || dt > 0.1f) dt = 0.016f;
    last_ms_ = elapsed_ms;
    clover_.tick(elapsed_ms);
    if (returning_ && clover_.exit_done()) return SceneId::BACK;
    if (!clover_.active()) return SceneId::NONE;

    angle_ += velocity_ * dt;
    velocity_ *= expf(-dt / 0.75f);
    clover_.widget().set_angle(angle_);
    float force = fabsf(velocity_) / 900.0f;
    if (force > 1.0f) force = 1.0f;
    clover_.widget().set_glow(done_ ? 0.35f : 0.08f + force * 0.75f);

    float gust = has_gust_ && elapsed_ms - gust_ms_ < 330
                     ? 1.0f - static_cast<float>(elapsed_ms - gust_ms_) / 330.0f
                     : 0.0f;
    for (int i = 0; i < 3; ++i)
      lv_obj_set_style_text_opa(wind_[i],
                                static_cast<lv_opa_t>(gust * (230 - i * 35)), 0);
    return SceneId::NONE;
  }

  const char *name() const override { return "BlowAway"; }

private:
  void finish() {
    done_ = true;
    lv_label_set_text(status_, "坏运走啦");
    if (app::SystemHooks::instance().test_haptic)
      app::SystemHooks::instance().test_haptic();
  }

  void reset() {
    done_ = false;
    progress_ = 0.0f;
    velocity_ = 0.0f;
    has_gust_ = false;
    cat_sprite::set_pose(cat_, cat_sprite::Pose::BLOW);
    lv_label_set_text(status_, "转动吹散");
  }

  RitualClover clover_;
  lv_obj_t *cat_{nullptr};
  lv_obj_t *status_{nullptr};
  lv_obj_t *wind_[3]{};
  float angle_{0.0f};
  float velocity_{0.0f};
  float progress_{0.0f};
  uint32_t last_ms_{0};
  uint32_t gust_ms_{0};
  bool done_{false};
  bool has_gust_{false};
  bool returning_{false};
};

} // namespace ui
