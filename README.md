# Oracle OS — 掌心仪式挂件 (Palm Ritual Pendant)

一个通过**实体滚轮**完成「提问 · 旋转 · 沉静 · 揭示」的掌心仪式设备。
方形小屏 + 半嵌入式滚轮（可旋转 / 短按 / 长按），运行 Oracle OS 仪式型 UI。

硬件：ESP32-S3-WROOM-1-N8R2 + AS5600 磁编码器（滚轮角度）+ DRV2605L + LRA 线性马达（触觉）
+ SPI 屏（ST7789V2，非触摸，只写不收）+ 按键 + 电池供电（一键开关机自锁）。

> ### ⚠️ 硬件改版过一次，看引脚请只认两处
> **`firmware/esp32s3_knob_idf/main/bsp/pins.hpp`** 和**本文件下方的 GPIO 表**是当前有效的新板引脚。
>
> 以下位置的引脚是**改版前的旧板**，仅作历史参考，照着接会烧不亮：
> - `hardware/硬件自检说明.md`
> - `hardware/bringup-tests/` 下 5 个 PlatformIO 工程
>
> 新板引脚是 2026-07-27 用单线翻转法在实物上逐根测出来的（详见下方「踩坑记录」）。

## 目录结构

```
oracle-os/
├── README.md              # 本文件:项目总览 + 当前有效 GPIO
├── firmware/              # 产品固件(Oracle OS 在此实现)
│   └── esp32s3_knob_idf/  # ESP-IDF v5.5.1 + espp + LVGL,主固件
├── hardware/              # 硬件:原理图、数据手册、上电验证、自检文档
│   ├── doc/               # 原理图(Oracle OS.pdf) + 芯片/屏幕数据手册 + 厂家示例代码
│   ├── bringup-tests/     # 5 个上电验证工程(旧板引脚,历史参考)
│   └── 硬件自检说明.md      # 电源电路原理 + 串口踩坑(引脚表已过期)
├── design/                # Oracle OS UI 设计规范 + 参考图 + 网页原型
├── product/               # 产品任务清单 + 猫 IP 精灵图源图
└── 思迈微1.54寸-12P插接技术手册/  # 另一款 ST7789 屏的厂家资料(初始化序列可参考)
```

## 当前有效 GPIO（新板）

| 功能 | GPIO |
|------|------|
| AS5600 I2C（滚轮角度，I2C0） | SDA=1, SCL=2 |
| DRV2605L I2C（触觉，I2C1） | SDA=8, SCL=9, EN=10 |
| LCD 背光 BLK | 11 |
| LCD DC | 12 |
| LCD CS | 13 |
| LCD SCLK | 14 |
| LCD MOSI | 21 |
| LCD RST | 47 |
| KEY 按键 | 39（按下为低，板上 10K 上拉） |
| POWER_CTL 自锁 | 40（拉高维持供电，拉低断电） |
| BAT_ADC 电池采样 | 4（ADC1_CH3，100K/100K 分压，实际电压 = 读数 × 2） |
| 原生 USB D-/D+ | 19/20（芯片固定功能脚） |

`DRV2605L` 的 `EN` **必须先拉高**芯片才应答 I2C（原理图 IN_TRIG 直接接地，只能走 I2C 内部触发）。

## 屏幕配置

固件按 **240×240** 配置（`pins.hpp` 同目录的 `board.cpp` / `display_panel.hpp`），
初始化用厂家序列并**开启反色 `0x21`**（关掉会整屏逐色取反：红变青、白变黑）。

> **待确认**：面板实物是 2.8" 240×320，但按 240×320 排版时底部约 80px 内容落在不可视区外，
> 改成 240×240 后所有页面完整显示。有效可视区为什么是正方形还没查清 —— 动分辨率前先确认。

## 硬件状态

| 外设 | 状态 |
|------|------|
| 主控 / USB 下载 / 串口 | ✅ 已验证 |
| SPI 屏幕 | ✅ 已点亮，颜色正确 |
| 按键 KEY | ✅ 短按 / 双击 / 长按 3s / 长按 10s |
| DRV2605L + LRA 马达 | ✅ 已接入 UI 各交互点 |
| 电池 ADC | ✅ |
| 一键开关机自锁 | ✅ 含 USB 供电下的软关机（轻睡眠 + 按键唤醒） |
| AS5600 磁编码器 | ⚠️ 新板上持续 I2C NACK；**旋转组件尚未装配** |

因为旋转组件没装，固件里 `ROTARY_INSTALLED = false`（`app_main.cpp`），
霍尔输入被停用，改走**临时单键导航**。装好旋转件后把它改成 `true` 即恢复正式方案。

## 交互（临时单键模式）

| 操作 | 行为 |
|------|------|
| 首页短按 | 进入「选择仪式」 |
| 选择页短按 | 切换到下一项 |
| 选择页双击 | 进入当前仪式 |
| 任意子页长按 3 秒 | 返回上一页 |
| 首页长按 10 秒 | 进入关机确认 |
| 关机确认页短按 | 立即关机 |

旋转件装好后改为：旋转选择 / 短按进入 / 长按 3 秒返回。

## 五个仪式

| 菜单名 | 场景文件 | SceneId |
|--------|---------|---------|
| 今日幸运 | `ui/scenes/daily_scene.hpp` | `DAILY` |
| 逗猫时光 | `ui/scenes/play_scene.hpp` | `PLAY` |
| 四叶呼吸 | `ui/scenes/orbit_scene.hpp` | `ORBIT` |
| 吹走厄运 | `ui/scenes/quick_ask_scene.hpp` | `QUICK_ASK` |
| 揍走烦恼 | `ui/scenes/triad_scene.hpp` | `TRIAD` |

> 文件名和 `SceneId` 是早期占卜版留下的，和现在的仪式名对不上（`QUICK_ASK` 其实是「吹走厄运」）。
> 改名是待办项，改时记得同步 `scene.hpp` 的枚举注释和 `scene_manager.cpp` 的 switch。
>
> `MOOD` / `HISTORY` / `SETTINGS` / `HARDWARE_DIAG` 四个场景仍在编译，但菜单里没有入口，
> 运行时进不去 —— 是旧版遗留，留着以后做设置页，或按需删除。

## 编译烧录

本工程**关闭了 IDF Component Manager**，直接用本机 clone 的 [espp](https://github.com/esp-cpp/espp) 仓库。

```bash
# 1. 一次性:拉 espp,并告诉工程它在哪
git clone https://github.com/esp-cpp/espp.git
setx ESPP_PATH D:\esp\espp          # Windows;换成你自己的路径

# 2. 编译烧录
cd firmware/esp32s3_knob_idf
idf.py build
idf.py -p COM8 flash monitor        # 串口号看设备管理器,VID=303A 那个
```

没设 `ESPP_PATH` 时会回退到 `D:/esp/espp`，找不到目录会在 configure 阶段直接报错。

> 用板子自带 Type-C 原生 USB 烧录。**看串口打印要用真正的终端软件**，
> 不要用脚本手动摆 DTR/RTS —— 原因见 `hardware/硬件自检说明.md`。

## 资源生成

字模和猫 IP 精灵图是**生成物**，源图/字体已入库，改动后需重新生成：

```bash
cd firmware/esp32s3_knob_idf/tools
npm install                    # 一次性,装 lv_font_conv
node gen_fonts.js              # 生成 main/ui/fonts/han_16.c、han_24.c
python build_cat_sprites.py    # 生成 main/ui/cat_sprites.c
```

`build_cat_sprites.py` 同时读 `product/` 下**三张**源图，缺一不可：
`oracle-cat-ip-sprites-v3.png`（主源）、`oracle-cat-ip-sprites-v2.png`（选择页探头猫）、
`oracle-cat-tail-sprites-v4.png`（尾巴）。

## 踩坑记录

**LCD 整组引脚曾经错位**（2026-07-27 修正）。症状很迷惑：背光随刷屏节奏快闪、屏幕全黑。
根因是固件把 `GPIO11` 当 CS、把 `GPIO47` 当背光，而实物上 `GPIO11` 就是背光 ——
于是每次 SPI 片选都在高速开关背光板。

定位方法：关掉 LCD/SPI/LVGL，只让**单根** GPIO 以 4Hz 翻转，肉眼看背光反应。
`GPIO14`、`GPIO13` 翻转时背光稳定，翻到 `GPIO11` 时背光跟着闪 —— 一根就锁定了。
以后遇到「外设行为和另一个外设耦合」，这个单线隔离法比查原理图快。
