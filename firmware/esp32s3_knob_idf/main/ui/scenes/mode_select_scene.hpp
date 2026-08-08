#pragma once

#include <array>

#include "ui/cat_sprites.hpp"
#include "ui/scene.hpp"
#include "ui/theme.hpp"

namespace ui {

class ModeSelectScene : public Scene {
public:
  struct Item {
    const char *label;
    SceneId scene;
  };

  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);
    items_ = {{
        {"今日幸运", SceneId::DAILY},
        {"逗猫时光", SceneId::PLAY},
        {"四叶呼吸", SceneId::ORBIT},
        {"吹走厄运", SceneId::QUICK_ASK},
        {"揍走烦恼", SceneId::TRIAD},
    }};

    auto *title = theme::make_label(root, "选择仪式", theme::font_large(), theme::gold());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);
    // Restore the exact old LOOK composition: a 48 px sprite centered inside
    // the historical 112 px layout box. The opaque first card is created
    // afterwards and naturally hides the body, leaving the familiar peeking
    // head and its small clover above the card.
    auto *cat = cat_sprite::draw(root, cat_sprite::Pose::MENU_PEEK, 48);
    lv_obj_set_size(cat, 112, 112);
    lv_obj_align(cat, LV_ALIGN_TOP_LEFT, 2, 0);

    // 固定高度的真实滚动视口。当前样机底部有较大的不可视安全区，
    // 因此一次完整显示三项，焦点移动时自动把目标项滚入视野。
    list_ = lv_obj_create(root);
    // Use the space previously occupied by operation hints for the content.
    // Four complete rows remain visible and the last row scrolls fully in.
    lv_obj_set_size(list_, 224, 177);
    lv_obj_align(list_, LV_ALIGN_TOP_MID, 0, 55);
    lv_obj_set_style_bg_opa(list_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_, 0, 0);
    lv_obj_set_style_pad_all(list_, 3, 0);
    lv_obj_set_style_radius(list_, 0, 0);
    lv_obj_set_scroll_dir(list_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list_, LV_SCROLLBAR_MODE_OFF);

    for (int i = 0; i < COUNT; ++i) {
      rows_[i] = lv_obj_create(list_);
      lv_obj_set_size(rows_[i], 206, 38);
      lv_obj_align(rows_[i], LV_ALIGN_TOP_MID, 0, i * 43);
      lv_obj_clear_flag(rows_[i], LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_style_radius(rows_[i], 5, 0);
      lv_obj_set_style_pad_all(rows_[i], 0, 0);

      marks_[i] = theme::make_label(rows_[i], "+", theme::font_normal(),
                                    theme::clover_bright());
      lv_obj_align(marks_[i], LV_ALIGN_LEFT_MID, 14, 0);
      labels_[i] = theme::make_label(rows_[i], items_[i].label, theme::font_normal(),
                                     theme::text());
      lv_obj_align(labels_[i], LV_ALIGN_LEFT_MID, 44, 0);
    }

    sel_ = 0;
    refresh();
  }

  SceneId on_event(Event ev) override {
    if (ev == Event::ROTATE_CW) {
      sel_ = (sel_ + 1) % COUNT;
      refresh();
    } else if (ev == Event::ROTATE_CCW) {
      sel_ = (sel_ - 1 + COUNT) % COUNT;
      refresh();
    } else if (ev == Event::PRESS) {
      return items_[sel_].scene;
    } else if (ev == Event::BACK) {
      return SceneId::BACK;
    }
    return SceneId::NONE;
  }

  const char *name() const override { return "ModeSelect"; }

private:
  void refresh() {
    for (int i = 0; i < COUNT; ++i) {
      bool selected = i == sel_;
      lv_obj_set_style_bg_color(rows_[i], selected ? theme::forest() : theme::bg(), 0);
      lv_obj_set_style_bg_opa(rows_[i], LV_OPA_COVER, 0);
      lv_obj_set_style_border_width(rows_[i], selected ? 2 : 1, 0);
      lv_obj_set_style_border_color(rows_[i],
                                    selected ? theme::gold() : theme::gold_dim(), 0);
      lv_obj_set_style_text_color(labels_[i],
                                  selected ? theme::gold_bright() : theme::text_dim(), 0);
      lv_obj_set_style_text_opa(marks_[i], selected ? LV_OPA_COVER : LV_OPA_40, 0);
    }
    if (rows_[sel_])
      lv_obj_scroll_to_view(rows_[sel_], LV_ANIM_ON);
  }

  static constexpr int COUNT = 5;
  std::array<Item, COUNT> items_{};
  std::array<lv_obj_t *, COUNT> rows_{};
  std::array<lv_obj_t *, COUNT> labels_{};
  std::array<lv_obj_t *, COUNT> marks_{};
  lv_obj_t *list_{nullptr};
  int sel_{0};
};

} // namespace ui
