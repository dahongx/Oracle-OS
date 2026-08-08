#pragma once

#include <cstdint>

#include "ui/spin_clover.hpp"

namespace ui {

// Every ritual uses the same visual clover: it leaves the cat's tail, becomes
// the interactive object, then returns before the page closes.
class RitualClover {
public:
  void build(lv_obj_t *parent, int display_px, int from_x, int from_y,
             int target_x, int target_y) {
    from_x_ = from_x;
    from_y_ = from_y;
    target_x_ = target_x;
    target_y_ = target_y;
    clover_.build(parent, display_px);
    lv_obj_align(clover_.root(), LV_ALIGN_CENTER, from_x_, from_y_);
    lv_obj_set_style_opa(clover_.root(), LV_OPA_TRANSP, 0);
    clover_.set_scale(0.28f);
  }

  void tick(uint32_t elapsed_ms) {
    last_ms_ = elapsed_ms;
    if (phase_ == Phase::ENTERING) {
      float t = clamp01(static_cast<float>(elapsed_ms) / ENTER_MS);
      float e = ease_out(t);
      move_between(from_x_, from_y_, target_x_, target_y_, e);
      clover_.set_scale(0.28f + 0.72f * e);
      lv_obj_set_style_opa(clover_.root(), static_cast<lv_opa_t>(255.0f * e), 0);
      if (t >= 1.0f) phase_ = Phase::ACTIVE;
    } else if (phase_ == Phase::EXITING) {
      float t = clamp01(static_cast<float>(elapsed_ms - exit_start_ms_) / EXIT_MS);
      float e = ease_in(t);
      move_between(target_x_, target_y_, from_x_, from_y_, e);
      clover_.set_scale(1.0f - 0.72f * e);
      lv_obj_set_style_opa(clover_.root(),
                           static_cast<lv_opa_t>(255.0f * (1.0f - 0.25f * e)), 0);
      if (t >= 1.0f) phase_ = Phase::DONE;
    }
  }

  void request_exit() {
    if (phase_ == Phase::DONE || phase_ == Phase::EXITING) return;
    phase_ = Phase::EXITING;
    exit_start_ms_ = last_ms_;
  }

  bool active() const { return phase_ == Phase::ACTIVE; }
  bool exiting() const { return phase_ == Phase::EXITING; }
  bool exit_done() const { return phase_ == Phase::DONE; }

  void set_target(int x, int y) {
    target_x_ = x;
    target_y_ = y;
    if (phase_ == Phase::ACTIVE)
      lv_obj_align(clover_.root(), LV_ALIGN_CENTER, target_x_, target_y_);
  }

  SpinClover &widget() { return clover_; }

private:
  enum class Phase { ENTERING, ACTIVE, EXITING, DONE };
  static constexpr float ENTER_MS = 620.0f;
  static constexpr float EXIT_MS = 460.0f;

  static float clamp01(float t) {
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t;
  }
  static float ease_out(float t) {
    float u = 1.0f - t;
    return 1.0f - u * u * u;
  }
  static float ease_in(float t) { return t * t * t; }

  void move_between(int ax, int ay, int bx, int by, float t) {
    // A small upward arc keeps the motion organic without moving the cat.
    float arc = -22.0f * 4.0f * t * (1.0f - t);
    int x = static_cast<int>(ax + (bx - ax) * t);
    int y = static_cast<int>(ay + (by - ay) * t + arc);
    lv_obj_align(clover_.root(), LV_ALIGN_CENTER, x, y);
  }

  SpinClover clover_;
  Phase phase_{Phase::ENTERING};
  int from_x_{-70};
  int from_y_{55};
  int target_x_{35};
  int target_y_{-35};
  uint32_t last_ms_{0};
  uint32_t exit_start_ms_{0};
};

} // namespace ui
