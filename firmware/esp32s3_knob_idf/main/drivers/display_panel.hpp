#pragma once
// ============================================================================
// 显示屏驱动 (Driver Layer - Display Panel)
// ----------------------------------------------------------------------------
// 封装 SPI 总线 + ST7789V2 面板 + LVGL 显示对象。对 UI 层只暴露一个初始化好的
// espp::Display<lv_color16_t>(LVGL 直接在上面建界面),SPI 时序/刷屏回调全封在内部。
//
// 用的是厂家验证过的 ST7789V2 初始化序列(st7789_vendor.hpp),不是 espp 自带的
// st7789.hpp —— 后者混入了 ILI9341 命令会导致这块屏花屏/黑屏。
// ============================================================================
#include <memory>

#include "display.hpp"
#include "logger.hpp"
#include "spi.hpp"

#include "drivers/st7789_vendor.hpp"

namespace drivers {

class DisplayPanel {
public:
  using DisplayDriver = espp::St7789Vendor;

  struct Config {
    gpio_num_t mosi, sclk, cs, dc, rst, blk;
    size_t width{240};
    size_t height{240};
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::WARN};
  };

  explicit DisplayPanel(const Config &config);

  bool ok() const { return display_ != nullptr; }
  std::shared_ptr<espp::Display<lv_color16_t>> lvgl_display() const { return display_; }

  // 设置背光亮度 [0,1]
  void set_brightness(float b) {
    if (display_)
      display_->set_brightness(b);
  }

private:
  espp::Logger logger_;
  std::unique_ptr<espp::Spi> spi_;
  std::unique_ptr<espp::SpiPanelIo> panel_io_;
  std::shared_ptr<DisplayDriver> driver_;
  std::shared_ptr<espp::Display<lv_color16_t>> display_;
};

} // namespace drivers
