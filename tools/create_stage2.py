"""Generate Stage2.json using the same schema as the in-game stage editor."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "project" / "Resources" / "Stages" / "Stage2.json"
WIDTH, HEIGHT = 72, 15
tiles = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]

def fill_row(y, start, end, tile=1):
    for x in range(start, end + 1):
        tiles[y][x] = tile

# Four ground islands with two short falling-floor bridges.
fill_row(0, 0, 12)
fill_row(0, 16, 31)
fill_row(0, 35, 71)

# Readable secondary routes and the elevated key platform.
fill_row(3, 7, 12)
fill_row(5, 24, 29)
fill_row(5, 39, 46)

# Goal chamber: solid roof and rear wall; its only entrance is the locked door.
fill_row(4, 62, 70, 2)
for y in range(1, 4):
    tiles[y][70] = 2

objects = []
next_id = 1

def add(item_id, position, scale, variant=0, move=(0.0, 3.0, 0.0)):
    global next_id
    # Development editor cursors always target tile centers (n + 0.5).
    # Reject off-grid authored objects so they remain selectable, rotatable, and removable.
    for axis_name, value in (("x", position[0]), ("y", position[1])):
        if abs((value - 0.5) - round(value - 0.5)) > 0.0001:
            raise ValueError(f"Object {item_id} {axis_name}={value} is not on the Development grid")
    objects.append({
        "id": next_id,
        "item_id": item_id,
        "position": list(position),
        "rotation": [0.0, 0.0, 0.0],
        "scale": list(scale),
        "variant": variant,
        "move_offset": list(move),
    })
    next_id += 1

# Start and route guidance.
add(25, (2.5, 1.5, 0.0), (0.5, 0.5, 0.5))
add(31, (18.5, 1.5, 0.0), (0.35, 1.2, 0.35))
add(31, (48.5, 1.5, 0.0), (0.35, 1.2, 0.35))

# Falling bridges preserve the established block gimmick language.
for x in (13.5, 14.5, 15.5, 32.5, 33.5, 34.5):
    add(28, (x, 0.5, 0.0), (1.0, 0.3, 1.0))

# Spring route leads to the elevated key platform.
add(29, (37.5, 1.5, 0.0), (0.9, 0.2, 1.0))
add(20, (43.5, 6.5, 0.0), (0.55, 0.55, 0.35))

# Both required enemy types: grounded Crash Cubes and airborne Prism Gliders.
add(22, (20.5, 1.5, 0.0), (0.7, 0.7, 0.7))
add(22, (53.5, 1.5, 0.0), (0.7, 0.7, 0.7))
add(23, (10.5, 5.5, 0.0), (0.62, 0.62, 0.62))
add(23, (42.5, 8.5, 0.0), (0.62, 0.62, 0.62))

# The door spans floor-to-ceiling, so the goal cannot be entered without the key.
add(9, (62.5, 2.5, 0.0), (1.0, 3.0, 0.45))
add(17, (67.5, 1.5, 0.0), (0.3, 2.0, 0.3))

stage = {
    "coordinate_system": "tilemap_xy",
    "name": "Stage2",
    "objects": objects,
    "size": [WIDTH, HEIGHT],
    "tile_size": 16,
    "tile_world_size": 1.0,
    "tiles": tiles,
    "version": 5,
}
OUTPUT.write_text(json.dumps(stage, ensure_ascii=False, indent=2), encoding="utf-8")
print(f"STAGE2_EXPORTED:{OUTPUT}")
