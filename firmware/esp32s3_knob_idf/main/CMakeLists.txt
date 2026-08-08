# 分层架构:各层源码分目录存放。SRC_DIRS 逐目录列出,INCLUDE_DIRS 用 "." 让
# #include "bsp/xxx.hpp" / "drivers/xxx.hpp" / "app/xxx.hpp" / "ui/xxx.hpp" 都能解析。
# 注:ui/ 下含生成的 cat_sprites.c(猫咪精灵图数据,由 tools/build_cat_sprites.py 生成)
#     和 ui/fonts/han_16.c、han_24.c(中文字模,由 tools/gen_fonts.js 生成)。
#     新增源文件后需 reconfigure(idf.py reconfigure 或删 build/ 重建)。
#
# ⚠ SRC_DIRS 含 "." ,意味着 main/ 根目录下所有 .c/.cpp 都会被编译。
#   不要在这里放"备份版" app_main(会和 app_main.cpp 撞 app_main() 符号,
#   而且靠链接顺序决定谁生效,极难排查)。退役代码请放到项目根的 _attic/。
idf_component_register(
    SRC_DIRS "." "bsp" "drivers" "app" "ui" "ui/fonts" "ui/scenes"
    INCLUDE_DIRS "."
    REQUIRES
      nvs_flash i2c as5600 drv2605 button adc display display_drivers spi
      filters task logger led format base_component esp_driver_gpio
      esp_driver_spi esp_driver_i2c esp_adc esp_driver_usb_serial_jtag lvgl)
