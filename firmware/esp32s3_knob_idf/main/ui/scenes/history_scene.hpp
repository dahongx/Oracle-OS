#pragma once
// ============================================================================
// 收藏历史 (History / Vault)
// ----------------------------------------------------------------------------
// 浏览已收藏的仪式结果:旋转翻看条目(符号 + 文字 + 来源),显示"第 i/共 n 条"。
// 空时提示"nothing kept yet"。长按返回。数据来自 app::Vault 单例。
// ============================================================================
#include "ui/scene.hpp"
#include "ui/theme.hpp"
#include "ui/symbol.hpp"
#include "app/vault.hpp"

namespace ui {

class HistoryScene : public Scene {
public:
  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);
    root_ = root;

    lv_obj_t *title = theme::make_label(root, "收藏", theme::font_normal(), theme::text_dim());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    text_ = theme::make_label(root, "", theme::font_large(), theme::gold_bright());
    lv_obj_set_width(text_, 220);
    lv_obj_set_style_text_align(text_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(text_, LV_ALIGN_CENTER, 0, 6);

    from_ = theme::make_label(root, "", theme::font_normal(), theme::text_dim());
    lv_obj_align(from_, LV_ALIGN_CENTER, 0, 34);

    counter_ = theme::make_label(root, "", theme::font_normal(), theme::gold());
    lv_obj_align(counter_, LV_ALIGN_BOTTOM_MID, 0, -16);

    idx_ = 0;
    refresh();
  }

  SceneId on_event(Event ev) override {
    int n = app::Vault::instance().count();
    switch (ev) {
    case Event::ROTATE_CW:
      if (n > 0) { idx_ = (idx_ + 1) % n; refresh(); }
      return SceneId::NONE;
    case Event::ROTATE_CCW:
      if (n > 0) { idx_ = (idx_ - 1 + n) % n; refresh(); }
      return SceneId::NONE;
    case Event::BACK:
      return SceneId::BACK;
    default:
      return SceneId::NONE;
    }
  }

  const char *name() const override { return "History"; }

private:
  void clear_symbol() {
    if (symbol_) { lv_obj_del(symbol_); symbol_ = nullptr; }
  }

  void refresh() {
    clear_symbol();
    auto &vault = app::Vault::instance();
    int n = vault.count();
    if (n == 0) {
      lv_label_set_text(text_, "尚无收藏");
      lv_label_set_text(from_, "");
      lv_label_set_text(counter_, "完成一次仪式 而后收藏");
      return;
    }
    app::VaultEntry e;
    if (!vault.get(idx_, e))
      return;
    if (e.symbol >= 0) {
      symbol_ = symbol::draw(root_, e.symbol, 40);
      lv_obj_align(symbol_, LV_ALIGN_CENTER, 0, -40);
    }
    lv_label_set_text(text_, e.text);
    lv_label_set_text(from_, e.from);
    char buf[24];
    snprintf(buf, sizeof(buf), "%d / %d", idx_ + 1, n);
    lv_label_set_text(counter_, buf);
  }

  lv_obj_t *root_{nullptr};
  lv_obj_t *text_{nullptr};
  lv_obj_t *from_{nullptr};
  lv_obj_t *counter_{nullptr};
  lv_obj_t *symbol_{nullptr};
  int idx_{0};
};

} // namespace ui
