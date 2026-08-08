#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
从 UI 源码里【只提取双引号字符串字面量】中的汉字,汇总成字库子集所需的字符集。
- 只扫 main/ui 下的 .hpp/.cpp(界面显示文本都在这里;注释里的中文不会进字库)
- 只取 "..." 里的内容,过滤掉注释,避免把大量注释汉字塞进字库
- 另加 scene_manager.cpp 里状态栏的 霍/模
输出:
  - glyphs_cjk.txt : 去重排序后的汉字(供人工核对)
  - symbols.txt    : lv_font_conv 用的完整 symbols 串(CJK,不含 ASCII,ASCII 用 --range 单独加)
"""
import os
import re
import sys

UI_DIR = os.path.join(os.path.dirname(__file__), "..", "main", "ui")
EXTRA_FILES = [
    os.path.join(os.path.dirname(__file__), "..", "main", "ui", "scene_manager.cpp"),
]

# 匹配双引号字符串(处理转义)
STR_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')

def is_cjk(ch):
    o = ord(ch)
    return (0x4E00 <= o <= 0x9FFF) or (0x3000 <= o <= 0x303F) or (0xFF00 <= o <= 0xFFEF)

def strip_line_comment(line):
    # 去掉 // 之后的内容(简单处理,足够:UI 文件里不会有 // 出现在字符串内的复杂情况)
    idx = line.find("//")
    if idx >= 0:
        line = line[:idx]
    return line

chars = set()
files = []
for root, _dirs, names in os.walk(UI_DIR):
    for n in names:
        if n.endswith(".hpp") or n.endswith(".cpp"):
            files.append(os.path.join(root, n))

for path in files:
    try:
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = strip_line_comment(line)
                for m in STR_RE.finditer(line):
                    for ch in m.group(1):
                        if is_cjk(ch):
                            chars.add(ch)
    except Exception as e:
        print(f"skip {path}: {e}", file=sys.stderr)

cjk_sorted = "".join(sorted(chars))
out_dir = os.path.dirname(__file__)
with open(os.path.join(out_dir, "glyphs_cjk.txt"), "w", encoding="utf-8") as f:
    f.write(cjk_sorted)
with open(os.path.join(out_dir, "symbols.txt"), "w", encoding="utf-8") as f:
    f.write(cjk_sorted)

print(f"扫描文件数: {len(files)}")
print(f"唯一汉字数: {len(chars)}")
print(cjk_sorted)
