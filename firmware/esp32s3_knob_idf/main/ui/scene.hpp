#pragma once
// ============================================================================
// UI 场景抽象 (Scene) + 输入事件模型
// ----------------------------------------------------------------------------
// 滚轮的物理动作被抽象成 4 种 UI 事件,场景只处理事件,不关心事件从哪来
// (真实滚轮 / 串口模拟 / 物理按键都产生同样的事件):
//   ROTATE_CW / ROTATE_CCW : 旋转一格(顺/逆) —— 选择、切换焦点
//   PRESS                  : 短按          —— 确认、进入
//   BACK                   : 长按          —— 返回、取消
//
// 每个界面 = 一个 Scene:自己建 LVGL 对象、响应事件、可请求切换到别的场景。
// SceneManager(见 scene_manager.hpp)持有场景栈并把事件分发给栈顶场景。
// ============================================================================
#include <memory>

#include "display.hpp"

namespace ui {

enum class Event {
  ROTATE_CW,   // 顺时针一格
  ROTATE_CCW,  // 逆时针一格
  PRESS,       // 短按:确认/进入
  BACK,        // 长按:返回/取消
  POWER,       // 长按10秒:进入关机确认
};

// 场景标识:场景通过返回它来请求导航。用枚举而非直接 new,避免场景间硬依赖。
enum class SceneId {
  NONE,        // 不切换(留在当前场景)
  BACK,        // 弹出当前场景,回到上一个
  REPLACE_HOME,// 用 Home 替换整个栈(Boot 结束用:不希望能返回到 Boot)
  BOOT,        // 开机场景
  HOME,        // 首页
  MODE_SELECT, // 模式选择
  QUICK_ASK,   // 即时一问(示例模式)
  PLAY,        // 逗猫时光
  ORBIT,       // 轨道捕捉
  TRIAD,       // 三符占卜
  MOOD,        // 情绪罗盘
  DAILY,       // 每日指引
  HISTORY,     // 收藏历史 / Vault
  SETTINGS,    // 设置
  HARDWARE_DIAG, // 硬件诊断
  POWER_CONFIRM, // 关机确认
};

class SceneManager; // 前向声明

// 场景基类。子类在 on_enter 里建 UI,在 on_event 里响应输入并返回要跳转的目标。
class Scene {
public:
  virtual ~Scene() = default;

  // 进入场景:在给定的父对象(一个占满屏的容器)上建立本场景的 LVGL 界面。
  virtual void on_enter(lv_obj_t *root) = 0;

  // 处理一个输入事件,返回导航意图:
  //   NONE      = 留在本场景
  //   BACK      = 返回上一场景
  //   其它 Id   = 进入该场景
  // 大多数场景只需重写这个版本(不关心旋转强度)。
  virtual SceneId on_event(Event ev) { return SceneId::NONE; }

  // 带旋转强度的版本:strength≈1 为慢速一格,快转时 >1(速度越快越大)。
  // 关心速度反馈的场景(如 QuickAsk 能量积累)重写这个;默认转发到上面的无强度版本。
  // 通过虚函数默认转发,已有场景无需改动。
  virtual SceneId on_event(Event ev, float strength) {
    (void)strength;
    return on_event(ev);
  }

  // 每帧回调(GUI 任务里调),now_ms=进入本场景以来的毫秒数。默认不做事、不切换。
  // 用于自驱动场景(如 Boot 停留后自动进 Home)和动效。返回导航意图,语义同 on_event。
  virtual SceneId on_tick(uint32_t /*elapsed_ms*/) { return SceneId::NONE; }

  // 场景名(日志/调试)
  virtual const char *name() const = 0;
};

} // namespace ui
