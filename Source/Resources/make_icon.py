#!/usr/bin/env python3
"""Render the b33p app icon at every size macOS / Windows / Linux need.

Outputs (relative to this script's directory):
  icon_big.png   — 1024×1024, picked up by JUCE's juce_add_plugin
                   ICON_BIG and used to drive the Windows .ico /
                   non-bundle Linux PNG.
  icon_small.png — 64×64, picked up by JUCE's ICON_SMALL.
  Icon.icns      — macOS .icns containing 16, 32, 64, 128, 256,
                   512, 1024 single-resolution sizes plus the
                   conventional @2x retina variants. CMake's
                   post-build step on macOS overwrites JUCE's
                   default .icns (which only carries 48 + 1024@2x)
                   with this one so the dock / Finder pick the
                   right intrinsic size at every display scale.

Run on macOS:
  python3 -m venv /tmp/iconvenv
  /tmp/iconvenv/bin/pip install Pillow
  /tmp/iconvenv/bin/python make_icon.py

The Icon.icns step requires the macOS-only `iconutil` binary; on
non-macOS hosts the script skips the .icns and just regenerates
the PNGs.
"""

import os
import platform
import shutil
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFont

# Resolve every output path relative to this script so the file is
# safe to run from any working directory and doesn't bake in any
# personal-machine paths.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
OUT_DIR = SCRIPT_DIR

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

# Sizes the macOS .icns expects, per Apple's Human Interface
# Guidelines. Each entry produces a single-resolution variant; the
# write_iconset step also lays down @2x retina variants for every
# size below 1024.
ICNS_SIZES = [16, 32, 64, 128, 256, 512, 1024]


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


def write_iconset(directory):
    """Render every required size into an `.iconset` directory.

    Apple's iconset naming convention pairs every base size with an
    `@2x` variant rendered at twice the resolution. We render each
    size from scratch so the bisected font-fitting matches the
    target resolution.
    """
    os.makedirs(directory, exist_ok=True)
    for base in ICNS_SIZES:
        # 1x — exact size.
        draw_icon(base).save(
            os.path.join(directory, f"icon_{base}x{base}.png"), "PNG")
        # 2x retina — only meaningful below the largest size; macOS
        # doesn't define a 2048×2048 retina variant for the 1024
        # base.
        if base < 1024:
            draw_icon(base * 2).save(
                os.path.join(directory, f"icon_{base}x{base}@2x.png"), "PNG")


def build_icns(iconset_dir, output_path):
    """Run `iconutil -c icns` to bundle an iconset into a .icns file.

    Returns True on success; False (with an explanation printed) when
    iconutil is unavailable. Non-macOS hosts hit the latter path.
    """
    iconutil = shutil.which("iconutil")
    if iconutil is None:
        print("iconutil not found — skipping .icns generation "
              "(non-macOS host or Xcode CLI tools missing)")
        return False

    subprocess.run([iconutil, "-c", "icns", iconset_dir,
                    "-o", output_path], check=True)
    return True


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    # JUCE inputs (drive the Windows .ico + Linux freestanding PNGs).
    draw_icon(1024).save(os.path.join(OUT_DIR, "icon_big.png"), "PNG")
    draw_icon(64)  .save(os.path.join(OUT_DIR, "icon_small.png"), "PNG")
    print("Wrote", os.path.join(OUT_DIR, "icon_big.png"))
    print("Wrote", os.path.join(OUT_DIR, "icon_small.png"))

    # macOS multi-size .icns. CMakeLists' post-build step copies this
    # over JUCE's juceaide-generated .icns inside the .app bundle.
    iconset_dir = os.path.join(OUT_DIR, "Icon.iconset")
    icns_path   = os.path.join(OUT_DIR, "Icon.icns")
    write_iconset(iconset_dir)
    if build_icns(iconset_dir, icns_path):
        print("Wrote", icns_path)
        # iconset/ is build scratch; remove it so the repo only
        # carries the canonical .icns + .png inputs.
        shutil.rmtree(iconset_dir, ignore_errors=True)
    else:
        # On a non-macOS host we leave the iconset around as a
        # courtesy so the PNGs can still serve as a Linux icon
        # source.
        print("Iconset PNGs left at", iconset_dir)
        if platform.system() != "Darwin":
            print("(Build the .icns on a macOS host to ship the dock icon.)")


if __name__ == "__main__":
    sys.exit(main() or 0)
