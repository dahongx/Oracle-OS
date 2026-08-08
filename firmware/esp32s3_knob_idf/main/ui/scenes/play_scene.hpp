#pragma once

#include <cmath>

#include "app/system_hooks.hpp"
#include "ui/cat_sprites.hpp"
#include "ui/ritual_clover.hpp"
#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class PlayScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);

    auto *title = theme::make_label(root, "逗猫时光", theme::font_normal(), theme::text());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    for (int i = 0; i < 4; ++i) {
      sparkles_[i] = theme::make_label(root, "+", theme::font_normal(), theme::gold());
      lv_obj_align(sparkles_[i], LV_ALIGN_CENTER, -8 + i * 28, -78 + (i % 2) * 25);
      lv_obj_set_style_text_opa(sparkles_[i], LV_OPA_30, 0);
    }

    cat_tail_ = cat_sprite::draw(root, cat_sprite::Pose::BARE_TAIL_LEFT, 78);
    lv_obj_align(cat_tail_, LV_ALIGN_CENTER, -69, 16);
    cat_sprite::set_pivot(cat_tail_, 82, 91);

    cat_ = cat_sprite::draw(root, cat_sprite::Pose::PLAY, 112);
    lv_obj_align(cat_, LV_ALIGN_CENTER, -50, 12);

    clover_.build(root, 58, -78, 38, 44, -43);
    clover_.widget().set_glow(0.12f);

    status_ = theme::make_label(root, "转动逗逗它", theme::font_normal(),
                                theme::clover_bright());
    lv_obj_align(status_, LV_ALIGN_BOTTOM_MID, 0, -8);
  }

  SceneId on_event(Event ev, float) override {
    if (ev == Event::BACK) {
      clover_.request_exit();
      returning_ = true;
      return SceneId::NONE;
    }
    if (!clover_.active() || clover_.exiting()) return SceneId::NONE;

    if (ev == Event::ROTATE_CW || ev == Event::ROTATE_CCW) {
      if (caught_) reset_round();
      direction_ = ev == Event::ROTATE_CW ? 1 : -1;
      step_++;
      goal_x_ = direction_ > 0 ? 69 : 5;
      goal_y_ = -45 + (step_ % 3) * 27;
      clover_angle_target_ += direction_ * 70.0f;
      if ((step_ % 3) == 0) pounce();
    } else if (ev == Event::PRESS) {
      pounce();
    }
    return SceneId::NONE;
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    last_ms_ = elapsed_ms;
    clover_.tick(elapsed_ms);
    if (returning_ && clover_.exit_done()) return SceneId::BACK;
    cat_sprite::set_rotation(cat_tail_, sinf(elapsed_ms / 880.0f) * 1.7f);
    if (!clover_.active()) return SceneId::NONE;

    toy_x_ += (goal_x_ - toy_x_) * 0.12f;
    toy_y_ += (goal_y_ - toy_y_) * 0.12f;
    clover_.set_target(static_cast<int>(toy_x_), static_cast<int>(toy_y_));
    clover_angle_ += (clover_angle_target_ - clover_angle_) * 0.16f;
    clover_.widget().set_angle(clover_angle_);

    for (int i = 0; i < 4; ++i) {
      float wave = (sinf(elapsed_ms / 330.0f + i * 1.5f) + 1.0f) * 0.5f;
      lv_obj_set_style_text_opa(sparkles_[i],
                                static_cast<lv_opa_t>(45 + wave * 135), 0);
    }

    if (pouncing_ && elapsed_ms - pounce_ms_ > 330) {
      pouncing_ = false;
      if (step_ >= 9) {
        caught_ = true;
        goal_x_ = -4;
        goal_y_ = 40;
        cat_sprite::set_pose(cat_, cat_sprite::Pose::CATCH);
        lv_label_set_text(status_, "抓到你啦");
        clover_.widget().set_glow(0.75f);
        if (app::SystemHooks::instance().test_haptic)
          app::SystemHooks::instance().test_haptic();
      } else {
        cat_sprite::set_pose(cat_, cat_sprite::Pose::PLAY);
      }
    }
    return SceneId::NONE;
  }

  const char *name() const override { return "CatPlay"; }

private:
  void pounce() {
    if (pouncing_ || caught_) return;
    pouncing_ = true;
    pounce_ms_ = last_ms_;
    cat_sprite::set_pose(cat_, cat_sprite::Pose::POUNCE);
    lv_label_set_text(status_, step_ > 5 ? "就差一点" : "它要扑啦");
  }

  void reset_round() {
    caught_ = false;
    pouncing_ = false;
    step_ = 0;
    goal_x_ = 44;
    goal_y_ = -40;
    cat_sprite::set_pose(cat_, cat_sprite::Pose::PLAY);
    lv_label_set_text(status_, "再玩一次");
    clover_.widget().set_glow(0.12f);
  }

  RitualClover clover_;
  lv_obj_t *cat_{nullptr};
  lv_obj_t *cat_tail_{nullptr};
  lv_obj_t *status_{nullptr};
  lv_obj_t *sparkles_[4]{};
  float toy_x_{44.0f};
  float toy_y_{-40.0f};
  int goal_x_{44};
  int goal_y_{-40};
  int direction_{1};
  int step_{0};
  float clover_angle_{0.0f};
  float clover_angle_target_{0.0f};
  uint32_t last_ms_{0};
  uint32_t pounce_ms_{0};
  bool pouncing_{false};
  bool caught_{false};
  bool returning_{false};
};

} // namespace ui
