Rebuild the cube_launcher project, screenshot the running window, then run pixel-level analysis against the captured PNG. Two scripts are available — pick the one that fits the verification goal:

| Script | Purpose |
|---|---|
| `ocr_screenshot.py` | Verify **which text strings** are rendered — matches glyph shapes via the font bitmap |
| `ocr_screenshot_color.py` | Verify **what colors** are rendered where — classifies bands and rows by dominant color |

The skill arguments (`$ARGUMENTS`) are a space-separated list of strings or color assertions. Optionally prefixed with env-var assignments (e.g. `CUBE_APP_PATH=/path` SETTINGS Snake) which are forwarded to the launcher process.

Spawn a Sonnet subagent with these exact instructions — do NOT do this work yourself:

```
You are verifying a UI change in the cube_launcher LVGL project at the
current project root. Run these steps in order.

ARGUMENTS passed to this skill: $ARGUMENTS
Treat any leading FOO=BAR tokens as env vars to prepend to the launcher
command. Treat the rest as verification arguments (see script selection below).

1. Rebuild:
   cmake --build build -j 2>&1 | tail -5
   If it fails, report the error lines and stop.

2. Launch the binary in the background WITH any env vars from $ARGUMENTS:
   <ENV_VARS> ./build/apps/desktop/cube_launcher &
   Capture the PID. Sleep 4 seconds (longer than basic skill — the OCR
   is sensitive to stale-frame captures and longer settle helps).

3. Get the window ID:
   swift .claude/skills/verify-with-screenshot-macos/get_window_id.swift

4. Capture the window:
   TS=$(date +%Y%m%d_%H%M%S)
   SHOT="/tmp/cube_ocr_${TS}.png"
   screencapture -x -o -l <WID> "$SHOT"

   Verify the capture is not a stale black frame: check the PNG file size
   (under 4 KB usually means a black/blank capture). If it looks blank,
   sleep 2 more seconds and retry the capture once before proceeding.

5. Kill the launcher:
   kill <PID> 2>/dev/null; wait 2>/dev/null

6. Choose and run the right script:

   ── TEXT MATCHING (default) ──────────────────────────────────────────
   Use ocr_screenshot.py when the goal is to confirm specific strings are
   rendered. Works best when text appears on a plain black background.
   Fails on highlighted/colored-background rows because the background
   luma is treated as lit, drowning out glyph shapes.

   python3 .claude/skills/verify-with-pixel-ocr-macos/ocr_screenshot.py \
     "$SHOT" --expect <CANDIDATE_STRINGS>

   The script auto-locates shared/src/lv_font_cube_5px.c, downscales the
   PNG to 64x64, finds text bands by scanning consecutive lit rows, then
   slides each candidate's rendered bitmap across the image to find the
   best-matching y position. A score of >=95% counts as a hit.

   ── COLOR ANALYSIS ───────────────────────────────────────────────────
   Use ocr_screenshot_color.py when the goal is to verify highlight
   colors, focus indicators, selection states, or any rendering where
   the question is about color rather than text content.
   Color names: WHITE, BLUE, RED, GREEN, YELLOW, MIX, BLACK.

   # Classify all bands and print per-row breakdown:
   python3 .claude/skills/verify-with-pixel-ocr-macos/ocr_screenshot_color.py \
     "$SHOT"

   # With pixel-level color map per row:
   python3 .claude/skills/verify-with-pixel-ocr-macos/ocr_screenshot_color.py \
     "$SHOT" --pixel-map

   # Assert that at least N bands have a given dominant color:
   python3 .claude/skills/verify-with-pixel-ocr-macos/ocr_screenshot_color.py \
     "$SHOT" --expect-color BLUE 2

   # Inspect a single logical row in detail:
   python3 .claude/skills/verify-with-pixel-ocr-macos/ocr_screenshot_color.py \
     "$SHOT" --row <Y>

   You can run both scripts on the same screenshot when needed.

   Pillow is required: `pip3 install Pillow` if either script errors out.

7. Read the screenshot via the Read tool so the human reviewer also sees it.

8. Report:
   - build result (pass/fail)
   - screenshot path and file size
   - which script(s) were run and why
   - full script output
   - a one-sentence verdict per candidate or color assertion (HIT/MISS or PASS/FAIL)
   - any anomalies (borderline scores 90-94%, unexpected color classifications,
     bands where color and text results conflict)
```

After the subagent returns, surface the screenshot and the script output to the user. For text OCR: a score >=95% is a confirmed HIT. For color: a PASS means the expected number of bands with that color were found. If the screenshot looks correct but text OCR misses, the most likely cause is the font generator changed `unicode_list_0`/`glyph_bitmap` layout — re-read `shared/src/lv_font_cube_5px.c` and confirm the parser regex still matches.
