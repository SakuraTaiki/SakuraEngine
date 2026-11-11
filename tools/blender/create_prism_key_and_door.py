"""Create hand-authored prism key and locked door assets."""
from pathlib import Path
import bpy
import math

ROOT = Path(__file__).resolve().parents[2]
KEY_OUT = ROOT / "project" / "Resources" / "Gimmick" / "PrismKey"
DOOR_OUT = ROOT / "project" / "Resources" / "Gimmick" / "PrismLockedDoor"
KEY_OUT.mkdir(parents=True, exist_ok=True)
DOOR_OUT.mkdir(parents=True, exist_ok=True)

COLORS = [
    (0.035, 0.075, 0.14, 1.0),   # dark navy metal
    (0.08, 0.24, 0.34, 1.0),    # blue frame
    (0.10, 0.72, 0.86, 1.0),    # cyan prism line
    (0.98, 0.60, 0.08, 1.0),    # key gold
    (1.00, 0.88, 0.30, 1.0),    # gold highlight
    (0.34, 0.12, 0.12, 1.0),    # muted lock red
]

def clear():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)

def make_texture(path, name):
    width, height = 384, 64
    image = bpy.data.images.new(name, width=width, height=height, alpha=True)
    band = width // len(COLORS)
    pixels = [0.0] * (width * height * 4)
    for y in range(height):
        for x in range(width):
            color = list(COLORS[min(x // band, len(COLORS) - 1)])
            local = x % band
            edge = min(local, band - 1 - local, y, height - 1 - y)
            if edge < 3:
                color[0] *= 0.58; color[1] *= 0.58; color[2] *= 0.58
            elif y > 44:
                color[0] *= 0.82; color[1] *= 0.82; color[2] *= 0.82
            p = (y * width + x) * 4
            pixels[p:p + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(path)
    image.file_format = "PNG"
    image.save()
    return image

def make_material(name, image):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    for node in list(nodes): nodes.remove(node)
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    shader.inputs["Roughness"].default_value = 0.55
    shader.inputs["Metallic"].default_value = 0.34
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material

def set_uv(obj, palette):
    layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    u = (palette + 0.5) / len(COLORS)
    for loop in layer.data:
        loop.uv = (u, 0.5)

def box(name, location, size, palette, material, bevel=0.025):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = tuple(value * 0.5 for value in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    modifier = obj.modifiers.new("Single bevel", "BEVEL")
    modifier.width = bevel
    modifier.segments = 1
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.data.materials.append(material)
    set_uv(obj, palette)
    return obj

def cylinder(name, location, radius, depth, palette, material, vertices=8, rotation=(math.pi/2, 0, 0)):
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices, radius=radius, depth=depth,
        location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(material)
    set_uv(obj, palette)
    bevel = obj.modifiers.new("Edge bevel", "BEVEL")
    bevel.width = 0.018
    bevel.segments = 1
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    return obj

def export(parts, out_dir, stem, object_name, rotate_z=0.0):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts: part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    bpy.context.object.name = object_name
    if rotate_z != 0.0:
        # Rotate the completed silhouette around its shared origin. In this project,
        # +90 degrees maps the former right-facing stem to game-visible down.
        bpy.context.object.rotation_euler.z = rotate_z
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    bpy.ops.object.shade_smooth_by_angle()
    bpy.ops.wm.obj_export(
        filepath=str(out_dir / f"{stem}.obj"),
        export_selected_objects=True, export_materials=True,
        export_triangulated_mesh=True, forward_axis="NEGATIVE_Y", up_axis="Z")

def create_key():
    clear()
    texture = make_texture(KEY_OUT / "prism_key.png", "PrismKeyPalette")
    material = make_material("PrismKeyMaterial", texture)
    parts = []
    # Eight-sided ring keeps the silhouette deliberately game-like rather than ornamental.
    bpy.ops.mesh.primitive_torus_add(
        major_segments=8, minor_segments=4,
        location=(-0.38, 0.0, 0.0),
        major_radius=0.28, minor_radius=0.085)
    ring = bpy.context.object
    ring.name = "KeyRing"
    ring.data.materials.append(material)
    set_uv(ring, 3)
    parts.append(ring)
    parts.append(box("KeyStem", (0.20, 0.0, 0.0), (0.82, 0.16, 0.16), 3, material, 0.02))
    parts.append(box("KeyToothOuter", (0.52, 0.17, 0.0), (0.18, 0.34, 0.16), 3, material, 0.018))
    parts.append(box("KeyToothInner", (0.25, 0.13, 0.0), (0.15, 0.26, 0.16), 4, material, 0.015))
    parts.append(box("KeyPrism", (-0.38, 0.0, -0.075), (0.16, 0.16, 0.07), 2, material, 0.018))
    export(parts, KEY_OUT, "prism_key", "PrismKey", rotate_z=math.pi * 0.5)
    bpy.ops.wm.save_as_mainfile(filepath=str(KEY_OUT / "prism_key.blend"))

def create_door():
    clear()
    texture = make_texture(DOOR_OUT / "prism_locked_door.png", "PrismLockedDoorPalette")
    material = make_material("PrismLockedDoorMaterial", texture)
    parts = [
        box("DoorPanel", (0.0, 0.0, 0.0), (0.76, 2.72, 0.22), 0, material, 0.035),
        box("LeftFrame", (-0.47, 0.0, 0.0), (0.18, 2.96, 0.32), 1, material, 0.028),
        box("RightFrame", (0.47, 0.0, 0.0), (0.18, 2.96, 0.32), 1, material, 0.028),
        box("TopFrame", (0.0, -1.39, 0.0), (0.80, 0.18, 0.32), 1, material, 0.028),
        box("BottomFrame", (0.0, 1.39, 0.0), (0.80, 0.18, 0.32), 1, material, 0.028),
        box("CenterSeam", (0.0, 0.0, -0.13), (0.055, 2.40, 0.055), 2, material, 0.010),
        box("LockBar", (0.0, 0.0, -0.18), (0.72, 0.17, 0.11), 3, material, 0.020),
        cylinder("LockHousing", (0.0, 0.0, -0.25), 0.22, 0.12, 5, material, 8),
        box("KeySlot", (0.0, -0.01, -0.325), (0.065, 0.22, 0.025), 4, material, 0.008),
    ]
    # Corner cyan strips distinguish it from the circular beacon goal.
    for x in (-0.36, 0.36):
        parts.append(box("PrismStrip", (x, 0.0, -0.18), (0.045, 2.35, 0.045), 2, material, 0.008))
    export(parts, DOOR_OUT, "prism_locked_door", "PrismLockedDoor")
    bpy.ops.wm.save_as_mainfile(filepath=str(DOOR_OUT / "prism_locked_door.blend"))

create_key()
create_door()
print(f"PRISM_KEY_EXPORTED:{KEY_OUT}")
print(f"PRISM_LOCKED_DOOR_EXPORTED:{DOOR_OUT}")
