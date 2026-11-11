"""Build the side-facing Crash Cube walker and export CG2 runtime assets."""

from pathlib import Path

import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Enemy" / "CrashCube"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "crash_cube.png"
BLEND = OUT / "crash_cube.blend"
OBJ = OUT / "crash_cube.obj"

COLORS = [
    (0.075, 0.085, 0.12, 1.0),   # charcoal shell
    (0.16, 0.12, 0.24, 1.0),     # purple armour
    (0.025, 0.035, 0.065, 1.0),  # joints / screen
    (1.0, 0.30, 0.045, 1.0),     # orange eye
    (0.72, 0.075, 0.035, 1.0),   # red core
    (0.30, 0.18, 0.38, 1.0),     # worn edge
]


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.materials):
        for block in list(datablocks):
            if block.users == 0:
                datablocks.remove(block)


def make_texture():
    width, height = 384, 64
    image = bpy.data.images.new("CrashCubePalette", width=width, height=height, alpha=True)
    pixels = [0.0] * (width * height * 4)
    band = width // len(COLORS)
    for y in range(height):
        for x in range(width):
            index = min(x // band, len(COLORS) - 1)
            color = list(COLORS[index])
            # Subtle hand-painted variation, kept deterministic and restrained.
            edge = min(x % band, band - 1 - (x % band), y, height - 1 - y)
            if edge < 2:
                color[0] *= 0.78
                color[1] *= 0.78
                color[2] *= 0.78
            dst = (y * width + x) * 4
            pixels[dst:dst + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("CrashCubeMaterial")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    for node in list(nodes):
        nodes.remove(node)
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    shader.inputs["Roughness"].default_value = 0.68
    shader.inputs["Metallic"].default_value = 0.18
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def set_palette_uv(obj, palette_index):
    mesh = obj.data
    uv_layer = mesh.uv_layers.active or mesh.uv_layers.new(name="UVMap")
    u = (palette_index + 0.5) / len(COLORS)
    for loop in uv_layer.data:
        loop.uv = (u, 0.5)


def add_box(name, location, scale, palette, material, bevel=0.055, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = (scale[0] * 0.5, scale[1] * 0.5, scale[2] * 0.5)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    modifier = obj.modifiers.new("Small machined bevel", "BEVEL")
    modifier.width = bevel
    modifier.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.data.materials.append(material)
    set_palette_uv(obj, palette)
    return obj


def add_core(material):
    # Core sits visibly on both camera-facing sides so it remains readable after turning.
    parts = []
    for z in (-0.355, 0.355):
        bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=0.19, location=(-0.08, 0.22, z))
        core = bpy.context.object
        core.name = "CrackedPrismCore"
        core.scale = (1.0, 1.12, 0.42)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        core.data.materials.append(material)
        set_palette_uv(core, 4)
        parts.append(core)
    return parts


def build(material):
    objects = []
    # The runtime model faces +X. CG2's OBJ handedness conversion is accounted for by the exporter.
    objects.append(add_box("MainBody", (0.04, 0.05, 0), (0.98, 0.55, 0.66), 0, material, 0.09))
    objects.append(add_box("UpperArmour", (0.06, 0.31, 0), (0.72, 0.20, 0.58), 1, material, 0.06))
    objects.append(add_box("FrontRam", (0.54, 0.04, 0), (0.32, 0.38, 0.54), 1, material, 0.055,
                           rotation=(0, -0.26, 0.16)))
    objects.append(add_box("RearPack", (-0.52, 0.12, 0), (0.22, 0.37, 0.50), 5, material, 0.045))

    # A large side-profile visor makes the front unmistakable in a side-scroller.
    # It is duplicated on both Z sides so turning never hides the readable face.
    for z in (-0.345, 0.345):
        objects.append(add_box("FaceVisor", (0.32, 0.16, z), (0.42, 0.25, 0.035), 2, material, 0.025))
        objects.append(add_box("OrangeEye", (0.43, 0.17, z * 1.015), (0.14, 0.105, 0.028), 3, material, 0.018))
        # A short brow points toward +X and adds an aggressive, hand-authored expression.
        objects.append(add_box("EyeBrow", (0.405, 0.255, z * 1.02), (0.23, 0.045, 0.026), 5, material, 0.012,
                               rotation=(0, 0, -0.10 if z < 0 else 0.10)))

    # The lower jaw exists only at the front; together with the ram it separates
    # the face silhouette from the flat rear energy pack.
    objects.append(add_box("LowerJaw", (0.52, -0.16, 0), (0.34, 0.16, 0.50), 1, material, 0.045))

    # Four short block legs establish contact and keep the silhouette far from the player.
    for x in (-0.29, 0.31):
        for z in (-0.25, 0.25):
            objects.append(add_box("Leg", (x, -0.34, z), (0.24, 0.27, 0.22), 2, material, 0.04))
            objects.append(add_box("Foot", (x + 0.045, -0.50, z), (0.34, 0.13, 0.28), 1, material, 0.035))

    objects.extend(add_core(material))
    return objects


def join_and_export(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    model = bpy.context.object
    model.name = "CrashCube"
    # Origin is centered on the gameplay placement; feet reach about -0.57.
    bpy.ops.object.shade_smooth_by_angle()

    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    bpy.ops.wm.obj_export(
        filepath=str(OBJ),
        export_selected_objects=True,
        export_materials=True,
        export_triangulated_mesh=True,
        # Geometry is authored directly in CG2's X/Y gameplay plane. Preserve
        # those coordinates instead of applying Blender's usual Z-up -> Y-up conversion.
        forward_axis="NEGATIVE_Y",
        up_axis="Z",
    )


clear_scene()
texture = make_texture()
material = make_material(texture)
join_and_export(build(material))
print(f"CRASH_CUBE_EXPORTED:{OBJ}")
