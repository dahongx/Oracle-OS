#pragma once

#include "display_drivers.hpp"

// espp 库自带的 espp::St7789 驱动,它的 initialize() 里混入了一些 ILI9341 专属的寄存器命令
// (0xCF/0xED/0xCB/0xF7/0xEA 等 Power Control A/B、Driver Timing Control 命令,这些是 ILI9341
// 才有的私有寄存器,在真正的 ST7789 上会写到完全不同、大多是厂商保留的内部寄存器),
// 会导致电源/伽马配置错误,表现为背光正常但屏幕花屏或黑屏。
//
// 这里改用厂家(鸿讯电子/BOE)针对这块 2.8寸 CL28BC251-40A (ST7789V2) 屏幕
// 提供的、已经验证过的官方初始化序列(见 doc/屏幕/_extracted/.../IC&Code/BOE2.8 IPS_7789V2_Code.txt),
// 其余逻辑(写窗口、清屏、旋转等)复用 espp::display_drivers::MipiDbiDisplayDriver 基类不变。
namespace espp {

class St7789Vendor : public display_drivers::MipiDbiDisplayDriver {
public:
  enum class Command : uint8_t {
    swreset = 0x01,
    slpout = 0x11,
    teon = 0x35,
    madctl = 0x36,
    colmod = 0x3a,
    porctrl = 0xb2, // Porch control
    gctrl = 0xb7,   // Gate control
    vcoms = 0xbb,
    lcmctrl = 0xc0,
    vdvvrhen = 0xc2,
    vrhs = 0xc3,
    vdvset = 0xc4,
    frctr2 = 0xc6,
    pwctrl1 = 0xd0,
    pwctrl2 = 0xd6,
    pvgamctrl = 0xe0, // 正极伽马
    nvgamctrl = 0xe1, // 负极伽马
    invoff = 0x20,
    invon = 0x21,
    dispon = 0x29,
    caset = 0x2a,
    raset = 0x2b,
    ramwr = 0x2c,
  };

  explicit St7789Vendor(const display_drivers::Config &config)
      : MipiDbiDisplayDriver(config,
                             {.column_address_command = static_cast<uint8_t>(Command::caset),
                              .row_address_command = static_cast<uint8_t>(Command::raset),
                              .memory_write_command = static_cast<uint8_t>(Command::ramwr)}) {}

  bool initialize() override {
    display_drivers::init_pins(config_.reset_pin, config_.data_command_pin, config_.reset_value);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(120ms);

    write_command(static_cast<uint8_t>(Command::slpout), {}, 0);
    std::this_thread::sleep_for(120ms);

    auto madctl_data = std::array<uint8_t, 1>{make_madctl(DisplayRotation::LANDSCAPE)};
    write_command(static_cast<uint8_t>(Command::madctl), madctl_data, 0);

    write_command(static_cast<uint8_t>(Command::colmod), std::array<uint8_t, 1>{0x55}, 0);

    // Porch control
    write_command(static_cast<uint8_t>(Command::porctrl),
                  std::array<uint8_t, 5>{0x0C, 0x0C, 0x00, 0x33, 0x33}, 0);

    write_command(static_cast<uint8_t>(Command::gctrl), std::array<uint8_t, 1>{0x46}, 0);

    write_command(static_cast<uint8_t>(Command::vcoms), std::array<uint8_t, 1>{0x1B}, 0);
    write_command(static_cast<uint8_t>(Command::lcmctrl), std::array<uint8_t, 1>{0x2C}, 0);
    write_command(static_cast<uint8_t>(Command::vdvvrhen), std::array<uint8_t, 1>{0x01}, 0);
    write_command(static_cast<uint8_t>(Command::vrhs), std::array<uint8_t, 1>{0x0F}, 0);
    write_command(static_cast<uint8_t>(Command::vdvset), std::array<uint8_t, 1>{0x20}, 0);
    write_command(static_cast<uint8_t>(Command::frctr2), std::array<uint8_t, 1>{0x0F}, 0);
    write_command(static_cast<uint8_t>(Command::pwctrl1), std::array<uint8_t, 2>{0xA4, 0xA1}, 0);
    write_command(static_cast<uint8_t>(Command::pwctrl2), std::array<uint8_t, 1>{0xA1}, 0);

    // 正极伽马
    write_command(static_cast<uint8_t>(Command::pvgamctrl),
                  std::array<uint8_t, 14>{0xF0, 0x00, 0x06, 0x04, 0x05, 0x05, 0x31, 0x44, 0x48,
                                          0x36, 0x12, 0x12, 0x2B, 0x34},
                  0);
    // 负极伽马
    write_command(static_cast<uint8_t>(Command::nvgamctrl),
                  std::array<uint8_t, 14>{0xF0, 0x0B, 0x0F, 0x0F, 0x0D, 0x26, 0x31, 0x43, 0x47,
                                          0x38, 0x14, 0x14, 0x2C, 0x32},
                  0);

    // 2.8" BOE/ST7789V2 面板必须开启反色，关闭时 RGB 与黑白都会呈互补色。
    write_command(static_cast<uint8_t>(Command::invon), {}, 0);

    write_command(static_cast<uint8_t>(Command::dispon), {}, 0);
    std::this_thread::sleep_for(50ms);

    return true;
  }

  void set_rotation(const DisplayRotation &rotation) override {
    Controller::set_rotation(rotation);
    auto data = std::array<uint8_t, 1>{make_madctl(rotation)};
    std::scoped_lock lock(io_mutex_);
    write_command(static_cast<uint8_t>(Command::madctl), data, 0);
  }

private:
  uint8_t make_madctl(DisplayRotation rotation) const {
    // MADCTL 位定义(ST7789 标准): MY(0x80) MX(0x40) MV(0x20) ML(0x10) RGB/BGR(0x08) MH(0x04)
    static constexpr uint8_t MADCTL_BGR_BIT = 0x08;
    static constexpr uint8_t MADCTL_MX_BIT = 0x40;
    static constexpr uint8_t MADCTL_MY_BIT = 0x80;
    static constexpr uint8_t MADCTL_MV_BIT = 0x20;
    auto value = display_drivers::make_madctl_base(config_, MADCTL_BGR_BIT, MADCTL_MX_BIT,
                                                   MADCTL_MY_BIT, MADCTL_MV_BIT);
    return display_drivers::apply_standard_rotation(value, config_, rotation, MADCTL_MX_BIT,
                                                    MADCTL_MY_BIT, MADCTL_MV_BIT);
  }
};

} // namespace espp
