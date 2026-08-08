# Oracle OS Web UI — checked code-art edition

主页面中的猫猫不是 PNG、JPEG、WebP 或 CSS 背景图。

它由 `index.html` 内部的 SVG `<path>` 代码构成：
- 基础网格：80 × 65
- 显示尺寸：240 × 195
- 固定 3 倍整数放大
- `shape-rendering="crispEdges"`
- 不引用任何外部图像资源

直接打开 `index.html` 即可运行。

交互：
- 鼠标滚轮 / ↑↓：旋转
- 点击实体滚轮 / Enter：短按
- 按住实体滚轮 1.2 秒 / Esc：返回
