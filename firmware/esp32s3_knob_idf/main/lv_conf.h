#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_USE_DEV_VERSION
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

// 内存配置 - ESP32S3有8MB PSRAM
#define LV_MEM_SIZE (128 * 1024U)
#define LV_MEM_POOL_INCLUDE "stdlib.h"
#define LV_MEM_POOL_ALLOC   malloc

// 显示配置
#define LV_DPI_DEF 130

// 字体配置 - 暂时使用内置字体
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_12

// 启用必要的部件
#define LV_USE_LABEL 1
#define LV_USE_IMG 1
#define LV_USE_CANVAS 1

// 日志配置
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

// 性能优化
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#endif
