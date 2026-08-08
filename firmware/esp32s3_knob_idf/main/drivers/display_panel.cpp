#include "drivers/display_panel.hpp"

namespace drivers {

static constexpr int FLUSH_BIT = (1 << (int)espp::display_drivers::Flags::FLUSH_BIT);
static constexpr int DC_LEVEL_BIT = (1 << (int)espp::display_drivers::Flags::DC_LEVEL_BIT);

// 刷屏完成回调(SPI 事务后置回调里调,通知 LVGL 一帧刷完)。必须放 IRAM。
static void IRAM_ATTR lcd_flush_ready(uint32_t) {
  auto disp = lv_display_get_default();
  if (disp)
    lv_display_flush_ready(disp);
}

// driver_ 需要在 flush lambda 里被捕获;放静态指针供 lambda 使用(单实例设备)。
static std::shared_ptr<DisplayPanel::DisplayDriver> s_driver;

DisplayPanel::DisplayPanel(const Config &config)
    : logger_({.tag = "Display", .level = config.log_level}) {
  gpio_config_t backlight_config{
      .pin_bit_mask = 1ULL << config.blk,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&backlight_config));
  ESP_ERROR_CHECK(gpio_set_level(config.blk, 1));
  logger_.info("LCD diagnostic: backlight GPIO{} forced HIGH", (int)config.blk);

  static constexpr auto spi_host = SPI2_HOST;
  spi_ = std::make_unique<espp::Spi>(espp::Spi::Config{
      .host = spi_host,
      .sclk_io_num = config.sclk,
      .mosi_io_num = config.mosi,
      .miso_io_num = GPIO_NUM_NC, // 屏幕没接 MISO
      .max_transfer_sz = static_cast<int>(config.width * 50 * sizeof(lv_color16_t)),
  });
  if (!spi_->initialized()) {
    logger_.error("LCD SPI 总线初始化失败");
    return;
  }

  panel_io_ = std::make_unique<espp::SpiPanelIo>(espp::SpiPanelIo::Config{
      .spi = spi_.get(),
      .device_config =
          {
              .mode = 3, // 这块屏需要 Mode 3 (CPOL=1, CPHA=1)
              .clock_speed_hz = 10 * 1000 * 1000, // 降低到10MHz测试时序问题
              .input_delay_ns = 0,
              .cs_io_num = config.cs,
              .queue_size = 6,
          },
      .data_command_io = config.dc,
      .data_command_bit_mask = DC_LEVEL_BIT,
      .post_transaction_callback_bit_mask = FLUSH_BIT,
      .post_transaction_callback = lcd_flush_ready,
  });
  if (!panel_io_->initialized()) {
    logger_.error("LCD SPI panel IO 初始化失败");
    return;
  }

  driver_ = std::make_shared<DisplayDriver>(espp::display_drivers::Config{
      .panel_io = panel_io_.get(),
      .write_command = nullptr,
      .read_command = nullptr,
      .lcd_send_lines = nullptr,
      .reset_pin = config.rst,
      .data_command_pin = config.dc,
      .reset_value = false,
      .invert_colors = false,
      .offset_x = 0,
      .offset_y = 0,
      .mirror_x = false,
      .mirror_y = false,
  });
  logger_.info("开始初始化ST7789驱动...");
  driver_->initialize();
  logger_.info("ST7789驱动初始化完成");
  s_driver = driver_;

  logger_.info("开始创建LVGL Display对象...");
  display_ = std::make_shared<espp::Display<lv_color16_t>>(
      espp::Display<lv_color16_t>::LvglConfig{
          .width = config.width,
          .height = config.height,
          .flush_callback =
              [](lv_display_t *disp, const lv_area_t *area, uint8_t *color_map) {
                s_driver->flush(disp, area, color_map);
              },
          .rotation_callback =
              [](const espp::DisplayRotation &rotation) { s_driver->set_rotation(rotation); },
          // 不设置rotation,使用硬件默认方向
      },
      espp::Display<lv_color16_t>::LcdConfig{.backlight_pin = GPIO_NUM_NC,
                                             .backlight_on_value = true},
      espp::Display<lv_color16_t>::DynamicMemoryConfig{
          .pixel_buffer_size = config.width * 50, .double_buffered = true});
  logger_.info("LVGL Display对象创建完成");

  logger_.info("LCD diagnostic: PWM backlight disabled");

  logger_.info("LCD 显示初始化完成 ({}x{}, ST7789V2)", config.width, config.height);
}

} // namespace drivers
