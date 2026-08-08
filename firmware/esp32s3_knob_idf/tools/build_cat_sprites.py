"""Build layered Oracle cat sprites from the approved 4x3 V3 IP sheet."""
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[2] / "product" / "oracle-cat-ip-sprites-v3.png"
LEGACY_SOURCE = HERE.parents[2] / "product" / "oracle-cat-ip-sprites-v2.png"
TAIL_SOURCE = HERE.parents[2] / "product" / "oracle-cat-tail-sprites-v4.png"
OUT_C = HERE.parent / "main" / "ui" / "cat_sprites.c"
SIZE = 96
COLS = 4
ROWS = 3
NAMES = (
    "sit", "blink", "closed", "tail",
    "blow", "play", "pounce", "catch",
    "breathe", "swipe", "relax", "clover",
)

def remove_legacy_chroma(im: Image.Image) -> Image.Image:
    rgba = im.convert("RGBA")
    out = []
    for r, g, b, _ in rgba.getdata():
        keyness = min(r, b) - g
        if r > 170 and b > 150 and keyness > 90:
            alpha = max(0, min(255, 255 - (keyness - 90) * 3))
            out.append((r, g, b, alpha))
        else:
            out.append((r, g, b, 255))
    rgba.putdata(out)
    return rgba


def fit_sprite(im: Image.Image, box=None) -> Image.Image:
    if box is None:
        box = im.getchannel("A").getbbox()
    if box:
        im = im.crop(box)
    inner = SIZE - 6
    scale = min(inner / im.width, inner / im.height)
    target = (max(1, round(im.width * scale)), max(1, round(im.height * scale)))
    im = im.resize(target, Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    canvas.alpha_composite(im, ((SIZE - im.width) // 2, (SIZE - im.height) // 2))
    return canvas


def make_stable_blinks(base: Image.Image) -> tuple[Image.Image, Image.Image]:
    """Build both blink stages from SIT so no face pixel can jump frames."""
    mask = Image.new("L", base.size, 0)
    draw = ImageDraw.Draw(mask)
    draw.ellipse((31, 32, 47, 51), fill=255)
    draw.ellipse((56, 32, 72, 51), fill=255)

    # Diffuse the surrounding fur colour into the eye interiors. This removes
    # the open eyes without borrowing lighting or geometry from another pose.
    masked = [v > 0 for v in mask.getdata()]
    pixels = [list(px) for px in base.getdata()]
    boundary = []
    for y in range(28, 55):
        for x in range(27, 77):
            i = y * SIZE + x
            r, g, b, a = pixels[i]
            if not masked[i] and a > 0 and not (r > 125 and g > 85 and b < 100):
                boundary.append((r, g, b, a))
    mean = [
        sum(px[channel] for px in boundary) // len(boundary)
        for channel in range(4)
    ]
    for i, is_masked in enumerate(masked):
        if is_masked:
            pixels[i] = mean.copy()

    for _ in range(90):
        next_pixels = [px.copy() for px in pixels]
        for y in range(1, SIZE - 1):
            for x in range(1, SIZE - 1):
                i = y * SIZE + x
                if not masked[i]:
                    continue
                neighbours = (
                    pixels[i - 1], pixels[i + 1],
                    pixels[i - SIZE], pixels[i + SIZE],
                )
                next_pixels[i] = [
                    sum(px[channel] for px in neighbours) // 4
                    for channel in range(4)
                ]
        pixels = next_pixels

    fur = Image.new("RGBA", base.size)
    fur.putdata([tuple(px) for px in pixels])
    feather = mask.filter(ImageFilter.GaussianBlur(0.45))
    clean_face = Image.composite(fur, base, feather)

    blink = clean_face.copy()
    blink_lids = ImageDraw.Draw(blink)
    eyelid_gold = (231, 195, 90, 255)
    blink_lids.arc((32, 34, 46, 48), 0, 180, fill=eyelid_gold, width=2)
    blink_lids.arc((57, 34, 71, 48), 0, 180, fill=eyelid_gold, width=2)

    closed = clean_face.copy()
    closed_lids = ImageDraw.Draw(closed)
    closed_lids.arc((32, 38, 46, 48), 0, 180, fill=eyelid_gold, width=2)
    closed_lids.arc((57, 38, 71, 48), 0, 180, fill=eyelid_gold, width=2)
    return blink, closed


def emit_array(name: str, im: Image.Image) -> str:
    values = []
    for r, g, b, a in im.getdata():
        values.extend((b, g, r, a))
    lines = []
    for i in range(0, len(values), 24):
        lines.append("  " + ",".join(f"0x{v:02X}" for v in values[i:i + 24]) + ",")
    return f"const uint8_t cat_{name}_map[] = {{\n" + "\n".join(lines) + "\n};\n"


sheet = Image.open(SOURCE).convert("RGBA")
w, h = sheet.size
chunks = ['#include <stdint.h>\n']
preview = Image.new("RGBA", (SIZE * COLS, SIZE * ROWS), (15, 20, 23, 255))
cells = []
for index in range(len(NAMES)):
    col, row = index % COLS, index // COLS
    cells.append(sheet.crop((round(col * w / COLS), round(row * h / ROWS),
                             round((col + 1) * w / COLS),
                             round((row + 1) * h / ROWS))))

# SIT / BLINK / CLOSED must share one crop and one scale. Independent tight
# crops make the whole cat shift during a blink even when the source poses are
# visually aligned.
boxes = [cells[i].getchannel("A").getbbox() for i in range(3)]
common_blink_box = (
    min(b[0] for b in boxes), min(b[1] for b in boxes),
    max(b[2] for b in boxes), max(b[3] for b in boxes),
)

sprites = [fit_sprite(cells[index], common_blink_box if index < 3 else None)
           for index in range(len(NAMES))]

# Blink frames share the exact same head/body and differ only at the eyes.
sprites[1], sprites[2] = make_stable_blinks(sprites[0])

# Keep V3's original short, proportionate clover tail for Home. V4 only
# supplies the two bare game tails and the complete right-facing blowing cat.
tail_sheet = Image.open(TAIL_SOURCE).convert("RGBA")
tw, th = tail_sheet.size
tail_cells = [
    tail_sheet.crop((round(col * tw / 2), round(row * th / 2),
                     round((col + 1) * tw / 2), round((row + 1) * th / 2)))
    for row in range(2) for col in range(2)
]
bare_tail = fit_sprite(tail_cells[0])
bare_tail_left = fit_sprite(tail_cells[1])
sprites[4] = fit_sprite(tail_cells[3])

# Preserve the original side-profile peek approved for the selection page.
legacy = Image.open(LEGACY_SOURCE)
lw, lh = legacy.size
look_cell = legacy.crop((0, round(lh / 3), round(lw / 3), round(2 * lh / 3)))
menu_peek = fit_sprite(remove_legacy_chroma(look_cell))

for index, name in enumerate(NAMES):
    col, row = index % COLS, index // COLS
    sprite = sprites[index]
    chunks.append(emit_array(name, sprite))
    preview.alpha_composite(sprite, (col * SIZE, row * SIZE))

chunks.append(emit_array("menu_peek", menu_peek))
chunks.append(emit_array("bare_tail", bare_tail))
chunks.append(emit_array("bare_tail_left", bare_tail_left))

OUT_C.write_text("\n".join(chunks), encoding="utf-8")
preview.save(HERE / "cat_sprites_preview.png")
menu_peek.save(HERE / "menu_peek_preview.png")
bare_tail.save(HERE / "bare_tail_preview.png")
print(f"Wrote {OUT_C}")
