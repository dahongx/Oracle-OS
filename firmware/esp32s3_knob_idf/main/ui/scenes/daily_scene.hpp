#pragma once

#include <cmath>
#include <cstdio>

#include "app/daily_luck_store.hpp"
#include "app/system_hooks.hpp"
#include "ui/cat_sprites.hpp"
#include "ui/ritual_clover.hpp"
#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class DailyScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);
    const auto luck = app::DailyLuckStore::load(COLOR_COUNT);
    lucky_number_ = luck.number;
    lucky_color_ = luck.color_index;

    auto *title = theme::make_label(root, "今日幸运", theme::font_normal(), theme::text());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    clover_.build(root, 54, -96, 70, 0, -70);
    clover_.widget().set_glow(0.18f);

    // One calm result card, then a small companion cat in its own clear area.
    // No text is squeezed beside the cat.
    cat_tail_ = cat_sprite::draw(root, cat_sprite::Pose::BARE_TAIL_LEFT, 64);
    lv_obj_align(cat_tail_, LV_ALIGN_TOP_LEFT, 3, 168);
    cat_sprite::set_pivot(cat_tail_, 82, 91);

    cat_ = cat_sprite::draw(root, cat_sprite::Pose::RELAX, 66);
    lv_obj_align(cat_, LV_ALIGN_TOP_LEFT, 3, 167);

    panel_ = lv_obj_create(root);
    lv_obj_set_size(panel_, 216, 66);
    lv_obj_align(panel_, LV_ALIGN_TOP_MID, 0, 96);
    lv_obj_set_style_bg_color(panel_, lv_color_hex(0x111E23), 0);
    lv_obj_set_style_bg_opa(panel_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel_, 1, 0);
    lv_obj_set_style_border_color(panel_, theme::gold_dim(), 0);
    lv_obj_set_style_radius(panel_, 8, 0);
    lv_obj_set_style_pad_all(panel_, 0, 0);
    lv_obj_clear_flag(panel_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_opa(panel_, LV_OPA_TRANSP, 0);

    auto *divider = lv_obj_create(panel_);
    lv_obj_set_size(divider, 1, 46);
    lv_obj_center(divider);
    lv_obj_set_style_bg_color(divider, theme::gold_dim(), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_50, 0);
    lv_obj_set_style_border_width(divider, 0, 0);

    auto *number_label = theme::make_label(panel_, "数字", theme::font_normal(),
                                           theme::text_dim());
    lv_obj_align(number_label, LV_ALIGN_TOP_LEFT, 21, 7);
    char number[4];
    std::snprintf(number, sizeof(number), "%d", lucky_number_);
    number_ = theme::make_label(panel_, number, theme::font_large(),
                                theme::gold_bright());
    lv_obj_align(number_, LV_ALIGN_BOTTOM_LEFT, 47, -5);

    auto *color_label = theme::make_label(panel_, "颜色", theme::font_normal(),
                                          theme::text_dim());
    lv_obj_align(color_label, LV_ALIGN_TOP_RIGHT, -25, 7);

    swatch_ = lv_obj_create(panel_);
    lv_obj_set_size(swatch_, 18, 18);
    lv_obj_align(swatch_, LV_ALIGN_BOTTOM_RIGHT, -70, -8);
    lv_obj_set_style_radius(swatch_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(swatch_, lv_color_hex(COLORS[lucky_color_].rgb), 0);
    lv_obj_set_style_bg_opa(swatch_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(swatch_, 1, 0);
    lv_obj_set_style_border_color(swatch_, theme::gold_bright(), 0);
    lv_obj_clear_flag(swatch_, LV_OBJ_FLAG_SCROLLABLE);

    color_ = theme::make_label(panel_, COLORS[lucky_color_].name,
                               theme::font_normal(), theme::gold_bright());
    lv_obj_align(color_, LV_ALIGN_BOTTOM_RIGHT, -10, -9);
  }

  SceneId on_event(Event ev, float) override {
    if (ev == Event::BACK) {
      clover_.request_exit();
      returning_ = true;
    } else if (revealed_ &&
               (ev == Event::ROTATE_CW || ev == Event::ROTATE_CCW ||
                ev == Event::PRESS)) {
      bloom_start_ms_ = last_ms_;
    }
    return SceneId::NONE;
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    last_ms_ = elapsed_ms;
    clover_.tick(elapsed_ms);
    if (returning_ && clover_.exit_done()) return SceneId::BACK;

    // Only the tail sways; the cat body and head stay completely fixed.
    cat_sprite::set_rotation(cat_tail_, sinf(elapsed_ms / 850.0f) * 1.5f);

    if (!revealed_ && clover_.active()) {
      revealed_ = true;
      reveal_ms_ = elapsed_ms;
      bloom_start_ms_ = elapsed_ms;
      if (app::SystemHooks::instance().test_haptic)
        app::SystemHooks::instance().test_haptic();
    }

    if (revealed_) {
      float t = static_cast<float>(elapsed_ms - reveal_ms_) / 520.0f;
      if (t > 1.0f) t = 1.0f;
      lv_obj_set_style_opa(panel_, static_cast<lv_opa_t>(255.0f * t), 0);

      float bt = static_cast<float>(elapsed_ms - bloom_start_ms_) / 430.0f;
      float pop = 0.0f;
      if (bt >= 0.0f && bt < 1.0f)
        pop = sinf(bt * 3.1415926f);
      clover_.widget().set_scale(1.0f + 0.10f * pop);
      clover_.widget().set_glow(0.18f + 0.55f * pop);
    }
    return SceneId::NONE;
  }

  const char *name() const override { return "DailyLuck"; }

private:
  struct LuckyColor {
    const char *name;
    uint32_t rgb;
  };
  static constexpr LuckyColor COLORS[] = {
      {"薄荷绿", 0x8FCB75}, {"天空蓝", 0x72BBDD}, {"蜜桃粉", 0xE99BB5},
      {"琥珀黄", 0xE7C35A}, {"奶油白", 0xEFE8D5}, {"青苹果", 0x74B95A},
      {"薰衣紫", 0xA991D4},
  };
  static constexpr int COLOR_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);

  RitualClover clover_;
  lv_obj_t *panel_{nullptr};
  lv_obj_t *number_{nullptr};
  lv_obj_t *color_{nullptr};
  lv_obj_t *swatch_{nullptr};
  lv_obj_t *cat_{nullptr};
  lv_obj_t *cat_tail_{nullptr};
  int lucky_number_{1};
  int lucky_color_{0};
  uint32_t last_ms_{0};
  uint32_t reveal_ms_{0};
  uint32_t bloom_start_ms_{0};
  bool revealed_{false};
  bool returning_{false};
};

} // namespace ui
