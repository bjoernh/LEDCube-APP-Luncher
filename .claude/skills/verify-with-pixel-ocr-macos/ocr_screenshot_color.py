#!/usr/bin/env python3
"""
Color-sensitive pixel analysis for cube_launcher screenshots.

Unlike ocr_screenshot.py (which matches text shapes via font bitmaps),
this script answers a different question: WHAT COLORS are rendered where?
It does NOT do glyph matching — instead it classifies each detected text
band by its dominant color and prints per-row color breakdowns plus a
raw pixel map. Use it to verify highlight colors, focus indicators,
selection states, or any rendering that is about color rather than text.

The launcher renders a 64x64 framebuffer at SDL 4x scale (256x256 PNG).
This tool downscales to 64x64, samples one point per logical pixel at
the center of each 4x4 SDL block, and classifies rows by dominant color.

Color names: WHITE, BLUE, RED, GREEN, YELLOW, MIX, BLACK.

Usage:
    python3 ocr_screenshot_color.py <screenshot.png>
    python3 ocr_screenshot_color.py <screenshot.png> --expect-color BLUE 2
        # assert that at least 2 bands are classified BLUE
    python3 ocr_screenshot_color.py <screenshot.png> --row 26
        # dump per-pixel RGB for a single logical row
"""

import argparse
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("error: pip install Pillow")

SCREEN_W = 64
SCREEN_H = 64
LIT_THRESHOLD = 60   # sum of R+G+B below this → treated as background/black

KNOWN_COLORS = ('WHITE', 'BLUE', 'RED', 'GREEN', 'YELLOW', 'MIX', 'BLACK')


def downscale(img):
    """Crop off the macOS window titlebar and downscale to the 64x64 logical
    framebuffer using nearest-neighbour (preserves crisp pixel-art edges)."""
    if img.size == (SCREEN_W, SCREEN_H):
        return img
    w, h = img.size
    if h > w:
        img = img.crop((0, h - w, w, h))
    return img.resize((SCREEN_W, SCREEN_H), Image.NEAREST)


def color_pixels(img):
    """Return a 2D grid of (r, g, b) tuples. Dark pixels (luma <= LIT_THRESHOLD)
    are stored as (0, 0, 0)."""
    px = img.load()
    grid = []
    for y in range(SCREEN_H):
        row = []
        for x in range(SCREEN_W):
            r, g, b = px[x, y][:3]
            row.append((r, g, b) if (r + g + b) > LIT_THRESHOLD else (0, 0, 0))
        grid.append(row)
    return grid


def lit_from_colors(color_grid):
    """Derive a 2D bool lit-grid from the color grid."""
    return [[sum(color_grid[y][x]) > 0 for x in range(SCREEN_W)]
            for y in range(SCREEN_H)]


def classify_color(r, g, b):
    """Classify an average RGB triplet into a named color category."""
    if r + g + b == 0:
        return 'BLACK'
    if r > 200 and g > 200 and b > 200:
        return 'WHITE'
    if b > r and b > g:
        return 'BLUE'
    if r > g and r > b and g > 150:
        return 'YELLOW'
    if r > g and r > b:
        return 'RED'
    if g > r and g > b:
        return 'GREEN'
    return 'MIX'


def avg_color(color_grid, y0, y1, x0=0, x1=SCREEN_W):
    """Average RGB of all lit pixels in rows [y0..y1], columns [x0..x1).
    Returns (r, g, b, n) where n is the lit pixel count."""
    rs = gs = bs = n = 0
    for y in range(y0, y1 + 1):
        for x in range(x0, x1):
            r, g, b = color_grid[y][x]
            if r + g + b > 0:
                rs += r; gs += g; bs += b; n += 1
    if n == 0:
        return 0, 0, 0, 0
    return rs // n, gs // n, bs // n, n


def band_color(color_grid, y0, y1):
    r, g, b, _ = avg_color(color_grid, y0, y1)
    return classify_color(r, g, b)


def find_text_bands(lit):
    """Group consecutive rows that contain any lit pixels into bands.
    Returns list of (y_start, y_end_inclusive)."""
    rows_lit = [any(row) for row in lit]
    bands, i = [], 0
    while i < SCREEN_H:
        if rows_lit[i]:
            j = i
            while j < SCREEN_H and rows_lit[j]:
                j += 1
            bands.append((i, j - 1))
            i = j
        else:
            i += 1
    return bands


def pixel_map_row(color_grid, lit, y):
    """Return a 64-char string: color initial for lit pixels, '.' for dark."""
    COLOR_CHAR = {
        'WHITE': 'W', 'BLUE': 'B', 'RED': 'R',
        'GREEN': 'G', 'YELLOW': 'Y', 'MIX': 'M', 'BLACK': '.',
    }
    chars = []
    for x in range(SCREEN_W):
        if lit[y][x]:
            r, g, b = color_grid[y][x]
            chars.append(COLOR_CHAR[classify_color(r, g, b)])
        else:
            chars.append('.')
    return ''.join(chars)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("png", help="screenshot PNG path")
    ap.add_argument("--expect-color", nargs=2, metavar=("COLOR", "MIN_BANDS"),
                    help="assert that at least MIN_BANDS bands have COLOR as "
                         "dominant color (e.g. --expect-color BLUE 2)")
    ap.add_argument("--row", type=int, metavar="Y",
                    help="dump per-pixel RGB + color for a single logical row")
    ap.add_argument("--pixel-map", action="store_true",
                    help="show per-pixel color initial map for every band row "
                         "(W=white B=blue R=red G=green Y=yellow M=mix .=dark)")
    args = ap.parse_args()

    img        = downscale(Image.open(args.png).convert("RGB"))
    color_grid = color_pixels(img)
    lit        = lit_from_colors(color_grid)

    print(f"image:  {args.png}  -> downscaled to {img.size}")
    print()

    # Single-row debug dump
    if args.row is not None:
        y = args.row
        print(f"=== per-pixel dump for logical row {y} ===")
        for x in range(SCREEN_W):
            r, g, b = color_grid[y][x]
            label = classify_color(r, g, b)
            if r + g + b > 0:
                print(f"  x={x:2d}  ({r:3d},{g:3d},{b:3d})  {label}")
        return

    bands = find_text_bands(lit)

    print("=== band color analysis ===")
    band_results = []
    for y0, y1 in bands:
        r, g, b, n = avg_color(color_grid, y0, y1)
        label = classify_color(r, g, b)
        band_results.append((y0, y1, label, r, g, b, n))
        print(f"  y={y0:2d}..{y1:2d}  {label:6s}  avg_rgb=({r:3d},{g:3d},{b:3d})  lit_px={n}")

        # Per-row breakdown
        for y in range(y0, y1 + 1):
            rr, rg, rb, rn = avg_color(color_grid, y, y)
            row_label = classify_color(rr, rg, rb)
            pct_lit = rn / SCREEN_W * 100
            map_str = ("  " + pixel_map_row(color_grid, lit, y)) if args.pixel_map else ""
            print(f"    row {y:2d}: {row_label:6s}  avg=({rr:3d},{rg:3d},{rb:3d})  "
                  f"lit={rn:2d}/{SCREEN_W} ({pct_lit:4.1f}%){map_str}")
    print()

    # Color assertion
    if args.expect_color:
        expected_color = args.expect_color[0].upper()
        min_bands      = int(args.expect_color[1])
        if expected_color not in KNOWN_COLORS:
            sys.exit(f"error: unknown color '{expected_color}', choose from {KNOWN_COLORS}")
        matching = [(y0, y1) for y0, y1, label, *_ in band_results if label == expected_color]
        ok = len(matching) >= min_bands
        status = "PASS" if ok else "FAIL"
        print(f"=== color assertion: expect >= {min_bands} {expected_color} band(s) ===")
        print(f"  {status}  found {len(matching)} {expected_color} band(s) at: "
              f"{[(y0, y1) for y0, y1 in matching]}")
        if not ok:
            sys.exit(1)


if __name__ == "__main__":
    main()
