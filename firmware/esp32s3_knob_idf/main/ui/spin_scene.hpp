#pragma once
// ============================================================================
// 旋转动效引擎 (SpinScene) —— 所有小游戏共用的"指尖陀螺"骨架
// ----------------------------------------------------------------------------
// 五拍流程,子类只填内容(标题/引导语/结果集),动效与手感全在基类:
//   入场    : 猫坐着,中心一颗可旋转四叶草静候
//   旋转    : 拨滚轮 -> 四叶草获得角速度并旋转,光环随转速变亮(带惯性)
//   落定    : 松手后角速度按摩擦指数衰减,慢到阈值 -> 自然停下
//   揭示    : 四叶草绽放(缩放脉冲)+ 结果文字淡入(纯随机抽取)
//   收尾    : 短按收藏(若可)/ 再转重来 / 长按返回
//
// 物理用真实 dt(相邻帧 elapsed 差)积分,和帧率解耦。角速度单位 度/秒。
// ============================================================================
#include <cmath>
#include <cstdint>

#include "esp_random.h"

#include "app/vault.hpp"
#include "app/system_hooks.hpp"
#include "ui/cat_sprites.hpp"
#include "ui/scene.hpp"
#include "ui/spin_clover.hpp"
#include "ui/theme.hpp"

namespace ui {

// 一条结果:主词(大字) + 副句(小字,可空)。
struct SpinResult {
  const char *text;
  const char *sub;
};

class SpinScene : public Scene {
public:
  // ---- 子类需提供的内容 ----
  virtual const char *title() const = 0;          // 顶部标题(游戏名)
  virtual const char *prompt() const = 0;         // 待转引导语
  virtual const SpinResult *results(int &n) const = 0;  // 结果集 + 数量
  virtual const char *vault_tag() const { return nullptr; } // 非空则可收藏

  void on_enter(lv_obj_t *root) override {
    theme::apply_bg(root);
    root_ = root;

    title_ = theme::make_label(root, title(), theme::font_normal(), theme::text_dim());
    lv_obj_align(title_, LV_ALIGN_TOP_MID, 0, 14);

    clover_.build(root, CLOVER_PX);
    lv_obj_align(clover_.root(), LV_ALIGN_CENTER, 24, -38);

    mascot_ = cat_sprite::draw(root, cat_sprite::Pose::BLOW, 84);
    lv_obj_align(mascot_, LV_ALIGN_CENTER, -70, 40);

    for (int i = 0; i < 3; ++i) {
      wind_[i] = theme::make_label(root, "~", theme::font_normal(), theme::text());
      lv_obj_align(wind_[i], LV_ALIGN_CENTER, -28 + i * 17, 9 + (i % 2) * 12);
      lv_obj_set_style_text_opa(wind_[i], LV_OPA_TRANSP, 0);

      clouds_[i] = lv_obj_create(root);
      lv_obj_set_size(clouds_[i], 16 + i * 5, 13 + i * 4);
      lv_obj_align(clouds_[i], LV_ALIGN_CENTER, 62 + (i % 2) * 17, -65 + i * 28);
      lv_obj_set_style_radius(clouds_[i], LV_RADIUS_CIRCLE, 0);
      lv_obj_set_style_bg_color(clouds_[i], lv_color_hex(0x657078), 0);
      lv_obj_set_style_bg_opa(clouds_[i], LV_OPA_70, 0);
      lv_obj_set_style_border_width(clouds_[i], 0, 0);
    }

    answer_ = theme::make_label(root, "", theme::font_large(), theme::gold_bright());
    lv_obj_set_width(answer_, 130);
    lv_obj_set_style_text_align(answer_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(answer_, LV_ALIGN_CENTER, 45, 52);
    lv_obj_set_style_text_opa(answer_, LV_OPA_TRANSP, 0);

    sub_ = theme::make_label(root, "", theme::font_normal(), theme::gold());
    lv_obj_set_width(sub_, 130);
    lv_obj_set_style_text_align(sub_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(sub_, LV_ALIGN_CENTER, 45, 79);
    lv_obj_set_style_text_opa(sub_, LV_OPA_TRANSP, 0);

    hint_ = theme::make_label(root, prompt(), theme::font_normal(), theme::gold());
    lv_obj_set_width(hint_, 218);
    lv_obj_set_style_text_align(hint_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(hint_, LV_ALIGN_BOTTOM_MID, 0, -4);
  }

  SceneId on_event(Event ev, float strength) override {
    switch (ev) {
    case Event::POWER:
      return SceneId::NONE;
    case Event::ROTATE_CW:
    case Event::ROTATE_CCW: {
      if (phase_ == Phase::REVEAL)
        begin_spin();                       // 揭示后再转 = 重新来一次
      float dir = (ev == Event::ROTATE_CW) ? 1.0f : -1.0f;
      vel_ += dir * strength * IMPULSE;     // 注入角速度(速度反馈)
      if (vel_ > VEL_MAX) vel_ = VEL_MAX;
      if (vel_ < -VEL_MAX) vel_ = -VEL_MAX;
      has_spun_ = true;
      return SceneId::NONE;
    }
    case Event::PRESS:
      if (phase_ == Phase::REVEAL)
        toggle_save();
      return SceneId::NONE;
    case Event::BACK:
      return SceneId::BACK;
    }
    return SceneId::NONE;
  }

  SceneId on_tick(uint32_t elapsed_ms) override {
    float dt = (elapsed_ms - last_ms_) / 1000.0f;
    last_ms_ = elapsed_ms;
    if (dt <= 0.0f) return SceneId::NONE;
    if (dt > 0.10f) dt = 0.10f;             // 卡顿保护

    if (phase_ == Phase::SPIN) {
      // 角速度积分 + 指数摩擦衰减
      angle_ += vel_ * dt;
      vel_ *= expf(-dt / FRICTION_TAU);
      clover_.set_angle(angle_);
      clover_.set_glow(clampf(fabsf(vel_) / GLOW_FULL, 0.0f, 1.0f));
      float force = clampf(fabsf(vel_) / GLOW_FULL, 0.0f, 1.0f);
      for (int i = 0; i < 3; ++i) {
        lv_obj_set_style_text_opa(wind_[i], (lv_opa_t)(force * 230), 0);
        lv_obj_set_style_bg_opa(clouds_[i], (lv_opa_t)(180 * (1.0f - force)), 0);
      }

      // 转过之后慢到阈值 -> 自然落定并揭示
      if (has_spun_ && fabsf(vel_) < SETTLE_VEL) {
        vel_ = 0.0f;
        reveal(elapsed_ms);
      }
    } else if (phase_ == Phase::REVEAL) {
      // 绽放:缩放 1.0 -> BLOOM_PEAK -> 1.0,同时文字淡入
      float t = (elapsed_ms - reveal_ms_) / (float)BLOOM_MS;
      if (t < 1.0f) {
        float pop = (t < 0.5f) ? (t / 0.5f) : (1.0f - (t - 0.5f) / 0.5f); // 0..1..0
        clover_.set_scale(1.0f + pop * (BLOOM_PEAK - 1.0f));
        lv_opa_t opa = (lv_opa_t)(t * 255.0f);
        lv_obj_set_style_text_opa(answer_, opa, 0);
        lv_obj_set_style_text_opa(sub_, opa, 0);
      } else if (!bloom_done_) {
        clover_.set_scale(1.0f);
        lv_obj_set_style_text_opa(answer_, LV_OPA_COVER, 0);
        lv_obj_set_style_text_opa(sub_, LV_OPA_COVER, 0);
        bloom_done_ = true;
      }
    }
    return SceneId::NONE;
  }

private:
  enum class Phase { SPIN, REVEAL };

  void begin_spin() {
    phase_ = Phase::SPIN;
    vel_ = 0.0f;
    has_spun_ = false;
    saved_ = false;
    bloom_done_ = false;
    clover_.set_scale(1.0f);
    clover_.set_glow(0.0f);
    for (auto *w : wind_) lv_obj_set_style_text_opa(w, LV_OPA_TRANSP, 0);
    for (auto *c : clouds_) {
      lv_obj_clear_flag(c, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_style_bg_opa(c, LV_OPA_70, 0);
    }
    lv_obj_set_style_text_opa(answer_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_opa(sub_, LV_OPA_TRANSP, 0);
    lv_label_set_text(answer_, "");
    lv_label_set_text(sub_, "");
    lv_label_set_text(title_, title());
    lv_label_set_text(hint_, prompt());
  }

  void reveal(uint32_t now_ms) {
    phase_ = Phase::REVEAL;
    reveal_ms_ = now_ms;
    bloom_done_ = false;
    clover_.set_glow(0.0f);
    for (auto *w : wind_) lv_obj_set_style_text_opa(w, LV_OPA_TRANSP, 0);
    for (auto *c : clouds_) lv_obj_add_flag(c, LV_OBJ_FLAG_HIDDEN);

    int n = 0;
    const SpinResult *rs = results(n);
    int i = (n > 0) ? (int)(esp_random() % (uint32_t)n) : 0;
    cur_ = rs[i];

    lv_label_set_text(answer_, cur_.text);
    lv_label_set_text(sub_, cur_.sub ? cur_.sub : "");
    lv_obj_set_style_text_opa(answer_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_opa(sub_, LV_OPA_TRANSP, 0);
    lv_label_set_text(title_, "谕示");
    lv_label_set_text(hint_, vault_tag() ? "双击收藏  短按重来  长按返回"
                                         : "短按重来  长按返回");
    if (app::SystemHooks::instance().test_haptic)
      app::SystemHooks::instance().test_haptic();
  }

  void toggle_save() {
    const char *tag = vault_tag();
    if (!tag) return;
    saved_ = !saved_;
    lv_label_set_text(sub_, saved_ ? "已收藏" : (cur_.sub ? cur_.sub : ""));
    if (saved_)
      app::Vault::instance().add(cur_.text, tag, 0);
  }

  static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
  }

  // ---- 手感参数(度/秒) ----
  static constexpr int CLOVER_PX = 100;
  static constexpr float IMPULSE = 150.0f;      // 每格注入角速度 × strength
  static constexpr float VEL_MAX = 1800.0f;     // 角速度上限(~5 转/秒)
  static constexpr float FRICTION_TAU = 0.75f;  // 摩擦时间常数(越大转越久)
  static constexpr float SETTLE_VEL = 45.0f;    // 低于此转速则落定
  static constexpr float GLOW_FULL = 900.0f;    // 光环全亮对应转速
  static constexpr float BLOOM_PEAK = 1.18f;    // 绽放峰值缩放
  static constexpr int BLOOM_MS = 420;          // 绽放时长

  lv_obj_t *root_{nullptr};
  lv_obj_t *title_{nullptr};
  lv_obj_t *answer_{nullptr};
  lv_obj_t *sub_{nullptr};
  lv_obj_t *hint_{nullptr};
  lv_obj_t *mascot_{nullptr};
  lv_obj_t *wind_[3]{};
  lv_obj_t *clouds_[3]{};
  SpinClover clover_;

  Phase phase_{Phase::SPIN};
  float angle_{0.0f};
  float vel_{0.0f};
  bool has_spun_{false};
  bool saved_{false};
  bool bloom_done_{false};
  uint32_t last_ms_{0};
  uint32_t reveal_ms_{0};
  SpinResult cur_{"", nullptr};
};

}  // namespace ui
