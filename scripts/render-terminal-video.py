#!/usr/bin/env python3
"""Render timestamped PTY output into MP4/WebM terminal video."""
import argparse, base64, json, os, pathlib, subprocess, tempfile
from PIL import Image, ImageDraw, ImageFont

COLS, ROWS = 120, 32
CELL_W, CELL_H = 9, 18
FPS = 10

def blank():
    return [[" "] * COLS for _ in range(ROWS)]

def apply_chunk(screen, row, col, raw):
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
                parts = [p for p in body.split(";") if p.isdigit()]
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
                    screen[row][col:] = [" "] * (COLS - col)
                i = j + 1
                continue
            i += 2
            continue
        if ch == "\r":
            col = 0
        elif ch == "\n":
            row += 1
            if row >= ROWS:
                screen.pop(0)
                screen.append([" "] * COLS)
                row = ROWS - 1
        elif ch == "\b":
            col = max(0, col - 1)
        elif ch >= " ":
            if col >= COLS:
                col = 0
                row += 1
                if row >= ROWS:
                    screen.pop(0)
                    screen.append([" "] * COLS)
                    row = ROWS - 1
            screen[row][col] = ch
            col += 1
        i += 1
    return row, col

def render(screen, font, path):
    image = Image.new("RGB", (COLS * CELL_W, ROWS * CELL_H), (16, 18, 24))
    draw = ImageDraw.Draw(image)
    for y, line in enumerate(screen):
        draw.text((4, y * CELL_H + 1), "".join(line).rstrip(), font=font, fill=(220, 225, 235))
    image.save(path)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--chunks", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--ffmpeg", default=os.environ.get("FFMPEG", "ffmpeg"))
    args = parser.parse_args()
    chunks = [json.loads(line) for line in args.chunks.read_text().splitlines() if line.strip()]
    if not chunks:
        raise SystemExit("no terminal chunks")
    duration = chunks[-1]["t"] + 1.0
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 14)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    stem = output / "tui-session"
    with tempfile.TemporaryDirectory(dir=output) as frames:
        count = 0
        for index in range(int(duration * FPS) + 1):
            when = index / FPS
            screen, row, col = blank(), 0, 0
            for chunk in chunks:
                if chunk["t"] > when:
                    break
                row, col = apply_chunk(screen, row, col, base64.b64decode(chunk["data"]))
            render(screen, font, pathlib.Path(frames) / f"frame-{index:05d}.png")
            count = index + 1
        subprocess.run([args.ffmpeg, "-y", "-loglevel", "error", "-framerate", str(FPS), "-i", f"{frames}/frame-%05d.png", "-c:v", "libx264", "-pix_fmt", "yuv420p", f"{stem}.mp4"], check=True)
        subprocess.run([args.ffmpeg, "-y", "-loglevel", "error", "-framerate", str(FPS), "-i", f"{frames}/frame-%05d.png", "-c:v", "libvpx-vp9", "-b:v", "0", "-crf", "32", f"{stem}.webm"], check=True)
    print(json.dumps({"frames": count, "duration": duration, "mp4": f"{stem}.mp4", "webm": f"{stem}.webm"}))

if __name__ == "__main__":
    main()
