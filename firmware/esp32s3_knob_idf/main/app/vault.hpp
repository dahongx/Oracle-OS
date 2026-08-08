#pragma once
// ============================================================================
// 收藏库 (Vault) —— 保存用户"收藏/保留"的仪式结果
// ----------------------------------------------------------------------------
// 全局单例,各模式(QuickAsk/Triad/Daily...)在用户收藏时写入,History 场景读取浏览。
// 目前为内存版(掉电清空),满了从最旧的开始覆盖(环形)。后续可加 NVS 持久化。
// 线程安全:UI 任务和串口任务都可能访问,加锁保护。
// ============================================================================
#include <array>
#include <cstring>
#include <mutex>

namespace app {

struct VaultEntry {
  char text[32]; // 结果文字
  char from[16]; // 来源模式(QuickAsk/Triad/Daily)
  int symbol;    // 象征符号索引(ui::symbol),-1 表示无
};

class Vault {
public:
  static constexpr int CAP = 16;

  static Vault &instance() {
    static Vault v;
    return v;
  }

  void add(const char *text, const char *from, int symbol) {
    std::scoped_lock lk(mutex_);
    VaultEntry &e = ring_[tail_];
    std::strncpy(e.text, text ? text : "", sizeof(e.text) - 1);
    e.text[sizeof(e.text) - 1] = '\0';
    std::strncpy(e.from, from ? from : "", sizeof(e.from) - 1);
    e.from[sizeof(e.from) - 1] = '\0';
    e.symbol = symbol;
    tail_ = (tail_ + 1) % CAP;
    if (count_ < CAP)
      count_++;
  }

  int count() {
    std::scoped_lock lk(mutex_);
    return count_;
  }

  // 取第 i 条(0=最新)。成功返回 true 并填充 out。
  bool get(int i, VaultEntry &out) {
    std::scoped_lock lk(mutex_);
    if (i < 0 || i >= count_)
      return false;
    // 最新在 tail_-1,往前数
    int idx = (tail_ - 1 - i % CAP + CAP) % CAP;
    out = ring_[idx];
    return true;
  }

private:
  Vault() = default;
  std::mutex mutex_;
  std::array<VaultEntry, CAP> ring_{};
  int tail_{0};
  int count_{0};
};

} // namespace app
