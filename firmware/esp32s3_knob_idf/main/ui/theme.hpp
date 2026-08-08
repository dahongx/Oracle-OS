#pragma once
// ============================================================================
// Oracle OS 视觉主题 (UI Theme) —— 黑底金线
// ----------------------------------------------------------------------------
// 设计定位:高级、克制、神秘、精密。纯黑底 + 暖金/香槟金细线,留白充足。
// 设计规范给的色值是"米白暖调"示意版,这里按【黑底】重定义为"金色系线条在黑底上"。
// 全 UI 只从这里取色和字体,保证风格统一、便于整体调。
// ============================================================================
#include "display.hpp" // 引入 lvgl
#include "ui/fonts/han_fonts.hpp"

namespace ui::theme {

// ---- 色板(黑底金线)----
inline lv_color_t bg() { return lv_color_hex(0x0F1417); }
inline lv_color_t forest() { return lv_color_hex(0x1B2A1F); }
inline lv_color_t clover() { return lv_color_hex(0x4F8B46); }
inline lv_color_t clover_bright() { return lv_color_hex(0x8FCB75); }
inline lv_color_t gold() { return lv_color_hex(0xE7C35A); }
inline lv_color_t gold_bright() { return lv_color_hex(0xEFE8D5); }
inline lv_color_t gold_dim() { return lv_color_hex(0x6E5E3C); }
inline lv_color_t text() { return lv_color_hex(0xEFE8D5); }
inline lv_color_t text_dim() { return lv_color_hex(0x8A8270); }
inline lv_color_t cat_fur() { return lv_color_hex(0x12171C); }
inline lv_color_t cat_eye() { return lv_color_hex(0xD7BE75); }

// ---- 字体(中文子集字库,从 Fusion Pixel Font 生成)----
inline const lv_font_t *font_large() { return &han_24; }  // 标题/答案大字
inline const lv_font_t *font_normal() { return &han_16; } // 正文/提示

// 把某个屏幕对象刷成黑底(每个场景根对象都调一次)
inline void apply_bg(lv_obj_t *obj) {
  lv_obj_set_style_bg_color(obj, bg(), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_outline_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_style_radius(obj, 0, 0);
}

// 建一个金色标签的通用样式辅助
inline lv_obj_t *make_label(lv_obj_t *parent, const char *txt, const lv_font_t *font,
                            lv_color_t color) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color, 0);
  return l;
}

} // namespace ui::theme
