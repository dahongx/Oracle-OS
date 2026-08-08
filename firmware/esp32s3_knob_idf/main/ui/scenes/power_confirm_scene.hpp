#pragma once

#include <cmath>

#include "app/system_hooks.hpp"
#include "ui/cat_sprites.hpp"
#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class PowerConfirmScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);

    // Keep the complete confirmation UI inside the 240 px physical panel.
    tail_ = cat_sprite::draw(root, cat_sprite::Pose::TAIL, 80);
    lv_obj_align(tail_, LV_ALIGN_TOP_MID, 30, 23);
    cat_sprite::set_pivot(tail_, 18, 82);
    cat_ = cat_sprite::draw(root, cat_sprite::Pose::BREATHE, 112);
    lv_obj_align(cat_, LV_ALIGN_TOP_MID, -10, 2);

    auto *title = theme::make_label(root, "要休息了吗", theme::font_large(),
                                    theme::gold_bright());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 126);

    auto *line = lv_obj_create(root);
    lv_obj_set_size(line, 96, 1);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 159);
    lv_obj_set_style_bg_color(line, theme::gold_dim(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);

    auto *confirm = theme::make_label(root, "短按关机", theme::font_normal(),
                                      theme::clover_bright());
    lv_obj_align(confirm, LV_ALIGN_TOP_MID, 0, 172);

    auto *cancel = theme::make_label(root, "长按取消", theme::font_normal(),
                                     theme::text_dim());
    lv_obj_align(cancel, LV_ALIGN_TOP_MID, 0, 205);
  }

  SceneId on_event(Event ev) override {
    if (ev == Event::PRESS || ev == Event::ROTATE_CW) {
      auto &hooks = app::SystemHooks::instance();
      if (hooks.play_haptic_waveform) hooks.play_haptic_waveform(1);
      if (hooks.shutdown) hooks.shutdown();
      return SceneId::NONE;
    }
    if (ev == Event::BACK || ev == Event::POWER)
      return SceneId::BACK;
    return SceneId::NONE;
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    if (tail_)
      cat_sprite::set_rotation(tail_, sinf(elapsed_ms / 900.0f) * 0.7f);
    return SceneId::NONE;
  }

  const char *name() const override { return "PowerConfirm"; }

private:
  lv_obj_t *cat_{nullptr};
  lv_obj_t *tail_{nullptr};
};

} // namespace ui
