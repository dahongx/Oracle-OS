#pragma once
// ============================================================================
// 旋钮输入驱动接口 (Driver Layer - Input Source)
// ----------------------------------------------------------------------------
// 霍尔输入的物理含义 = 磁转子的【角度 / 转速 / 方向】。应用层只依赖 IInputSource
// 这个统一接口,不关心数据是来自真实磁编码器还是串口模拟:
//   - As5600Encoder : 真实 AS5600 磁编码器(见 as5600_encoder.hpp)
//   - SerialSource   : 串口命令驱动的模拟源(霍尔磁铁没调好前用)
// 磁铁调好后,Board 换成 As5600Encoder 即可,应用/UI 层零改动。
// ============================================================================
#include <atomic>
#include <cmath>
#include <cstdint>

namespace drivers {

// 统一旋钮输入接口:对应用层来说,旋钮就是"角度 + 转速"两个只读量。
class IInputSource {
public:
  virtual ~IInputSource() = default;
  virtual float degrees() = 0;      // 累计角度(deg,可为负、可超过 360)
  virtual float mech_degrees() = 0; // 机械角 0~360
  virtual float rpm() = 0;          // 转速(rpm;正=顺时针,负=逆时针)
  virtual const char *name() = 0;   // 数据源名字(用于日志/UI)
};

// ---- 串口模拟输入源 ----
// 角度由串口命令累加/设定;转速由命令设定,并在空闲一段时间后自动衰减到 0。
// 线程安全(全用 atomic),可被串口任务和旋钮任务并发访问。
class SerialSource : public IInputSource {
public:
  float degrees() override { return accum_deg_.load(); }
  float mech_degrees() override {
    float m = std::fmod(accum_deg_.load(), 360.0f);
    if (m < 0)
      m += 360.0f;
    return m;
  }
  float rpm() override { return rpm_.load(); }
  const char *name() override { return "SERIAL-SIM(串口模拟)"; }

  // 相对旋转 delta 度(可正可负)。根据与上一次旋转的时间间隔估算瞬时转速:
  // 连按越快 -> 间隔越小 -> 估出的 rpm 越大,从而驱动"速度越快能量越强"。
  void rotate(float delta_deg, uint32_t now_ms) {
    accum_deg_.store(accum_deg_.load() + delta_deg);
    uint32_t prev = last_move_ms_.load();
    last_move_ms_.store(now_ms);
    uint32_t dt = (prev != 0 && now_ms > prev) ? (now_ms - prev) : 0;
    if (dt > 0 && dt < 600) {
      // rpm = (delta_deg/360 圈) / (dt 分钟)
      float rpm = (delta_deg / 360.0f) / (dt / 60000.0f);
      rpm_.store(rpm);
    } else {
      // 间隔过大(慢速/首次)给个方向一致的低速默认值
      rpm_.store(delta_deg >= 0 ? 12.0f : -12.0f);
    }
  }
  void set_angle(float deg) { accum_deg_.store(deg); }
  void set_rpm(float v, uint32_t now_ms) {
    rpm_.store(v);
    last_move_ms_.store(now_ms);
  }

  // 周期调用:空闲超过 idle_ms 就把转速衰减到 0(模拟"停转")。
  void tick(uint32_t now_ms, uint32_t idle_ms = 800) {
    if (rpm_.load() != 0.0f && (now_ms - last_move_ms_.load()) >= idle_ms)
      rpm_.store(0.0f);
  }

private:
  std::atomic<float> accum_deg_{0.0f};
  std::atomic<float> rpm_{0.0f};
  std::atomic<uint32_t> last_move_ms_{0};
};

} // namespace drivers
