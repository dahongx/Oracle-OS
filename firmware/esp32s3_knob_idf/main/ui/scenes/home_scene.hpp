#pragma once

#include <cmath>

#include "ui/cat_sprites.hpp"
#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class HomeScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);

    static constexpr int stars[][2] = {
        {-86, -96}, {-54, -78}, {72, -92}, {92, -52}, {-92, -18}, {82, 18}};
    for (auto &p : stars) {
      auto *star = theme::make_label(root, "+", theme::font_normal(), theme::gold());
      lv_obj_align(star, LV_ALIGN_CENTER, p[0], p[1]);
    }

    // The tail is a separate layer. The cat body never floats or rotates.
    // Use the original proportionate short tail without horizontal stretching.
    // Its larger clover sits outside the right ear and the base remains hidden
    // behind the body.
    tail_ = cat_sprite::draw(root, cat_sprite::Pose::TAIL, 112);
    lv_obj_align(tail_, LV_ALIGN_CENTER, 40, 18);
    cat_sprite::set_pivot(tail_, 44, 92);

    cat_ = cat_sprite::draw(root, cat_sprite::Pose::SIT, 154);
    if (cat_)
      lv_obj_align(cat_, LV_ALIGN_CENTER, -16, -2);
  }

  SceneId on_event(Event ev) override {
    if (ev == Event::PRESS || ev == Event::ROTATE_CW || ev == Event::ROTATE_CCW)
      return SceneId::MODE_SELECT;
    if (ev == Event::POWER)
      return SceneId::POWER_CONFIRM;
    return SceneId::NONE;
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    // 每约 6 秒自然眨一次眼：睁眼 -> 半闭 -> 闭眼 -> 半闭 -> 睁眼。
    uint32_t blink = elapsed_ms % 6000;
    cat_sprite::Pose pose = cat_sprite::Pose::SIT;
    if (blink >= 4550 && blink < 4640) pose = cat_sprite::Pose::BLINK;
    else if (blink >= 4640 && blink < 4740) pose = cat_sprite::Pose::CLOSED;
    else if (blink >= 4740 && blink < 4830) pose = cat_sprite::Pose::BLINK;
    if (pose != pose_) {
      cat_sprite::set_pose(cat_, pose);
      pose_ = pose;
    }

    // A brief tail-tip sway every few seconds; it rests between gestures.
    uint32_t tail_cycle = elapsed_ms % 5200;
    float tail_angle = 0.0f;
    if (tail_cycle > 3650 && tail_cycle < 4700) {
      float t = static_cast<float>(tail_cycle - 3650) / 1050.0f;
      tail_angle = sinf(t * 2.0f * static_cast<float>(M_PI)) *
                   sinf(t * static_cast<float>(M_PI)) * 0.9f;
    }
    cat_sprite::set_rotation(tail_, tail_angle);

    return SceneId::NONE;
  }

  const char *name() const override { return "Home"; }

private:
  lv_obj_t *cat_{nullptr};
  lv_obj_t *tail_{nullptr};
  cat_sprite::Pose pose_{cat_sprite::Pose::SIT};
};

} // namespace ui
