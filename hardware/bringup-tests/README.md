# 硬件上电验证工程（旧板，历史参考）

> ⚠️ **这 5 个工程全部是硬件改版前写的，引脚已过时，不要直接烧到现在的板子上。**
> 它们的价值在于**验证方法和踩坑结论**，不在于引脚数值。

## 为什么保留

这批工程当年把板子从"电源通不通"一路验证到"屏幕能不能显示画面"，
里面的结论到现在依然有效，比如：

- ESP32-S3 原生 USB 的串口打印必须用真正的终端软件，用脚本摆 DTR/RTS 会永远看不到输出
- 用脚本对原生 USB 口做复位会被 ROM 强制带进下载模式（芯片既定行为，不是 bug）
- DRV2605L 的 EN 脚必须先拉高，芯片才应答 I2C
- 一键开关机自锁电路：MCU 启动后必须尽快拉高 POWER_CTL，否则松手就断电

这些内容写在 `hardware/硬件自检说明.md` 里，那份文档同样需要注意新旧板差异。

## 新旧引脚对照

| 功能 | 旧板（本目录代码） | 当前板 |
|---|---|---|
| DRV2605L SDA | GPIO3 | **GPIO8** |
| DRV2605L SCL | GPIO8 | **GPIO9** |
| DRV2605L EN | GPIO46 | **GPIO10** |
| LCD CS | GPIO9 | **GPIO13** |
| LCD DC | GPIO10 | **GPIO12** |
| LCD SCLK | GPIO11 | **GPIO14** |
| LCD MOSI | GPIO12 | **GPIO21** |
| LCD RST | GPIO13 | **GPIO47** |
| LCD BLK（背光） | GPIO14 | **GPIO11** |
| AS5600 SDA / SCL | GPIO1 / GPIO2 | 不变 |
| KEY | GPIO39 | 不变 |
| POWER_CTL | GPIO40 | 不变 |
| BAT_ADC | GPIO4 | 不变 |

**最危险的一处**：新板的 **GPIO11 是背光**，而旧代码把 GPIO11 当 SCLK 高速翻转。
直接烧旧固件的症状是"背光疯闪 + 屏幕全黑"，很容易被误判成屏幕坏了或排线插反。
当年就是靠逐根单线翻转 GPIO、观察背光反应，才定位到整组 LCD 引脚错位。

## 各工程用途

| 工程 | 用途 | 引脚是否受改版影响 |
|---|---|---|
| `esp32s3_serial_test` | USB 下载 + 串口打印 | 不涉及引脚，**仍可直接用** |
| `esp32s3_i2c_scan_test` | 扫两条 I2C + AS5600 角度 + KEY + 电池 | DRV 部分过时，AS5600/KEY/ADC 仍有效 |
| `esp32s3_motor_test` | DRV2605L 震动 | 三个引脚全部过时 |
| `esp32s3_lcd_id_test` | 原始 SPI 读屏 ID | 六个引脚全部过时 |
| `esp32s3_lcd_fill_test` | TFT_eSPI 填色点亮 | 六个引脚全部过时 |

## 想在当前板上复用

改 `src/main.cpp`（或 `platformio.ini` 的 `build_flags`）里的引脚宏，按上面对照表替换即可，
逻辑部分不用动。屏幕相关的还要注意有效显示区是 **240×240**，不是 240×320。

正式固件在 `firmware/esp32s3_knob_idf/`，引脚统一由 `main/bsp/pins.hpp` 提供，
那里是全项目唯一的引脚来源。
