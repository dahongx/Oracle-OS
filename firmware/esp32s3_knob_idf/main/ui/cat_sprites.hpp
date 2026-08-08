#pragma once

#include <cstdint>

#include "display.hpp"

extern "C" {
extern const uint8_t cat_sit_map[];
extern const uint8_t cat_blink_map[];
extern const uint8_t cat_closed_map[];
extern const uint8_t cat_tail_map[];
extern const uint8_t cat_blow_map[];
extern const uint8_t cat_play_map[];
extern const uint8_t cat_pounce_map[];
extern const uint8_t cat_catch_map[];
extern const uint8_t cat_breathe_map[];
extern const uint8_t cat_swipe_map[];
extern const uint8_t cat_relax_map[];
extern const uint8_t cat_clover_map[];
extern const uint8_t cat_menu_peek_map[];
extern const uint8_t cat_bare_tail_map[];
extern const uint8_t cat_bare_tail_left_map[];
}

namespace ui::cat_sprite {

enum class Pose {
  SIT,
  BLINK,
  CLOSED,
  TAIL,
  BLOW,
  PLAY,
  POUNCE,
  CATCH,
  BREATHE,
  SWIPE,
  RELAX,
  CLOVER,
  MENU_PEEK,
  BARE_TAIL,
  BARE_TAIL_LEFT,
};
inline constexpr int SIZE = 96;

struct Instance {
  lv_image_dsc_t dsc{};
  lv_obj_t *image{nullptr};
  int display_px{SIZE};
};

inline const uint8_t *data_for(Pose pose) {
  const uint8_t *data = cat_sit_map;
  switch (pose) {
  case Pose::SIT: data = cat_sit_map; break;
  case Pose::BLINK: data = cat_blink_map; break;
  case Pose::CLOSED: data = cat_closed_map; break;
  case Pose::TAIL: data = cat_tail_map; break;
  case Pose::BLOW: data = cat_blow_map; break;
  case Pose::PLAY: data = cat_play_map; break;
  case Pose::POUNCE: data = cat_pounce_map; break;
  case Pose::CATCH: data = cat_catch_map; break;
  case Pose::BREATHE: data = cat_breathe_map; break;
  case Pose::SWIPE: data = cat_swipe_map; break;
  case Pose::RELAX: data = cat_relax_map; break;
  case Pose::CLOVER: data = cat_clover_map; break;
  case Pose::MENU_PEEK: data = cat_menu_peek_map; break;
  case Pose::BARE_TAIL: data = cat_bare_tail_map; break;
  case Pose::BARE_TAIL_LEFT: data = cat_bare_tail_left_map; break;
  }
  return data;
}

inline lv_obj_t *draw(lv_obj_t *parent, Pose pose, int display_px = SIZE) {
  const uint8_t *data = data_for(pose);

  auto *instance = new Instance{};
  instance->display_px = display_px;
  instance->dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
  instance->dsc.header.cf = LV_COLOR_FORMAT_ARGB8888;
  instance->dsc.header.w = SIZE;
  instance->dsc.header.h = SIZE;
  instance->dsc.header.stride = SIZE * 4;
  instance->dsc.data_size = SIZE * SIZE * 4;
  instance->dsc.data = data;

  // Return a transparent container whose actual layout size equals the
  // displayed sprite. LVGL image transforms do not change layout bounds, so
  // aligning the raw image directly can place scaled pixels outside the screen.
  lv_obj_t *container = lv_obj_create(parent);
  lv_obj_set_size(container, display_px, display_px);
  lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(container, 0, 0);
  lv_obj_set_style_pad_all(container, 0, 0);
  lv_obj_set_style_radius(container, 0, 0);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(container, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_set_user_data(container, instance);

  lv_obj_t *img = lv_image_create(container);
  instance->image = img;
  lv_image_set_src(img, &instance->dsc);
  lv_image_set_antialias(img, true);
  lv_image_set_scale(img, (uint32_t)(256 * display_px / SIZE));
  lv_obj_center(img);
  lv_obj_add_event_cb(
      container,
      [](lv_event_t *e) {
        delete static_cast<Instance *>(lv_event_get_user_data(e));
      },
      LV_EVENT_DELETE, instance);
  return container;
}

inline Instance *instance_for(lv_obj_t *sprite) {
  return sprite ? static_cast<Instance *>(lv_obj_get_user_data(sprite)) : nullptr;
}

inline void set_pose(lv_obj_t *sprite, Pose pose) {
  auto *instance = instance_for(sprite);
  if (!instance || !instance->image) return;
  instance->dsc.data = data_for(pose);
  lv_image_set_src(instance->image, &instance->dsc);
  lv_obj_invalidate(instance->image);
}

inline void set_pivot(lv_obj_t *sprite, int source_x, int source_y) {
  auto *instance = instance_for(sprite);
  if (!instance || !instance->image) return;

  // Scaling is also performed around the image pivot in LVGL. Moving the
  // pivot away from the centre without compensating the image position pushes
  // part of the scaled sprite outside its container (the top of the Home
  // clover was clipped this way). Preserve the zero-rotation visual position.
  const int scale_delta = instance->display_px - SIZE;
  auto rounded_div = [](int value) {
    return value >= 0 ? (value + SIZE / 2) / SIZE
                      : (value - SIZE / 2) / SIZE;
  };
  const int offset_x =
      rounded_div(scale_delta * (source_x - SIZE / 2));
  const int offset_y =
      rounded_div(scale_delta * (source_y - SIZE / 2));
  lv_obj_align(instance->image, LV_ALIGN_CENTER, offset_x, offset_y);
  lv_image_set_pivot(instance->image, source_x, source_y);
}

inline void set_rotation(lv_obj_t *sprite, float degrees) {
  auto *instance = instance_for(sprite);
  if (!instance || !instance->image) return;
  lv_image_set_rotation(instance->image, static_cast<int32_t>(degrees * 10.0f));
}

} // namespace ui::cat_sprite
