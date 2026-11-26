#!/usr/bin/env python3
"""Render timestamped PTY output into MP4/WebM terminal video."""
import argparse, base64, json, os, pathlib, subprocess, tempfile
from PIL import Image, ImageDraw, ImageFont

COLS, ROWS = 120, 32
CELL_W, CELL_H = 10, 20
FPS = 10
DEFAULT_FG = (220, 225, 235)
DEFAULT_BG = (16, 18, 24)

def xterm_color(index):
    if index < 16:
        base = [(0,0,0),(128,0,0),(0,128,0),(128,128,0),(0,0,128),(128,0,128),(0,128,128),(192,192,192),
                (128,128,128),(255,0,0),(0,255,0),(255,255,0),(0,0,255),(255,0,255),(0,255,255),(255,255,255)]
        return base[index]
    if index < 232:
        index -= 16
        levels = [0, 95, 135, 175, 215, 255]
        return (levels[(index // 36) % 6], levels[(index // 6) % 6], levels[index % 6])
    return (8 + (index - 232) * 10,) * 3

def apply_sgr(params, fg, bg):
    values = [int(p) if p else 0 for p in params.split(";")] if params else [0]
    i = 0
    while i < len(values):
        value = values[i]
        if value == 0:
            fg, bg = None, None
        elif value == 1:
            pass
        elif value == 39:
            fg = None
        elif value == 49:
            bg = None
        elif 30 <= value <= 37:
            fg = xterm_color(value - 30)
        elif 90 <= value <= 97:
            fg = xterm_color(value - 90 + 8)
        elif 40 <= value <= 47:
            bg = xterm_color(value - 40)
        elif 100 <= value <= 107:
            bg = xterm_color(value - 100 + 8)
        elif value in (38, 48) and i + 1 < len(values):
            target = "fg" if value == 38 else "bg"
            if values[i + 1] == 5 and i + 2 < len(values):
                color = xterm_color(values[i + 2]); i += 2
            elif values[i + 1] == 2 and i + 4 < len(values):
                color = (values[i + 2], values[i + 3], values[i + 4]); i += 4
            else:
                color = None
            if target == "fg": fg = color
            else: bg = color
        i += 1
    return fg, bg

def blank():
    return [[(" ", None, None) for _ in range(COLS)] for _ in range(ROWS)]

def apply_chunk(screen, row, col, raw, fg=None, bg=None):
    text = raw.decode("utf-8", "replace")
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == "\x1b":
            if i + 1 < len(text) and text[i + 1] == "[":
                j = i + 2
                while j < len(text) and not ("@" <= text[j] <= "~"):
                    j += 1
                if j >= len(text):
                    break
                final = text[j]
                body = text[i + 2:j].lstrip("?")
                params, _, command = body.partition(" ")
                parts = [p for p in params.split(";") if p.isdigit()]
                nums = [int(p) for p in parts] if parts else []
                if final in ("H", "f"):
                    row = max(0, min(ROWS - 1, (nums[0] if nums else 1) - 1))
                    col = max(0, min(COLS - 1, (nums[1] if len(nums) > 1 else 1) - 1))
                elif final == "A":
                    row = max(0, row - (nums[0] if nums else 1))
                elif final == "B":
                    row = min(ROWS - 1, row + (nums[0] if nums else 1))
                elif final == "C":
                    col = min(COLS - 1, col + (nums[0] if nums else 1))
                elif final == "D":
                    col = max(0, col - (nums[0] if nums else 1))
                elif final == "J" and (not nums or nums[0] == 2):
                    screen[:] = blank()
                    row = col = 0
                elif final == "K":
                    screen[row][col:] = [(" ", None, None)] * (COLS - col)
                elif final == "m":
                    fg, bg = apply_sgr(params, fg, bg)
                i = j + 1
                continue
            i += 2
            continue
        if ch == "\r":
            col = 0
        elif ch == "\n":
            row += 1
            col = 0
            if row >= ROWS:
                screen.pop(0)
                screen.append([(" ", None, None)] * COLS)
                row = ROWS - 1
        elif ch == "\b":
            col = max(0, col - 1)
        elif ch >= " ":
            if col >= COLS:
                col = 0
                row += 1
                if row >= ROWS:
                    screen.pop(0)
                    screen.append([(" ", None, None)] * COLS)
                    row = ROWS - 1
            screen[row][col] = (ch, fg, bg)
            col += 1
        i += 1
    return row, col, fg, bg

def render(screen, font, path):
    image = Image.new("RGB", (COLS * CELL_W, ROWS * CELL_H), DEFAULT_BG)
    draw = ImageDraw.Draw(image)
    for y, line in enumerate(screen):
        for x, (ch, fg, bg) in enumerate(line):
            left, top = x * CELL_W, y * CELL_H
            if bg is not None:
                draw.rectangle([left, top, left + CELL_W - 1, top + CELL_H - 1], fill=bg)
            if ch != " ":
                draw.text((left + 3, top + 3), ch, font=font, fill=fg or DEFAULT_FG)
    image.save(path)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--chunks", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--ffmpeg", default=os.environ.get("FFMPEG", "ffmpeg"))
    parser.add_argument("--frame-out")
    args = parser.parse_args()
    chunks = [json.loads(line) for line in args.chunks.read_text().splitlines() if line.strip()]
    if not chunks:
        raise SystemExit("no terminal chunks")

    first_content = None
    probe_screen, probe_row, probe_col, probe_fg, probe_bg = blank(), 0, 0, None, None
    for chunk in chunks:
        probe_row, probe_col, probe_fg, probe_bg = apply_chunk(probe_screen, probe_row, probe_col, base64.b64decode(chunk["data"]), probe_fg, probe_bg)
        if any("Service Lasso" in "".join(cell[0] for cell in line) for line in probe_screen):
            first_content = chunk["t"]
            break
    if first_content is None:
        first_content = 0.0
    start = max(0.0, first_content - 0.35)
    end = chunks[-1]["t"] + 2.0
    duration = max(0.2, end - start)
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 13)
    if args.frame_out:
        screen, row, col, fg, bg = blank(), 0, 0, None, None
        for chunk in chunks:
            row, col, fg, bg = apply_chunk(screen, row, col, base64.b64decode(chunk["data"]), fg, bg)
        render(screen, font, args.frame_out)
        print(json.dumps({"frame": args.frame_out}))
        return
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    stem = output / "tui-session"
    with tempfile.TemporaryDirectory(dir=output) as frames:
        count = 0
        for index in range(int(duration * FPS) + 1):
            when = start + index / FPS
            screen, row, col, fg, bg = blank(), 0, 0, None, None
            for chunk in chunks:
                if chunk["t"] > when:
                    break
                row, col, fg, bg = apply_chunk(screen, row, col, base64.b64decode(chunk["data"]), fg, bg)
            render(screen, font, pathlib.Path(frames) / f"frame-{index:05d}.png")
            count = index + 1
        subprocess.run([args.ffmpeg, "-y", "-loglevel", "error", "-framerate", str(FPS), "-i", f"{frames}/frame-%05d.png", "-c:v", "libx264", "-pix_fmt", "yuv420p", f"{stem}.mp4"], check=True)
        subprocess.run([args.ffmpeg, "-y", "-loglevel", "error", "-framerate", str(FPS), "-i", f"{frames}/frame-%05d.png", "-c:v", "libvpx-vp9", "-b:v", "0", "-crf", "32", f"{stem}.webm"], check=True)
    print(json.dumps({"frames": count, "start": start, "duration": duration, "mp4": f"{stem}.mp4", "webm": f"{stem}.webm"}))

if __name__ == "__main__":
    main()
