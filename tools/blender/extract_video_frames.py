"""Extract evenly spaced PNG frames from a video using Blender's FFmpeg support."""

import sys
from pathlib import Path
import bpy

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
if len(args) < 2:
    raise RuntimeError("Usage: video_path output_directory [frame_count]")

video = Path(args[0]).resolve()
output = Path(args[1]).resolve()
count = int(args[2]) if len(args) > 2 else 8
output.mkdir(parents=True, exist_ok=True)

scene = bpy.context.scene
editor = scene.sequence_editor_create()
strip = editor.strips.new_movie("SourceVideo", str(video), channel=1, frame_start=1)
scene.render.resolution_x = min(strip.elements[0].orig_width, 1280)
scene.render.resolution_y = round(strip.elements[0].orig_height * scene.render.resolution_x / strip.elements[0].orig_width)
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = "PNG"
scene.render.film_transparent = False
scene.render.use_file_extension = True

duration = max(strip.frame_final_duration, 1)
for index in range(count):
    frame = 1 + round((duration - 1) * index / max(count - 1, 1))
    scene.frame_set(frame)
    scene.render.filepath = str(output / f"frame_{index + 1:02d}.png")
    bpy.ops.render.render(write_still=True, use_viewport=False)
print(f"VIDEO_FRAMES_EXTRACTED:{count}:{output}")
