#include "ui/scene_manager.hpp"

#include <esp_timer.h>

#include "ui/scenes/boot_scene.hpp"
#include "ui/scenes/home_scene.hpp"
#include "ui/scenes/mode_select_scene.hpp"
#include "ui/scenes/quick_ask_scene.hpp"
#include "ui/scenes/play_scene.hpp"
#include "ui/scenes/orbit_scene.hpp"
#include "ui/scenes/triad_scene.hpp"
#include "ui/scenes/mood_scene.hpp"
#include "ui/scenes/daily_scene.hpp"
#include "ui/scenes/history_scene.hpp"
#include "ui/scenes/settings_scene.hpp"
#include "ui/scenes/hardware_diag_scene.hpp"
#include "ui/scenes/power_confirm_scene.hpp"

static uint32_t sm_now_ms() { return (uint32_t)(esp_timer_get_time() / 1000); }

using namespace std::chrono_literals;

namespace ui {

SceneManager::SceneManager(const Config &config)
    : logger_({.tag = "SceneManager", .level = config.log_level}) {
  // 打印实际 LVGL 分辨率(旋转后),便于校准各场景布局坐标。
  {
    auto d = lv_display_get_default();
    logger_.info("==== 屏幕分辨率(旋转后): {} x {} ====",
                 (int)lv_display_get_horizontal_resolution(d),
                 (int)lv_display_get_vertical_resolution(d));
  }
  // 建一个占满屏幕的根容器,所有场景 UI 都建在它的子对象上;
  // 切场景时清空它再让新场景重建。
  root_ = lv_obj_create(lv_screen_active());
  lv_obj_set_size(root_, LV_PCT(100), LV_PCT(100));
  lv_obj_center(root_);
  lv_obj_clear_flag(root_, LV_OBJ_FLAG_SCROLLABLE);

  theme::apply_bg(lv_screen_active());
  theme::apply_bg(root_);

  // 产品版不显示覆盖全屏的长按进度圈。
  hold_arc_ = nullptr;

  // 全局状态栏:右上角小字(电池 + 输入源标记),各场景之上常驻。
  status_label_ = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_font(status_label_, theme::font_normal(), 0);
  lv_obj_set_style_text_color(status_label_, theme::text_dim(), 0);
  lv_label_set_text(status_label_, "");
  lv_obj_align(status_label_, LV_ALIGN_TOP_RIGHT, -6, 4);

  push_scene(config.initial);

  // GUI 任务:周期排空事件队列 + 跑 LVGL。所有 LVGL 操作都在这个任务里。
  task_ = espp::Task::make_unique({
      .callback = [this](std::mutex &m, std::condition_variable &cv) -> bool {
        return gui_tick(m, cv);
      },
      .task_config = {.name = "UI Task", .stack_size_bytes = 8 * 1024, .priority = 5},
  });
  task_->start();
}

std::unique_ptr<Scene> SceneManager::create_scene(SceneId id) {
  switch (id) {
  case SceneId::BOOT:
    return std::make_unique<BootScene>();
  case SceneId::HOME:
    return std::make_unique<HomeScene>();
  case SceneId::MODE_SELECT:
    return std::make_unique<ModeSelectScene>();
  case SceneId::QUICK_ASK:
    return std::make_unique<QuickAskScene>();
  case SceneId::PLAY:
    return std::make_unique<PlayScene>();
  case SceneId::ORBIT:
    return std::make_unique<OrbitScene>();
  case SceneId::TRIAD:
    return std::make_unique<TriadScene>();
  case SceneId::MOOD:
    return std::make_unique<MoodScene>();
  case SceneId::DAILY:
    return std::make_unique<DailyScene>();
  case SceneId::HISTORY:
    return std::make_unique<HistoryScene>();
  case SceneId::SETTINGS:
    return std::make_unique<SettingsScene>();
  case SceneId::HARDWARE_DIAG:
    return std::make_unique<HardwareDiagScene>();
  case SceneId::POWER_CONFIRM:
    return std::make_unique<PowerConfirmScene>();
  default:
    return nullptr;
  }
}

void SceneManager::push_scene(SceneId id) {
  auto scene = create_scene(id);
  if (!scene) {
    logger_.warn("场景 {} 未实现,忽略", (int)id);
    return;
  }
  logger_.info("push scene: {}", scene->name());
  stack_.push_back(std::move(scene));
  rebuild_active_scene();
}

void SceneManager::pop_scene() {
  if (stack_.size() <= 1) {
    logger_.info("已在根场景,不再返回");
    return;
  }
  logger_.info("pop scene: {}", stack_.back()->name());
  stack_.pop_back();
  rebuild_active_scene();
}

void SceneManager::rebuild_active_scene() {
  std::scoped_lock<std::recursive_mutex> lk(mutex_);
  lv_obj_clean(root_); // 清掉旧场景的所有子对象
  scene_enter_ms_ = sm_now_ms();
  if (!stack_.empty()) {
    stack_.back()->on_enter(root_);
    logger_.info("==== 当前场景: {} (栈深 {}) ====", stack_.back()->name(), (int)stack_.size());
  }
}

void SceneManager::replace_with_home() {
  std::scoped_lock<std::recursive_mutex> lk(mutex_);
  stack_.clear();
  auto home = create_scene(SceneId::HOME);
  if (home) {
    logger_.info("replace stack -> Home");
    stack_.push_back(std::move(home));
    rebuild_active_scene();
  }
}

void SceneManager::handle_navigation(SceneId target) {
  switch (target) {
  case SceneId::NONE:
    break; // 留在当前场景
  case SceneId::BACK:
    pop_scene();
    break;
  case SceneId::REPLACE_HOME:
    replace_with_home();
    break;
  default:
    push_scene(target);
    break;
  }
}

void SceneManager::set_hold_progress(float progress) {
  hold_progress_.store(progress);
}

void SceneManager::apply_hold_progress() {
  float p = hold_progress_.load();
  if (p == applied_hold_progress_)
    return; // 没变化,不重画
  applied_hold_progress_ = p;
  if (!hold_arc_)
    return;
  if (p <= 0.001f) {
    lv_obj_add_flag(hold_arc_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(hold_arc_, LV_OBJ_FLAG_HIDDEN);
    lv_arc_set_value(hold_arc_, (int32_t)(p * 360.0f));
  }
}

void SceneManager::set_status(float battery_volts, bool using_hall) {
  status_volts_.store(battery_volts);
  status_hall_.store(using_hall);
  status_dirty_.store(true);
}

void SceneManager::apply_status() {
  if (!status_dirty_.exchange(false) || !status_label_)
    return;
  float v = status_volts_.load();
  bool hall = status_hall_.load();
  (void)hall;
  char buf[12];
  if (v < 2.5f) {
    lv_label_set_text(status_label_, "");
    return;
  }
  int percent = (int)((v - 3.3f) * 100.0f / 0.9f);
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  snprintf(buf, sizeof(buf), "%d%%", percent);
  lv_label_set_text(status_label_, buf);
}

void SceneManager::post_event(Event ev, float strength) {
  std::scoped_lock<std::mutex> lk(q_mutex_);
  int next = (q_tail_ + 1) % QUEUE_CAP;
  if (next == q_head_) {
    logger_.warn("事件队列满,丢弃事件");
    return;
  }
  queue_[q_tail_] = {ev, strength};
  q_tail_ = next;
}

void SceneManager::drain_events() {
  for (;;) {
    QueuedEvent qe;
    {
      std::scoped_lock<std::mutex> lk(q_mutex_);
      if (q_head_ == q_tail_)
        return; // 队列空
      qe = queue_[q_head_];
      q_head_ = (q_head_ + 1) % QUEUE_CAP;
    }
    if (!stack_.empty()) {
      SceneId target = stack_.back()->on_event(qe.ev, qe.strength);
      handle_navigation(target);
    }
  }
}

bool SceneManager::gui_tick(std::mutex &m, std::condition_variable &cv) {
  {
    std::scoped_lock<std::recursive_mutex> lk(mutex_);
    drain_events();
    apply_hold_progress();
    apply_status();
    // 驱动栈顶场景的 on_tick(自驱动切换 / 动效)
    if (!stack_.empty()) {
      SceneId target = stack_.back()->on_tick(sm_now_ms() - scene_enter_ms_);
      if (target != SceneId::NONE)
        handle_navigation(target);
    }
    lv_task_handler();
  }
  std::unique_lock<std::mutex> lk(m);
  cv.wait_for(lk, 16ms);
  return false;
}

} // namespace ui
