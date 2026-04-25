#!/usr/bin/env python3
"""Generate an LVGL-compatible binding for the 5x5 mcufont glyphs.

Input:  third_party/maurycyz_mcufont/mcufont.h  (upstream pristine copy)
Output: shared/src/lv_font_cube_5px.c           (LVGL lv_font_t definition)

Extra glyphs not present upstream (e.g. '%') are defined in
EXTRA_GLYPHS below so the vendored header stays byte-for-byte identical
to the upstream copy.

Run from the repo root:
    python3 tools/gen_5px_font.py
"""

import re
import sys
import textwrap
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SRC = REPO / "third_party/maurycyz_mcufont/mcufont.h"
DST = REPO / "shared/src/lv_font_cube_5px.c"

# Glyphs designed locally (not present in upstream mcufont.h). Each entry is
# 5 rows of 5 bits (MSB = leftmost pixel), matching upstream's convention.
EXTRA_GLYPHS: dict[str, list[int]] = {
    # Classic percent sign: 2x2 dot top-left, diagonal slash, 2x2 dot bottom-right.
    "%": [
        0b11001,
        0b11010,
        0b00100,
        0b01011,
        0b10011,
    ],
}


def parse_arrays(text: str) -> dict[str, list[list[int]]]:
    """Return {'fnt_upper': [[row, ...], ...], ...} from the mcufont header."""
    arrays: dict[str, list[list[int]]] = {}
    pattern = re.compile(
        r"const\s+uint8_t\s+(fnt_\w+)\s*\[\s*\d+\s*\]\s*\[\s*5\s*\]"
        r"\s*PROGMEM\s*=\s*\{(.*?)\};",
        re.DOTALL,
    )
    for match in pattern.finditer(text):
        name, body = match.group(1), match.group(2)
        chars = []
        for char_body in re.finditer(r"\{([^{}]*)\}", body):
            rows = [int(v, 2) for v in re.findall(r"0b([01]+)", char_body.group(1))]
            if len(rows) != 5:
                raise RuntimeError(f"{name}: expected 5 rows, got {len(rows)}")
            chars.append(rows)
        arrays[name] = chars
    return arrays


def pack_glyph(rows: list[int]) -> bytes:
    """Pack 5 rows of 5 bits (MSB-first) into 4 bytes, bit-stream style."""
    bits = []
    for row in rows:
        for col in range(4, -1, -1):  # MSB first
            bits.append((row >> col) & 1)
    # 25 bits → 4 bytes (7 trailing padding bits = 0).
    while len(bits) % 8:
        bits.append(0)
    out = bytearray()
    for i in range(0, len(bits), 8):
        byte = 0
        for b in bits[i:i + 8]:
            byte = (byte << 1) | b
        out.append(byte)
    return bytes(out)


def build_glyphs(arrays: dict[str, list[list[int]]]) -> list[tuple[int, bytes]]:
    """Return a list of (codepoint, packed-bitmap) sorted by codepoint.

    A leading zero-size space glyph (U+0020) is injected so layouts can break
    words; space has no bitmap, only an advance width.
    """
    glyphs: list[tuple[int, bytes]] = [(0x20, b"")]  # space: empty bitmap

    for i, rows in enumerate(arrays["fnt_upper"]):
        glyphs.append((ord("A") + i, pack_glyph(rows)))
    for i, rows in enumerate(arrays["fnt_lower"]):
        glyphs.append((ord("a") + i, pack_glyph(rows)))
    for i, rows in enumerate(arrays["fnt_digits"]):
        glyphs.append((ord("0") + i, pack_glyph(rows)))

    # fnt_other order per mcufont.h: - + . , ? ! : = '
    symbol_chars = ["-", "+", ".", ",", "?", "!", ":", "=", "'"]
    for ch, rows in zip(symbol_chars, arrays["fnt_other"]):
        glyphs.append((ord(ch), pack_glyph(rows)))

    for ch, rows in EXTRA_GLYPHS.items():
        if len(ch) != 1:
            raise RuntimeError(f"EXTRA_GLYPHS key must be a single char, got {ch!r}")
        if len(rows) != 5 or any(r > 0b11111 for r in rows):
            raise RuntimeError(f"EXTRA_GLYPHS[{ch!r}]: expected 5 rows of 5 bits")
        glyphs.append((ord(ch), pack_glyph(rows)))

    glyphs.sort(key=lambda g: g[0])
    return glyphs


def render_bitmap(bitmap_bytes: bytes) -> str:
    lines = []
    for i in range(0, len(bitmap_bytes), 12):
        chunk = ", ".join(f"0x{b:02X}" for b in bitmap_bytes[i:i + 12])
        lines.append(f"    {chunk},")
    return "\n".join(lines)


def render_glyph_dsc(entries: list[tuple[int, int, int]]) -> str:
    """entries: [(bitmap_index, box_w, box_h), ...] for glyph IDs 0..N.

    Glyph ID 0 is the reserved 'not found' slot; ID 1..N map to real glyphs.
    adv_w is fixed at 6 px (96 = 6 << 4) for every glyph — monospace.
    """
    lines = ["    { .bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0 },"]
    for bm, bw, bh in entries:
        lines.append(
            f"    {{ .bitmap_index = {bm}, .adv_w = 96, "
            f".box_w = {bw}, .box_h = {bh}, .ofs_x = 0, .ofs_y = 0 }},"
        )
    return "\n".join(lines)


def render_unicode_list(codepoints: list[int], range_start: int) -> str:
    offsets = [f"0x{(cp - range_start):04X}" for cp in codepoints]
    wrapped = textwrap.fill(
        ", ".join(offsets), width=88, initial_indent="    ", subsequent_indent="    "
    )
    return wrapped


def main() -> int:
    text = SRC.read_text()
    arrays = parse_arrays(text)
    for required in ("fnt_upper", "fnt_lower", "fnt_digits", "fnt_other"):
        if required not in arrays:
            print(f"missing array {required}", file=sys.stderr)
            return 1

    glyphs = build_glyphs(arrays)

    bitmap = bytearray()
    dsc_entries: list[tuple[int, int, int]] = []
    for cp, bm in glyphs:
        bm_index = len(bitmap)
        if cp == 0x20:
            dsc_entries.append((bm_index, 0, 0))
        else:
            bitmap += bm
            dsc_entries.append((bm_index, 5, 5))

    codepoints = [cp for cp, _ in glyphs]
    range_start = codepoints[0]
    range_length = codepoints[-1] - range_start + 1
    list_length = len(codepoints)

    symbol_coverage = " ".join(
        chr(cp) for cp in codepoints
        if not (0x30 <= cp <= 0x39 or 0x41 <= cp <= 0x5A or 0x61 <= cp <= 0x7A)
        and cp != 0x20
    )

    body = f"""\
/* Generated by tools/gen_5px_font.py. Do not edit.
 * Source: third_party/maurycyz_mcufont/mcufont.h (by maurycyz,
 * https://maurycyz.com/projects/mcufont/ — derived from lcamtuf's
 * font-inline.h). Extra glyphs (e.g. '%') are defined in the generator.
 *
 * Monospace 5x5 pixel font, rendered on a 6x6 grid (advance = 6 px).
 * Coverage: space, A-Z, a-z, 0-9, and {symbol_coverage}
 */

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "../../lib/lvgl/lvgl.h"
#endif

/* --- Glyph bitmap (1bpp, packed bit-stream per glyph, MSB-first) --- */
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {{
{render_bitmap(bytes(bitmap))}
}};

/* --- Glyph descriptors (index 0 is reserved 'not found') --- */
static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {{
{render_glyph_dsc(dsc_entries)}
}};

/* --- Unicode list (offsets from range_start) for sparse cmap --- */
static const uint16_t unicode_list_0[] = {{
{render_unicode_list(codepoints, range_start)}
}};

/* --- Character maps: single sparse range --- */
static const lv_font_fmt_txt_cmap_t cmaps[] = {{
    {{
        .range_start = 0x{range_start:04X}, .range_length = {range_length}, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = {list_length},
        .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }}
}};

/* --- Font descriptor --- */
static const lv_font_fmt_txt_dsc_t font_dsc = {{
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,  /* LV_FONT_FMT_TXT_PLAIN */
}};

/* --- Public font --- */
const lv_font_t lv_font_cube_5px = {{
    .get_glyph_dsc    = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 6,
    .base_line = 0,
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = 0,
    .underline_thickness = 0,
    .dsc = &font_dsc,
    .fallback = NULL,
    .user_data = NULL,
}};
"""

    DST.write_text(body)
    print(f"wrote {DST.relative_to(REPO)} — {len(glyphs)} glyphs, "
          f"{len(bitmap)} bitmap bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
