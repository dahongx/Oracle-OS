# I2C 扫描 + AS5600 + KEY + 电池电压 测试

> ⚠️ **部分引脚是旧板的。** DRV2605L 那条总线写死的是改版前的 `SDA=3 SCL=8`,
> 当前板已改为 **`SDA=8 SCL=9`**(EN 也从 46 换到 10)。
> AS5600(`SDA=1 SCL=2`)、KEY(39)、BAT_ADC(4) 三项**新旧板一致**,这部分仍可参考。
> 现行引脚见仓库根 `README.md` 或 `firmware/esp32s3_knob_idf/main/bsp/pins.hpp`。

引脚已根据**改版前**的原理图核实,见 `hardware/硬件自检说明.md`。

## 使用步骤

1. VSCode + PlatformIO 打开本文件夹
2. Upload(`pio run -t upload`),如自动下载失败,参照 `esp32s3_serial_test/README.md` 里手动进 BOOT 模式的步骤
3. `pio device monitor` 查看输出

## 预期现象

- 两条 I2C 总线扫描结果里能看到 `0x36`(AS5600)。DRV2605L(`0x5A`)在这个工程里大概率**扫不到**,
  因为它的 EN 脚(GPIO46)在这个工程里没有拉高 —— 这是正常的,DRV2605L 的测试请看 `esp32s3_motor_test`。
- 转动 AS5600 上方的磁铁,`angle` 数值应该跟着连续变化,不应该跳变或卡死不动。
- 按下 BUTTON,`KEY` 打印值应该从 1 变成 0。
- `BAT` 电压如果接了电池,数值应该接近电池实际电压(±0.1V 左右,电阻精度影响);
  只用 USB 供电、没接电池的话,这个数值没有参考意义(悬空或者是漏电流决定的杂散值)。
