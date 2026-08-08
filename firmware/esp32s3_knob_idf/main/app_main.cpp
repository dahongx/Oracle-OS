// Oracle OS - 主程序入口
// 使用完整的BSP + Scene架构
#include <cstdlib>
#include <ctime>

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "app/knob_service.hpp"
#include "app/system_hooks.hpp"
#include "bsp/board.hpp"
#include "bsp/pins.hpp"
#include "drivers/display_panel.hpp"
#include "drivers/power.hpp"
#include "ui/scene_manager.hpp"

static const char *TAG = "Main";

extern "C" void app_main() {
  // 软件关机时会锁存背光低电平；按键唤醒并软重启后，先解除锁存，
  // 后续 DisplayPanel 才能重新把背光打开。
  gpio_hold_dis(bsp::pins::LCD_BLK);

  // Use China Standard Time whenever a trusted epoch is supplied later.
  setenv("TZ", "CST-8", 1);
  tzset();

  esp_err_t nvs_result = nvs_flash_init();
  if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES ||
      nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    nvs_result = nvs_flash_init();
  }
  ESP_ERROR_CHECK(nvs_result);

  // 最终硬件隔离：完全不初始化 LCD/SPI/LVGL，只验证新版背光网络 GPIO47。
  static constexpr bool BACKLIGHT_ONLY_DIAGNOSTIC = false;
  if constexpr (BACKLIGHT_ONLY_DIAGNOSTIC) {
    ESP_LOGI(TAG, "=== CS COUPLING HARDWARE DIAGNOSTIC ===");
    ESP_LOGI(TAG, "GPIO47 stays HIGH; only CS GPIO11 toggles at 4 Hz");

    drivers::Power power({.power_ctl = bsp::pins::POWER_CTL,
                          .log_level = espp::Logger::Verbosity::INFO});

    gpio_config_t backlight_config{
        .pin_bit_mask = (1ULL << bsp::pins::LCD_BLK) | (1ULL << bsp::pins::LCD_CS),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&backlight_config));
    ESP_ERROR_CHECK(gpio_set_level(bsp::pins::LCD_BLK, 1));

    uint32_t heartbeat = 0;
    bool cs_level = false;
    while (true) {
      cs_level = !cs_level;
      ESP_ERROR_CHECK(gpio_set_level(bsp::pins::LCD_CS, cs_level ? 1 : 0));
      if ((heartbeat++ % 8) == 0) {
        ESP_LOGI(TAG, "CS_TOGGLE_RUNNING GPIO47=HIGH GPIO11=%d", cs_level ? 1 : 0);
      }
      vTaskDelay(pdMS_TO_TICKS(125));
    }
  }

  static constexpr bool LCD_ISOLATION_DIAGNOSTIC = false;
  if constexpr (LCD_ISOLATION_DIAGNOSTIC) {
    ESP_LOGI(TAG, "=== NEW BOARD LCD ISOLATION DIAGNOSTIC ===");
    ESP_LOGI(TAG, "MOSI=21 SCLK=14 CS=13 DC=12 RST=47 BLK=11");
    ESP_LOGI(TAG, "AS5600 and DRV2605L are disabled");
    drivers::Power power({.power_ctl = bsp::pins::POWER_CTL,
                          .log_level = espp::Logger::Verbosity::INFO});
    drivers::DisplayPanel display({
        .mosi = bsp::pins::LCD_MOSI, .sclk = bsp::pins::LCD_SCLK,
        .cs = bsp::pins::LCD_CS, .dc = bsp::pins::LCD_DC,
        .rst = bsp::pins::LCD_RST, .blk = bsp::pins::LCD_BLK,
        .width = 240, .height = 240,
        .log_level = espp::Logger::Verbosity::INFO,
    });
    if (!display.ok()) {
      ESP_LOGE(TAG, "LCD_DIAG_INIT_FAILED");
      while (true) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    const lv_color_t colors[] = {
        lv_color_make(255, 0, 0), lv_color_make(0, 255, 0),
        lv_color_make(0, 0, 255), lv_color_make(255, 255, 255),
        lv_color_make(0, 0, 0),
    };
    const char *names[] = {"RED", "GREEN", "BLUE", "WHITE", "BLACK"};
    size_t color_index = 0;
    while (true) {
      lv_obj_set_style_bg_color(screen, colors[color_index], 0);
      lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
      lv_obj_invalidate(screen);
      ESP_LOGI(TAG, "LCD_DIAG_COLOR=%s", names[color_index]);
      for (int i = 0; i < 200; ++i) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
      }
      color_index = (color_index + 1) % 5;
    }
  }

  ESP_LOGI(TAG, "\n");
  ESP_LOGI(TAG, "╔══════════════════════════════════════╗");
  ESP_LOGI(TAG, "║   Oracle OS - Clover Cat Edition    ║");
  ESP_LOGI(TAG, "║   ESP32-S3 Knob Firmware v0.1       ║");
  ESP_LOGI(TAG, "╚══════════════════════════════════════╝");
  ESP_LOGI(TAG, "\n");

  // ========================================
  // 1. 初始化板级驱动 (BSP)
  // ========================================
  ESP_LOGI(TAG, ">> 初始化板级驱动...");
  bsp::Board board;

  // ========================================
  // 2. 初始化场景管理器 (UI)
  // ========================================
  ESP_LOGI(TAG, ">> 初始化UI场景管理器...");
  ui::SceneManager scene_mgr({
      .initial = ui::SceneId::BOOT,
      .log_level = espp::Logger::Verbosity::INFO,
  });

  // ========================================
  // 3. 初始化旋钮服务 (输入处理)
  // ========================================
  ESP_LOGI(TAG, ">> 初始化旋钮服务...");

  // 选择输入源: 优先使用真实编码器,不可用则用串口模拟
  // 当前样机尚未装旋转组件。必须禁用霍尔输入，否则悬空/不稳定的磁场会让菜单乱跳。
  // 旋转组件装好并校准后改为 true，即恢复“旋转选择、短按进入”。
  static constexpr bool ROTARY_INSTALLED = false;
  drivers::IInputSource *input_source =
      ROTARY_INSTALLED && board.encoder()
          ? static_cast<drivers::IInputSource *>(board.encoder())
          : static_cast<drivers::IInputSource *>(&board.serial_source());
  ESP_LOGI(TAG, "输入模式: %s", ROTARY_INSTALLED ? "旋转组件" : "临时单键导航");

  app::KnobService knob({
      .initial_source = input_source,
      .haptic = &board.haptic(), // 震动反馈
      .detent_step_deg = 30.0f, // 每30度一格(12格/圈)
      .on_angle =
          [&](float accum_deg, int mech_deg) {
            // 角度更新回调(可用于调试,暂时不用)
          },
      .on_detent = {},
      .on_step =
          [&](int dir, float strength) {
            // 旋转一格 → 发送旋转事件给UI
            auto ev = (dir > 0) ? ui::Event::ROTATE_CW : ui::Event::ROTATE_CCW;
            scene_mgr.post_event(ev, strength);
          },
      .log_level = espp::Logger::Verbosity::INFO,
  });

  knob.start();

  // ========================================
  // 4. 注入SystemHooks (让UI场景能访问硬件)
  // ========================================
  ESP_LOGI(TAG, ">> 注入SystemHooks...");
  auto &hooks = app::SystemHooks::instance();

  // 亮度控制
  hooks.set_brightness = [&](float brightness) {
    board.display().set_brightness(brightness);
  };

  // 震动控制
  hooks.set_haptic_strength = [&](float strength) {
    // TODO: 实现震动强度控制
  };
  hooks.test_haptic = [&]() {
    board.haptic().click();
  };
  hooks.play_haptic_waveform = [&](int waveform_id) {
    board.haptic().play(waveform_id);
  };

  // 关机
  hooks.shutdown = [&]() {
    ESP_LOGI(TAG, "准备关机: 关闭背光并释放电池自锁...");

    // 显示驱动当前为常亮 GPIO 模式，不经过 PWM，所以直接关闭新版背光脚。
    gpio_set_level(bsp::pins::LCD_BLK, 0);
    // 轻睡眠期间数字 GPIO 可能切换到睡眠配置；锁存低电平避免背光重新亮起。
    ESP_ERROR_CHECK(gpio_hold_en(bsp::pins::LCD_BLK));

    // KEY=GPIO39 不是 ESP32-S3 RTC IO，不能从 deep sleep 唤醒；
    // GPIO light-sleep wakeup 支持该脚。USB 供电时进入轻睡眠，按键低电平唤醒。
    ESP_ERROR_CHECK(gpio_wakeup_enable(bsp::pins::KEY, GPIO_INTR_LOW_LEVEL));
    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());

    // 仅电池供电时这里会直接物理断电；USB 供电时程序继续进入轻睡眠。
    board.power().shutdown();

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "USB仍供电: 进入按键唤醒的轻睡眠");
    esp_light_sleep_start();

    // 按键唤醒后完整重启，重新初始化屏幕和所有外设。
    esp_restart();
  };

  // 输入源切换
  hooks.hall_available = [&]() -> bool {
    return board.encoder() != nullptr;
  };
  hooks.use_hall = [&](bool use_hall) {
    if (use_hall && board.encoder()) {
      knob.set_source(board.encoder());
      scene_mgr.set_status(board.battery().voltage().value_or(0.0f), true);
    } else {
      knob.set_source(&board.serial_source());
      scene_mgr.set_status(board.battery().voltage().value_or(0.0f), false);
    }
  };
  hooks.is_using_hall = [&]() -> bool {
    return input_source == board.encoder();
  };

  // 硬件诊断接口
  hooks.get_encoder_angle = [&]() -> float {
    auto *enc = board.encoder();
    return enc ? enc->mech_degrees() : 0.0f;
  };
  hooks.get_encoder_rpm = [&]() -> float {
    auto *enc = board.encoder();
    return enc ? enc->rpm() : 0.0f;
  };
  hooks.get_encoder_ok = [&]() -> bool {
    auto *enc = board.encoder();
    return enc && enc->ok();
  };
  hooks.get_battery_voltage = [&]() -> float {
    return board.battery().voltage().value_or(0.0f);
  };

  // ========================================
  // 5. 配置按键回调
  // ========================================
  board.set_button_callbacks(
      // 临时单键导航:短按等价于向下选择一项。
      // 首页收到旋转事件也会直接进入选择页。
      [&]() {
        ESP_LOGI(TAG, "按键: 短按");
        scene_mgr.post_event(ui::Event::ROTATE_CW);
        board.haptic().click();
      },
      // 双击 → 确认/进入
      [&]() {
        ESP_LOGI(TAG, "按键: 双击(确认)");
        scene_mgr.post_event(ui::Event::PRESS);
        board.haptic().strong();
      },
      // 长按3秒 → BACK事件
      [&]() {
        ESP_LOGI(TAG, "按键: 长按(返回)");
        scene_mgr.post_event(ui::Event::BACK);
        board.haptic().buzz(120);
      },
      // 长按10秒 → 关机确认
      [&]() {
        ESP_LOGI(TAG, "按键: 长按10秒(关机)");
        scene_mgr.post_event(ui::Event::POWER);
      });

  // ========================================
  // 6. 启动电池监控任务
  // ========================================
  ESP_LOGI(TAG, ">> 启动电池监控...");
  espp::Task::make_unique({
      .callback =
          [&](std::mutex &m, std::condition_variable &cv) -> bool {
            float voltage = board.battery().voltage().value_or(0.0f);
            bool using_hall = (input_source == board.encoder());
            scene_mgr.set_status(voltage, using_hall);

            // 低电量检测
            if (voltage < 3.3f) {
              ESP_LOGW(TAG, "电池电量低: %.2fV", voltage);
            }

            std::unique_lock<std::mutex> lk(m);
            cv.wait_for(lk, std::chrono::seconds(5)); // 每5秒更新一次
            return false;                              // 一直运行
          },
      .task_config = {.name = "Battery Monitor", .stack_size_bytes = 4096, .priority = 3},
  })->start();

  // ========================================
  // 7. 主循环: 按键轮询
  // ========================================
  ESP_LOGI(TAG, ">> 系统就绪,进入主循环");
  ESP_LOGI(TAG, "\n");

  while (true) {
    board.poll_button();                  // 轮询按键状态
    vTaskDelay(pdMS_TO_TICKS(10));       // 10ms轮询周期
  }
}
