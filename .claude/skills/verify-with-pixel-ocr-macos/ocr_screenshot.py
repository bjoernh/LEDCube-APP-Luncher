#!/usr/bin/env python3
"""
Pixel-OCR for cube_launcher screenshots.

The launcher renders a 64x64 framebuffer at SDL 4x scale (256x256 PNG).
This tool downscales it to 64x64 and reconstructs rendered text by
reverse-mapping pixels through the project's lv_font_cube_5px font.

Given a list of candidate strings, reports the y-offset and match score
of each candidate against every 5-row band in the image. A 100% score
means the candidate's bitmap matches the captured pixels exactly.

Usage:
    python3 ocr_screenshot.py <screenshot.png> [--expect STR1 STR2 ...]
    python3 ocr_screenshot.py <screenshot.png>            # dump all bands
"""

import argparse
import re
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("error: pip install Pillow")

SCREEN_W = 64
SCREEN_H = 64
GLYPH_W  = 5
GLYPH_H  = 5
ADVANCE  = 6   # 96/16 px per cube_5px char


def parse_font(font_c_path):
    """Extract glyph_bitmap[] bytes, unicode_list, and the cmap range_start
    from an LVGL-generated font .c file. Returns (bitmap, unicode_list,
    range_start)."""
    text = font_c_path.read_text()

    m = re.search(r"glyph_bitmap\[\]\s*=\s*\{([^}]+)\}", text)
    if not m:
        sys.exit(f"error: glyph_bitmap[] not found in {font_c_path}")
    bitmap = [int(n, 16) for n in re.findall(r"0x[0-9A-Fa-f]+", m.group(1))]

    m = re.search(r"unicode_list_0\[\]\s*=\s*\{([^}]+)\}", text)
    if not m:
        sys.exit(f"error: unicode_list_0[] not found in {font_c_path}")
    unicode_list = [int(n, 16) for n in re.findall(r"0x[0-9A-Fa-f]+", m.group(1))]

    m = re.search(r"\.range_start\s*=\s*0x([0-9A-Fa-f]+)", text)
    range_start = int(m.group(1), 16) if m else 0x0020

    return bitmap, unicode_list, range_start


def char_to_glyph_idx(ch, unicode_list, range_start):
    """Map character to slot index in unicode_list (0-based), or -1 if missing.
    Space (the first char of the cmap range) returns 0 — slot exists but has
    no bitmap; we render it as a 5x5 blank grid."""
    cp = ord(ch) - range_start
    try:
        return unicode_list.index(cp)
    except ValueError:
        return -1


def render_glyph(bitmap, slot_idx):
    """Return a 5x5 bool grid for the slot. Slot 0 (space) is blank — its
    glyph descriptor in LVGL has box_w=0 so no bitmap bytes are stored."""
    if slot_idx <= 0:
        return [[False]*GLYPH_W for _ in range(GLYPH_H)]
    # Slot 0 is space (no bytes), slot 1 starts at bitmap[0]; each subsequent
    # slot consumes 4 bytes (5x5 = 25 bits, padded to 32).
    start = (slot_idx - 1) * 4
    bits = "".join(format(b, "08b") for b in bitmap[start:start+4])
    return [[bits[r*GLYPH_W + c] == "1" for c in range(GLYPH_W)]
            for r in range(GLYPH_H)]


def render_text(text, bitmap, unicode_list, range_start, width=SCREEN_W):
    """Render `text` centered horizontally on a `width`-px row. Returns a
    list of GLYPH_H bool rows."""
    total_w = len(text) * ADVANCE
    start_x = (width - total_w) // 2
    grid = [[False]*width for _ in range(GLYPH_H)]
    x = start_x
    for ch in text:
        glyph = render_glyph(bitmap,
                             char_to_glyph_idx(ch, unicode_list, range_start))
        for r in range(GLYPH_H):
            for c in range(GLYPH_W):
                px = x + c
                if 0 <= px < width and glyph[r][c]:
                    grid[r][px] = True
        x += ADVANCE
    return grid, max(0, start_x), min(width, start_x + total_w)


def downscale(img):
    """Crop off the macOS window titlebar (screencapture -l includes it) and
    downscale the SDL window's square content area to the 64x64 logical
    framebuffer. Uses nearest-neighbour to preserve crisp pixel-art edges.
    Assumption: the SDL window is always square (kScale * 64 px each side),
    so the content is the bottom `width × width` region of the capture."""
    if img.size == (SCREEN_W, SCREEN_H):
        return img
    w, h = img.size
    if h > w:
        img = img.crop((0, h - w, w, h))  # bottom square
    return img.resize((SCREEN_W, SCREEN_H), Image.NEAREST)


def lit_pixels(img):
    """Return a 2D bool grid (SCREEN_H x SCREEN_W). A pixel counts as 'lit'
    when its luma is clearly above the bg-vs-fg threshold."""
    px = img.load()
    grid = []
    for y in range(SCREEN_H):
        row = []
        for x in range(SCREEN_W):
            r, g, b = px[x, y][:3]
            row.append((r + g + b) > 60)
        grid.append(row)
    return grid


def score_band(rendered_grid, x0, x1, lit, y0):
    """Score a rendered candidate against a 5-row pixel band starting at y0.
    Compares only the columns the candidate's glyphs occupy [x0, x1) to
    avoid penalising blank padding."""
    if y0 < 0 or y0 + GLYPH_H > SCREEN_H:
        return 0.0
    matched = 0
    total   = 0
    for r in range(GLYPH_H):
        for c in range(x0, x1):
            if rendered_grid[r][c] == lit[y0 + r][c]:
                matched += 1
            total += 1
    return matched / total if total else 0.0


def best_y_for(candidate, bitmap, unicode_list, range_start, lit):
    """Slide the candidate's rendered bitmap across all valid y positions
    and return (best_y, best_score)."""
    rendered, x0, x1 = render_text(candidate, bitmap, unicode_list, range_start)
    if x1 <= x0:
        return (-1, 0.0)
    best_y, best_score = -1, 0.0
    for y in range(SCREEN_H - GLYPH_H + 1):
        s = score_band(rendered, x0, x1, lit, y)
        if s > best_score:
            best_y, best_score = y, s
    return best_y, best_score


def dump_band(lit, y0):
    """Render a 5-row band as ASCII art for visual inspection."""
    return ["".join("X" if lit[y0+r][c] else "." for c in range(SCREEN_W))
            for r in range(GLYPH_H)]


def find_text_bands(lit):
    """Group consecutive y rows that contain any lit pixels into bands.
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


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("png", help="screenshot PNG path")
    ap.add_argument("--expect", nargs="+", default=[],
                    help="candidate strings to OCR-match against the image")
    ap.add_argument("--font",
                    help="path to lv_font_cube_5px.c (default: auto-locate)")
    ap.add_argument("--threshold", type=float, default=0.95,
                    help="min match score to report as a hit (default 0.95)")
    args = ap.parse_args()

    if args.font:
        font_path = Path(args.font)
    else:
        # Default: walk up from this script to find shared/src/lv_font_cube_5px.c
        here = Path(__file__).resolve()
        for ancestor in here.parents:
            cand = ancestor / "shared/src/lv_font_cube_5px.c"
            if cand.exists():
                font_path = cand
                break
        else:
            sys.exit("error: could not auto-locate lv_font_cube_5px.c — pass --font")

    bitmap, unicode_list, range_start = parse_font(font_path)
    img = downscale(Image.open(args.png).convert("RGB"))
    lit = lit_pixels(img)

    print(f"image:       {args.png}  -> downscaled to {img.size}")
    print(f"font:        {font_path}  ({len(unicode_list)} glyphs)")
    print(f"threshold:   {args.threshold:.0%}")
    print()

    bands = find_text_bands(lit)
    print(f"text bands detected (consecutive lit-row groups): {bands}")
    print()

    if args.expect:
        print("=== candidate matches ===")
        rows = []
        for cand in args.expect:
            y, s = best_y_for(cand, bitmap, unicode_list, range_start, lit)
            hit  = "HIT" if s >= args.threshold else "   "
            rows.append((s, cand, y, hit))
        # Sort by score descending so hits float to the top.
        rows.sort(key=lambda r: -r[0])
        for s, cand, y, hit in rows:
            print(f"  {hit}  {s:5.1%}  y={y:>2}  '{cand}'")
        print()

    print("=== text bands (ASCII dump for human review) ===")
    for y0, y1 in bands:
        print(f"-- y={y0}..{y1} --")
        for r in range(y0, y1 + 1):
            print("  " + "".join("X" if lit[r][c] else "." for c in range(SCREEN_W)))


if __name__ == "__main__":
    main()
