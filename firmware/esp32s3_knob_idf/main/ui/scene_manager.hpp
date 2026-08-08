#pragma once
// ============================================================================
// 场景管理器 (SceneManager)
// ----------------------------------------------------------------------------
// UI 的中枢:持有场景栈、把外部输入事件安全地送给栈顶场景、按场景返回的
// SceneId 做压栈/出栈切换。所有 LVGL 操作都在自己的 GUI 任务里做(线程安全),
// 外部(旋钮服务/串口/按键)只调 post_event() 投递事件,不直接碰 LVGL。
//
// 依赖方向:app 层把输入事件 post 进来;本类只依赖 Scene 接口 + LVGL,不依赖驱动。
// ============================================================================
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "display.hpp"
#include "logger.hpp"
#include "task.hpp"

#include "ui/scene.hpp"

namespace ui {

class SceneManager {
public:
  struct Config {
    SceneId initial{SceneId::HOME};
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::INFO};
  };

  explicit SceneManager(const Config &config);

  // 外部线程安全投递输入事件(旋钮服务/串口/按键调用)。
  // strength:旋转事件的强度(≈1 慢速一格,快转 >1);非旋转事件忽略。
  void post_event(Event ev, float strength = 1.0f);

  // 设置长按进度 [0,1](按键驱动调用)。>0 时显示边缘进度环,=0 时隐藏。
  // 线程安全:只存一个 atomic,真正的 LVGL 绘制在 GUI 任务里做。
  void set_hold_progress(float progress);

  // 更新全局状态栏(电池任务/输入源切换调用)。线程安全:存 atomic,GUI 任务里刷。
  void set_status(float battery_volts, bool using_hall);

private:
  // 工厂:按 id 创建场景实例(集中在 .cpp 里,避免头文件互相 include)
  std::unique_ptr<Scene> create_scene(SceneId id);

  void push_scene(SceneId id);
  void pop_scene();
  void rebuild_active_scene(); // 清空 root 并让栈顶场景重建 UI
  void handle_navigation(SceneId target);

  bool gui_tick(std::mutex &m, std::condition_variable &cv);
  void drain_events();

  espp::Logger logger_;
  std::recursive_mutex mutex_;

  lv_obj_t *root_{nullptr};        // 占满屏的容器,场景 UI 都建在它上面
  lv_obj_t *hold_arc_{nullptr};    // 长按进度环(覆盖在最上层,平时隐藏)
  std::vector<std::unique_ptr<Scene>> stack_;

  std::atomic<float> hold_progress_{0.0f}; // 待应用的长按进度
  float applied_hold_progress_{-1.0f};      // 已应用到 arc 的值(避免每帧重画)
  void apply_hold_progress();

  uint32_t scene_enter_ms_{0}; // 栈顶场景进入时刻(供 on_tick 计算 elapsed)
  void replace_with_home();    // 清空栈并只放 Home(Boot 结束用)

  lv_obj_t *status_label_{nullptr};       // 全局状态栏(右上角:电池 + 输入源)
  std::atomic<float> status_volts_{0.0f};
  std::atomic<bool> status_hall_{false};
  std::atomic<bool> status_dirty_{false};
  void apply_status();

  // 简单事件队列(容量小,GUI 任务每帧排空)
  struct QueuedEvent {
    Event ev;
    float strength;
  };
  static constexpr int QUEUE_CAP = 16;
  QueuedEvent queue_[QUEUE_CAP];
  int q_head_{0}, q_tail_{0};
  std::mutex q_mutex_;

  std::unique_ptr<espp::Task> task_;
};

} // namespace ui
