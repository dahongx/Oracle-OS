#pragma once
// ============================================================================
// 可旋转四叶草 (SpinClover) —— 小游戏的"指尖陀螺"核心 widget
// ----------------------------------------------------------------------------
// 一层金色能量光环(随转速变亮/微胀) + 猫尾巴同款像素四叶草。
// 全系统只使用这一种四叶草视觉，避免尾巴与小游戏风格不一致。
//   build(parent, px)  建好并居中
//   set_angle(deg)     设置旋转角(度,自动归一化)
//   set_glow(0..1)     能量光环强度(转速映射)
// 由 SpinScene 逐帧驱动。
// ============================================================================
#include <cmath>

#include "ui/cat_sprites.hpp"
#include "ui/theme.hpp"

namespace ui {

class SpinClover {
public:
  // display_px:四叶草显示直径(源图 100px,非 100 时按比例缩放)。
  void build(lv_obj_t *parent, int display_px = 100) {
    ensure_dsc();
    disp_px_ = display_px;

    // 透明容器,留出光环扩张空间
    cont_ = lv_obj_create(parent);
    const int box = (int)(display_px * 1.55f);
    lv_obj_set_size(cont_, box, box);
    lv_obj_set_style_bg_opa(cont_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont_, 0, 0);
    lv_obj_set_style_pad_all(cont_, 0, 0);
    lv_obj_set_style_radius(cont_, 0, 0);
    lv_obj_clear_flag(cont_, LV_OBJ_FLAG_SCROLLABLE);

    // 金色能量光环(细环,平时隐形,转速越高越亮越胀)
    halo_base_ = (int)(display_px * 1.22f);
    halo_ = lv_obj_create(cont_);
    lv_obj_set_size(halo_, halo_base_, halo_base_);
    lv_obj_center(halo_);
    lv_obj_set_style_radius(halo_, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(halo_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(halo_, theme::gold(), 0);
    lv_obj_set_style_border_width(halo_, 2, 0);
    lv_obj_set_style_border_opa(halo_, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(halo_, LV_OBJ_FLAG_SCROLLABLE);

    // 四叶草图像
    img_ = lv_image_create(cont_);
    lv_image_set_src(img_, &dsc_);
    lv_image_set_antialias(img_, true);
    lv_image_set_pivot(img_, cat_sprite::SIZE / 2, cat_sprite::SIZE / 2);
    if (display_px != cat_sprite::SIZE)
      lv_image_set_scale(img_, (uint32_t)(256 * display_px / cat_sprite::SIZE));
    lv_obj_center(img_);
  }

  void set_angle(float deg) {
    angle_deg_ = deg;
    float a = fmodf(deg, 360.0f);
    if (a < 0) a += 360.0f;
    if (img_) lv_image_set_rotation(img_, (int32_t)(a * 10.0f));
  }
  float angle() const { return angle_deg_; }

  // k: 0..1 -> 光环透明度 + 轻微胀大,模拟蓄能发光
  void set_glow(float k) {
    if (k < 0.0f) k = 0.0f;
    if (k > 1.0f) k = 1.0f;
    if (halo_) {
      lv_obj_set_style_border_opa(halo_, (lv_opa_t)(k * 220.0f), 0);
      int sz = halo_base_ + (int)(k * disp_px_ * 0.25f);
      lv_obj_set_size(halo_, sz, sz);
      lv_obj_center(halo_);
    }
  }

  // 缩放脉冲(揭示时"绽放"用):factor 1.0 = 原始大小
  void set_scale(float factor) {
    if (!img_) return;
    uint32_t base = (uint32_t)(256 * disp_px_ / cat_sprite::SIZE);
    lv_image_set_scale(img_, (uint32_t)(base * factor));
  }

  lv_obj_t *root() { return cont_; }
  lv_obj_t *image() { return img_; }

private:
  static void ensure_dsc();
  static inline lv_image_dsc_t dsc_{};
  static inline bool dsc_ready_ = false;

  lv_obj_t *cont_{nullptr};
  lv_obj_t *halo_{nullptr};
  lv_obj_t *img_{nullptr};
  int disp_px_{100};
  int halo_base_{135};
  float angle_deg_{0.0f};
};

// 构造共享图像描述符(所有实例共用同一张图)。逐字段赋值,避免 C++20 指定
// 初始化器对位域顺序的限制。
inline void SpinClover::ensure_dsc() {
  if (dsc_ready_) return;
  dsc_.header.magic = LV_IMAGE_HEADER_MAGIC;
  dsc_.header.cf = LV_COLOR_FORMAT_ARGB8888;
  dsc_.header.flags = 0;
  dsc_.header.w = cat_sprite::SIZE;
  dsc_.header.h = cat_sprite::SIZE;
  dsc_.header.stride = cat_sprite::SIZE * 4;
  dsc_.header.reserved_2 = 0;
  dsc_.data_size = (uint32_t)cat_sprite::SIZE * cat_sprite::SIZE * 4;
  dsc_.data = cat_clover_map;
  dsc_ready_ = true;
}

}  // namespace ui
