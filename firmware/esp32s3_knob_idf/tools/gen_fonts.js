// 生成中文子集 LVGL 字库(v9,4bpp)。
// 源字体:Fusion Pixel Font 12px proportional zh_hans (SIL OFL 1.1)。
// Ark Pixel Font 当前字形覆盖不足，必须在生成前验证 UI 子集没有缺字。
// 字符集 = ui 源码里提取的汉字(symbols.txt) + ASCII 0x20-0x7F(数字/字母/标点/V% 等)。
// 输出两个字号:16px(正文)/ 24px(大字标题、答案)。
const { execFileSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const here = __dirname;
const conv = path.join(here, 'node_modules', 'lv_font_conv', 'lv_font_conv.js');
const font = path.join(here, 'fonts', 'fusion-pixel-12px-proportional-zh_hans.ttf');
const symbols = fs.readFileSync(path.join(here, 'symbols.txt'), 'utf8').trim();
const outDir = path.join(here, '..', 'main', 'ui', 'fonts');
fs.mkdirSync(outDir, { recursive: true });

// Fail before conversion if a selected font cannot render every requested
// Chinese character. lv_font_conv otherwise silently emits .notdef boxes.
const opentype = require('./node_modules/lv_font_conv/node_modules/opentype.js');
const parsed = opentype.loadSync(font);
const missing = [...symbols].filter(ch => parsed.charToGlyphIndex(ch) === 0);
if (missing.length) {
  throw new Error(`字体缺少 ${missing.length} 个 UI 字形: ${missing.join('')}`);
}
console.log(`字形覆盖检查通过: ${[...symbols].length} 个中文字符`);

function gen(size, name) {
  const out = path.join(outDir, name + '.c');
  const args = [
    conv,
    '--font', font,
    '-r', '0x20-0x7F',        // ASCII(数字/字母/标点)
    '--symbols', symbols,      // 汉字子集
    '--size', String(size),
    '--bpp', '4',
    '--format', 'lvgl',
    '--no-compress',           // 不压缩,解码更省 CPU(字库小,体积无所谓)
    '--lv-font-name', name,
    '-o', out,
  ];
  console.log(`生成 ${name} (size=${size}) ...`);
  execFileSync('node', args, { stdio: 'inherit' });
  const kb = (fs.statSync(out).size / 1024).toFixed(1);
  console.log(`  -> ${out}  (${kb} KB)`);
}

gen(16, 'han_16');
gen(24, 'han_24');
console.log('完成');
