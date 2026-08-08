# DRV2605L + 线性马达 震动测试

> ⚠️ **旧板引脚，在当前硬件上跑不通。** 本工程写死的是改版前的 GPIO：
> `SDA=3  SCL=8  EN=46`。当前板已改为 **`SDA=8  SCL=9  EN=10`**。
> 仅作历史参考，**不要直接烧到现在的板子上**。
> 现行引脚见仓库根 `README.md` 或 `firmware/esp32s3_knob_idf/main/bsp/pins.hpp`。

引脚已根据**改版前**的原理图核实,见 `hardware/硬件自检说明.md`。**关键点**: DRV2605L 的 EN 脚
必须由固件拉高,芯片才会应答 I2C——这也是和 `esp32s3_i2c_scan_test` 工程的主要区别。
(这一条结论在新板上依然成立,只是 EN 从 GPIO46 换到了 GPIO10。)

## 使用步骤

1. VSCode + PlatformIO 打开本文件夹
2. Upload(`pio run -t upload`)
3. `pio device monitor` 查看输出

## 预期现象

- 串口打印 "DRV2605L 已找到" 且没有报错
- 之后每 1.5 秒打印一次 "播放 effect #xx",此时马达应该有轻微震动感/能听到细微声音
- 如果一直提示"没有找到 DRV2605L",检查:
  - GPIO46(EN)有没有真的接到 DRV2605L 的 EN 脚并且被拉高了(可以用万用表在 EN 脚测电压,应该接近 3.3V)
  - I2C 接线(GPIO3/GPIO8)对不对
  - 芯片供电(3V3/GND)
- 如果 I2C 通了、effect 也在打印,但马达完全没反应,大概率是 M+/M- 到马达这段焊接或马达本身的问题
