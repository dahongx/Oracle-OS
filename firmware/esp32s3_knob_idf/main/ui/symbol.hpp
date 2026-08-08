#pragma once
// ============================================================================
// 神谕符号 (Oracle Symbols) —— 精致版
// ----------------------------------------------------------------------------
// 按设计图的"极简星图/星光/轨道/罗盘/四叶草"气质,用 LVGL 基本元素(细线折线 +
// 圆环 + 光点)组合出 6 枚神谕符号,黑底暖金细线。全部径向对称,天然居中。
//   0 星光 sparkle(四角星 ✦)   1 轨道 orbit(环+行星+中心)
//   2 星座 constellation(连线星图) 3 罗盘 compass(同心圆+十字刻度)
//   4 四叶 clover(四瓣)          5 太阳 sun(中心+八芒)
// 每个符号建在一个 size×size 的透明容器里,返回容器(调用方负责切换时 lv_obj_del)。
// ============================================================================
#include <cmath>
#include <cstdint>

#include "ui/theme.hpp"

namespace ui::symbol {

static constexpr int COUNT = 6;

inline lv_obj_t *draw(lv_obj_t *parent, int idx, int size) {
  idx = ((idx % COUNT) + COUNT) % COUNT;
  const int S = size;
  const int c = S / 2;

  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_size(cont, S, S);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_radius(cont, 0, 0);
  lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

  // 共享细线样式(暖金,宽 2,圆角端)
  static lv_style_t line_st;
  static bool line_init = false;
  if (!line_init) {
    lv_style_init(&line_st);
    lv_style_set_line_width(&line_st, 2);
    lv_style_set_line_color(&line_st, theme::gold_bright());
    lv_style_set_line_rounded(&line_st, true);
    line_init = true;
  }

  // 折线(点坐标为容器内绝对坐标)
  auto add_line = [&](lv_point_precise_t *pts, uint32_t n) {
    lv_obj_t *ln = lv_line_create(cont);
    lv_line_set_points(ln, pts, n);
    lv_obj_add_style(ln, &line_st, 0);
    lv_obj_set_pos(ln, 0, 0);
    return ln;
  };
  // 圆环(描边圆),d=直径,居中偏移 (dx,dy)
  auto add_ring = [&](int d, int dx, int dy, lv_color_t col, int w) {
    lv_obj_t *o = lv_obj_create(cont);
    lv_obj_set_size(o, d, d);
    lv_obj_align(o, LV_ALIGN_CENTER, dx, dy);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, w, 0);
    lv_obj_set_style_border_color(o, col, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
  };
  // 实心光点,d=直径,居中偏移
  auto add_dot = [&](int d, int dx, int dy, lv_color_t col) {
    lv_obj_t *o = lv_obj_create(cont);
    lv_obj_set_size(o, d, d);
    lv_obj_align(o, LV_ALIGN_CENTER, dx, dy);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(o, col, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(o, 0, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    return o;
  };

  switch (idx) {
  case 0: { // 星光 ✦:四角凹星(8 点凹多边形),四角另点缀小星
    int R = c;             // 外尖到边
    int ri = (int)(c * 0.34f); // 内凹半径
    static lv_point_precise_t p[9];
    p[0] = {(lv_value_precise_t)c, (lv_value_precise_t)(c - R)};
    p[1] = {(lv_value_precise_t)(c + ri), (lv_value_precise_t)(c - ri)};
    p[2] = {(lv_value_precise_t)(c + R), (lv_value_precise_t)c};
    p[3] = {(lv_value_precise_t)(c + ri), (lv_value_precise_t)(c + ri)};
    p[4] = {(lv_value_precise_t)c, (lv_value_precise_t)(c + R)};
    p[5] = {(lv_value_precise_t)(c - ri), (lv_value_precise_t)(c + ri)};
    p[6] = {(lv_value_precise_t)(c - R), (lv_value_precise_t)c};
    p[7] = {(lv_value_precise_t)(c - ri), (lv_value_precise_t)(c - ri)};
    p[8] = {(lv_value_precise_t)c, (lv_value_precise_t)(c - R)};
    add_line(p, 9);
    break;
  }
  case 1: { // 轨道:外环 + 环上一颗行星 + 中心点
    add_ring(S - 2, 0, 0, theme::gold(), 2);
    int orbit_r = (S - 2) / 2;
    add_dot(4, 0, 0, theme::gold_bright());                          // 中心
    add_dot(9, (int)(orbit_r * 0.71f), -(int)(orbit_r * 0.71f), theme::gold_bright()); // 行星(右上)
    break;
  }
  case 2: { // 星座:连线星图(折线 + 各顶点光点)
    static lv_point_precise_t p[5];
    p[0] = {(lv_value_precise_t)(int)(S * 0.12f), (lv_value_precise_t)(int)(S * 0.62f)};
    p[1] = {(lv_value_precise_t)(int)(S * 0.36f), (lv_value_precise_t)(int)(S * 0.30f)};
    p[2] = {(lv_value_precise_t)(int)(S * 0.58f), (lv_value_precise_t)(int)(S * 0.52f)};
    p[3] = {(lv_value_precise_t)(int)(S * 0.80f), (lv_value_precise_t)(int)(S * 0.22f)};
    p[4] = {(lv_value_precise_t)(int)(S * 0.90f), (lv_value_precise_t)(int)(S * 0.66f)};
    add_line(p, 5);
    for (int i = 0; i < 5; i++) {
      int dx = (int)p[i].x - c;
      int dy = (int)p[i].y - c;
      add_dot(i == 1 ? 8 : 6, dx, dy, theme::gold_bright());
    }
    break;
  }
  case 3: { // 罗盘:同心双环 + 四向十字刻度点
    add_ring(S - 2, 0, 0, theme::gold(), 2);
    add_ring((int)(S * 0.42f), 0, 0, theme::gold_dim(), 2);
    int e = (S - 2) / 2 - 1;
    add_dot(5, 0, -e, theme::gold_bright());
    add_dot(5, 0, e, theme::gold_bright());
    add_dot(5, -e, 0, theme::gold_bright());
    add_dot(5, e, 0, theme::gold_bright());
    break;
  }
  case 4: { // 四叶:四瓣(四个环)+ 中心点
    int o = (int)(S * 0.22f);
    int leaf = (int)(S * 0.42f);
    add_ring(leaf, 0, -o, theme::gold_bright(), 2);
    add_ring(leaf, 0, o, theme::gold_bright(), 2);
    add_ring(leaf, -o, 0, theme::gold_bright(), 2);
    add_ring(leaf, o, 0, theme::gold_bright(), 2);
    add_dot(5, 0, 0, theme::gold_bright());
    break;
  }
  default: { // 5 太阳:中心圆 + 八芒(八颗小点)
    add_ring((int)(S * 0.34f), 0, 0, theme::gold_bright(), 2);
    int rr = (int)(c * 0.82f);
    for (int i = 0; i < 8; i++) {
      float a = i * (float)M_PI / 4.0f;
      int dx = (int)(std::cos(a) * rr);
      int dy = (int)(std::sin(a) * rr);
      add_dot(4, dx, dy, theme::gold_bright());
    }
    break;
  }
  }
  return cont;
}

} // namespace ui::symbol
