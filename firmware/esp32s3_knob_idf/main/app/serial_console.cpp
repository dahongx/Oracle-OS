#include "app/serial_console.hpp"

#include <cstdio>
#include <fcntl.h>

#include <esp_timer.h>
#include <driver/usb_serial_jtag.h>
#include <driver/usb_serial_jtag_vfs.h>

using namespace std::chrono_literals;

namespace app {

static uint32_t now_ms() { return (uint32_t)(esp_timer_get_time() / 1000); }

SerialConsole::SerialConsole(const Config &config)
    : cfg_(config), logger_({.tag = "SerialConsole", .level = config.log_level}) {}

void SerialConsole::print_help() {
  logger_.info("---- 串口模拟命令 ----");
  logger_.info(" a/d=逆/顺转一格  A/D=快转5格  f/s=快/慢转速  0=停转  z=角度清零");
  logger_.info(" p=短按(确认/进入)  b=长按(返回)  Q=关机");
  logger_.info(" H=切真实霍尔  S=切串口模拟  ?=帮助");
}

void SerialConsole::sim_step_cw() {
  if (!cfg_.sim)
    return;
  uint32_t t = now_ms();
  cfg_.sim->set_rpm(0, t);
  cfg_.sim->rotate(+cfg_.detent_step_deg, t);
}

void SerialConsole::start() {
  task_ = espp::Task::make_unique({
      .callback = [this](std::mutex &m, std::condition_variable &cv) -> bool {
        return poll(m, cv);
      },
      .task_config = {.name = "Serial Cmd", .stack_size_bytes = 4096, .priority = 5},
  });
  task_->start();
}

bool SerialConsole::poll(std::mutex &m, std::condition_variable &cv) {
  static bool stdin_ready = false;
  if (!stdin_ready) {
    // 关键:本板控制台走原生 USB-Serial/JTAG。默认控制台【只装了输出】,没有接收驱动,
    // 所以 getchar() 读不到任何主机发来的字节——主机端(串口助手/PowerShell)因为数据
    // 没人消费,write 会一直阻塞,表现为"一点发送就卡死"。这就是发不出去的根因。
    // 装上 USB-Serial/JTAG 驱动并把它绑到 VFS,stdin 才能真正收到字节。
    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    usb_serial_jtag_driver_install(&cfg);
    usb_serial_jtag_vfs_use_driver(); // 让 stdin/stdout 走带缓冲的 USB-JTAG 驱动
    // stdin 设为非阻塞:无输入时 getchar() 立刻返回 EOF,不卡任务
    setvbuf(stdin, nullptr, _IONBF, 0);
    int fl = fcntl(fileno(stdin), F_GETFL, 0);
    fcntl(fileno(stdin), F_SETFL, fl | O_NONBLOCK);
    stdin_ready = true;
    logger_.info("USB-Serial/JTAG 接收驱动已安装,串口输入已就绪");
  }
  int c = getchar();
  if (c != EOF && c > 0)
    handle_char(c);

  std::unique_lock<std::mutex> lk(m);
  cv.wait_for(lk, 20ms);
  return false;
}

void SerialConsole::handle_char(int c) {
  auto *sim = cfg_.sim;
  if (!sim)
    return;
  uint32_t t = now_ms();
  logger_.info("[串口收到] '{}' (0x{:02X})", (c >= 32 && c < 127) ? (char)c : '.', c);
  const float step = cfg_.detent_step_deg;
  switch (c) {
  // 注:不再在 rotate 前 set_rpm(0),让 rotate() 能按连按间隔估算转速(速度反馈)。
  case 'a': sim->rotate(-step, t); logger_.info("[SIM] 逆时针一格 -> {:.0f}deg", sim->degrees()); break;
  case 'd': sim->rotate(+step, t); logger_.info("[SIM] 顺时针一格 -> {:.0f}deg", sim->degrees()); break;
  case 'A': sim->rotate(-5 * step, t); logger_.info("[SIM] 逆时针快转5格 -> {:.0f}deg", sim->degrees()); break;
  case 'D': sim->rotate(+5 * step, t); logger_.info("[SIM] 顺时针快转5格 -> {:.0f}deg", sim->degrees()); break;
  case 'f': sim->set_rpm(120.0f, t); logger_.info("[SIM] 快速旋转 120rpm"); break;
  case 's': sim->set_rpm(20.0f, t); logger_.info("[SIM] 慢速旋转 20rpm"); break;
  case '0': sim->set_rpm(0.0f, t); logger_.info("[SIM] 停转"); break;
  case 'z': sim->set_angle(0.0f); logger_.info("[SIM] 角度清零"); break;
  case 'H': if (cfg_.on_switch_to_hall) cfg_.on_switch_to_hall(); break;
  case 'S': if (cfg_.on_switch_to_sim) cfg_.on_switch_to_sim(); break;
  case 'p': if (cfg_.on_press) cfg_.on_press(); logger_.info("[SIM] PRESS"); break;
  case 'b': if (cfg_.on_back) cfg_.on_back(); logger_.info("[SIM] BACK"); break;
  case 'Q': logger_.warn("[SIM] 关机"); if (cfg_.on_shutdown) cfg_.on_shutdown(); break;
  case '?': print_help(); break;
  default: break;
  }
}

} // namespace app
