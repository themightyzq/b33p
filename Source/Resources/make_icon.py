import os
from PIL import Image, ImageDraw, ImageFont

OUT_DIR = "/Users/zacharylquarles/PROJECTS_Apps/Project_b33p/Source/Resources"

BG  = (24, 32, 48, 255)
FG  = (140, 210, 255, 255)
SUB = (60, 140, 220, 255)

# Prefer explicitly-bold faces; fall back through the system Helvetica
# collection (which contains a bold variant) and finally PIL's default.
BOLD_FONTS = [
    "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
    "/System/Library/Fonts/HelveticaNeue.ttc",
    "/System/Library/Fonts/Helvetica.ttc",
]

def load_font(font_size):
    for path in BOLD_FONTS:
        if os.path.exists(path):
            try:
                return ImageFont.truetype(path, font_size)
            except OSError:
                continue
    return ImageFont.load_default()

def draw_icon(size):
    img = Image.new("RGBA", (size, size), BG)
    draw = ImageDraw.Draw(img)

    inset = max(2, int(size * 0.06))
    draw.rounded_rectangle(
        (inset, inset, size - inset, size - inset),
        radius=int(size * 0.12),
        fill=(36, 44, 64, 255),
        outline=SUB,
        width=max(1, int(size * 0.012)),
    )

    text = "b33p"
    panel_w = size - 2 * inset
    panel_inner_pad = max(4, int(size * 0.08))
    target_text_w = panel_w - 2 * panel_inner_pad

    # Bisect a font size so the rendered text width fits the panel.
    lo, hi = 8, size
    while lo < hi:
        mid = (lo + hi + 1) // 2
        font = load_font(mid)
        bbox = draw.textbbox((0, 0), text, font=font)
        if (bbox[2] - bbox[0]) <= target_text_w:
            lo = mid
        else:
            hi = mid - 1
    font = load_font(lo)
    bbox = draw.textbbox((0, 0), text, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]
    x = (size - text_w) / 2 - bbox[0]
    y = (size - text_h) / 2 - bbox[1]
    draw.text((x, y), text, fill=FG, font=font)
    return img

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    big = draw_icon(1024)
    small = draw_icon(64)
    big.save(os.path.join(OUT_DIR, "icon_big.png"), "PNG")
    small.save(os.path.join(OUT_DIR, "icon_small.png"), "PNG")
    print("Wrote", os.path.join(OUT_DIR, "icon_big.png"))
    print("Wrote", os.path.join(OUT_DIR, "icon_small.png"))

if __name__ == "__main__":
    main()
