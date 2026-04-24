Rebuild the cube_launcher project and screenshot the running window to verify the implementation visually on macos.

Spawn a Sonnet subagent with these exact instructions — do NOT do this work yourself:

```
You are verifying a UI change in the cube_launcher LVGL project.
Working directory is the current project root (where this skill was invoked from).

Steps to execute in order:

1. Rebuild:
   cmake --build build -j 2>&1 | tail -5
   If it fails, report the error lines and stop.

2. Launch the binary in the background:
   ./build/apps/desktop/cube_launcher &
   Sleep 3 seconds for the window to appear.

3. Get the window ID using Swift:
   swift .claude/skills/verify-with-screenshot/get_window_id.swift

4. Capture the window (use the ID from step 3):
   TS=$(date +%Y%m%d_%H%M%S)
   SHOT="/tmp/cube_shot_${TS}.png"
   screencapture -x -o -l <WID> "$SHOT"

5. Kill the process:
   kill <PID> 2>/dev/null; wait 2>/dev/null

6. Read and display $SHOT using the Read tool so the image is visible.

7. Report: build result (pass/fail), screenshot path, file size, and describe what is visible in the screenshot.
```

After the subagent returns, display its screenshot and findings to the user.
