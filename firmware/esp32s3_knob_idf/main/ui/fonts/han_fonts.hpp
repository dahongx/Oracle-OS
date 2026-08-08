#pragma once
// 中文子集字库声明(位图在 han_16.c / han_24.c,由 tools/gen_fonts.js 从 simhei.ttf 生成)。
// 字符集 = ui 源码里用到的 161 个汉字 + ASCII。改文案后需重新跑 collect_glyphs.py + gen_fonts.js。
#include "display.hpp" // 引入 lvgl 类型

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_font_t han_16; // 正文
extern const lv_font_t han_24; // 大字/标题

#ifdef __cplusplus
}
#endif
