#pragma once

#include <cmath>

#include "app/system_hooks.hpp"
#include "ui/cat_sprites.hpp"
#include "ui/ritual_clover.hpp"
#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class TriadScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);

    auto *title = theme::make_label(root, "揍走烦恼", theme::font_normal(), theme::text());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    cat_tail_ = cat_sprite::draw(root, cat_sprite::Pose::BARE_TAIL, 76);
    lv_obj_align(cat_tail_, LV_ALIGN_CENTER, -35, 31);
    cat_sprite::set_pivot(cat_tail_, 14, 91);

    cat_ = cat_sprite::draw(root, cat_sprite::Pose::RELAX, 104);
    lv_obj_align(cat_, LV_ALIGN_CENTER, -61, 21);

    clover_.build(root, 50, -77, 51, -7, 10);
    clover_.widget().set_glow(0.10f);

    build_worry(root);

    for (int i = 0; i < 5; ++i) {
      sparks_[i] = theme::make_label(root, "+", theme::font_normal(),
                                     i % 2 ? theme::gold_bright()
                                           : theme::clover_bright());
      lv_obj_align(sparks_[i], LV_ALIGN_CENTER,
                   43 + (i % 3) * 19, -42 + (i / 3) * 48);
      lv_obj_set_style_text_opa(sparks_[i], LV_OPA_TRANSP, 0);
    }

    status_ = theme::make_label(root, "短按出拳", theme::font_large(),
                                theme::gold_bright());
    lv_obj_align(status_, LV_ALIGN_BOTTOM_MID, 0, -8);
  }

  SceneId on_event(Event ev, float) override {
    if (ev == Event::BACK) {
      clover_.request_exit();
      returning_ = true;
      return SceneId::NONE;
    }
    if (!clover_.active() || clover_.exiting()) return SceneId::NONE;

    if (done_ &&
        (ev == Event::ROTATE_CW || ev == Event::ROTATE_CCW || ev == Event::PRESS)) {
      reset();
    } else if (ev == Event::ROTATE_CW || ev == Event::ROTATE_CCW ||
               ev == Event::PRESS) {
      hit();
    }
    return SceneId::NONE;
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    float dt = frame_ms_ == 0 ? 0.016f
                              : static_cast<float>(elapsed_ms - frame_ms_) / 1000.0f;
    frame_ms_ = elapsed_ms;
    if (dt > 0.1f) dt = 0.1f;
    last_ms_ = elapsed_ms;
    clover_.tick(elapsed_ms);
    if (returning_ && clover_.exit_done()) return SceneId::BACK;
    cat_sprite::set_rotation(cat_tail_, sinf(elapsed_ms / 900.0f) * 1.6f);
    if (!clover_.active()) return SceneId::NONE;

    clover_angle_ += clover_velocity_ * dt;
    clover_velocity_ *= expf(-dt / 0.22f);
    clover_.widget().set_angle(clover_angle_);

    if (swinging_ && elapsed_ms - hit_ms_ > 250) {
      swinging_ = false;
      cat_sprite::set_pose(cat_, done_ ? cat_sprite::Pose::RELAX
                                      : cat_sprite::Pose::SIT);
    }

    float spark_t = has_hit_
                        ? static_cast<float>(elapsed_ms - hit_ms_) / 360.0f
                        : 2.0f;
    lv_opa_t spark_opa =
        spark_t >= 0.0f && spark_t < 1.0f
            ? static_cast<lv_opa_t>((1.0f - spark_t) * 255)
            : static_cast<lv_opa_t>(LV_OPA_TRANSP);
    for (int i = 0; i < 5; ++i) {
      lv_obj_set_style_text_opa(sparks_[i], spark_opa, 0);
      if (spark_opa > 0)
        lv_obj_align(sparks_[i], LV_ALIGN_CENTER,
                     43 + (i % 3) * 19 + static_cast<int>(spark_t * (i - 2) * 4),
                     -42 + (i / 3) * 48 - static_cast<int>(spark_t * 12));
    }

    float glow_t = has_hit_
                       ? static_cast<float>(elapsed_ms - hit_ms_) / 280.0f
                       : 2.0f;
    clover_.widget().set_glow(glow_t >= 0.0f && glow_t < 1.0f
                                  ? 0.15f + (1.0f - glow_t) * 0.8f
                                  : 0.10f);
    return SceneId::NONE;
  }

  const char *name() const override { return "PunchWorry"; }

private:
  void build_worry(lv_obj_t *root) {
    worry_ = lv_obj_create(root);
    lv_obj_set_size(worry_, 68, 108);
    lv_obj_align(worry_, LV_ALIGN_CENTER, 64, -17);
    lv_obj_set_style_bg_opa(worry_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(worry_, 0, 0);
    lv_obj_set_style_pad_all(worry_, 0, 0);
    lv_obj_clear_flag(worry_, LV_OBJ_FLAG_SCROLLABLE);

    auto *head = lv_obj_create(worry_);
    lv_obj_set_size(head, 30, 30);
    lv_obj_align(head, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_radius(head, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(head, lv_color_hex(0x737B80), 0);
    lv_obj_set_style_bg_opa(head, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(head, 1, 0);
    lv_obj_set_style_border_color(head, theme::gold_dim(), 0);

    body_ = lv_obj_create(worry_);
    lv_obj_set_size(body_, 42, 52);
    lv_obj_align(body_, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_radius(body_, 5, 0);
    lv_obj_set_style_bg_color(body_, lv_color_hex(0x596268), 0);
    lv_obj_set_style_bg_opa(body_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(body_, 1, 0);
    lv_obj_set_style_border_color(body_, theme::gold_dim(), 0);

    auto *mark = theme::make_label(body_, "烦", theme::font_large(),
                                   theme::gold_bright());
    lv_obj_center(mark);

    for (int side = -1; side <= 1; side += 2) {
      auto *leg = lv_obj_create(worry_);
      lv_obj_set_size(leg, 10, 22);
      lv_obj_align(leg, LV_ALIGN_BOTTOM_MID, side * 12, 0);
      lv_obj_set_style_radius(leg, 3, 0);
      lv_obj_set_style_bg_color(leg, lv_color_hex(0x596268), 0);
      lv_obj_set_style_bg_opa(leg, LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(leg, 0, 0);
    }
  }

  void hit() {
    hits_++;
    has_hit_ = true;
    swinging_ = true;
    hit_ms_ = last_ms_;
    cat_sprite::set_pose(cat_, cat_sprite::Pose::SWIPE);
    clover_velocity_ += 760.0f;

    int shake = (hits_ % 2 ? 1 : -1) * (3 + hits_);
    lv_obj_align(worry_, LV_ALIGN_CENTER, 64 + shake, -17 + hits_ * 2);
    lv_obj_set_style_opa(worry_,
                         static_cast<lv_opa_t>(255 - (hits_ < 5 ? hits_ * 32 : 220)), 0);
    lv_label_set_text(status_, hits_ < 3 ? "再来一下" : "快跑掉了");

    if (app::SystemHooks::instance().test_haptic)
      app::SystemHooks::instance().test_haptic();
    if (hits_ >= 5) finish();
  }

  void finish() {
    done_ = true;
    lv_obj_add_flag(worry_, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(status_, "烦恼退散");
  }

  void reset() {
    hits_ = 0;
    has_hit_ = false;
    done_ = false;
    swinging_ = false;
    lv_obj_clear_flag(worry_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_opa(worry_, LV_OPA_COVER, 0);
    lv_obj_align(worry_, LV_ALIGN_CENTER, 64, -17);
    cat_sprite::set_pose(cat_, cat_sprite::Pose::RELAX);
    lv_label_set_text(status_, "短按出拳");
  }

  RitualClover clover_;
  lv_obj_t *cat_{nullptr};
  lv_obj_t *cat_tail_{nullptr};
  lv_obj_t *worry_{nullptr};
  lv_obj_t *body_{nullptr};
  lv_obj_t *status_{nullptr};
  lv_obj_t *sparks_[5]{};
  uint32_t last_ms_{0};
  uint32_t frame_ms_{0};
  uint32_t hit_ms_{0};
  float clover_angle_{0.0f};
  float clover_velocity_{0.0f};
  int hits_{0};
  bool swinging_{false};
  bool has_hit_{false};
  bool done_{false};
  bool returning_{false};
};

} // namespace ui
