"""Create the bright clear-scene sky dome and GOAL CLEAR 3D lettering."""

from pathlib import Path
import math
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Clear"
OUT.mkdir(parents=True, exist_ok=True)
FONT = Path("C:/Windows/Fonts/BIZ-UDGothicB.ttc")


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_image(name, width, height, pixel_fn):
    image = bpy.data.images.new(name, width=width, height=height, alpha=True)
    pixels = []
    for y in range(height):
        v = y / max(height - 1, 1)
        for x in range(width):
            u = x / max(width - 1, 1)
            pixels.extend(pixel_fn(u, v))
    image.pixels = pixels
    image.filepath_raw = str(OUT / f"{name}.png")
    image.file_format = "PNG"
    image.save()
    return image


def material_with_texture(name, image, roughness=0.55):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Linear"
    shader.inputs["Roughness"].default_value = roughness
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(texture.outputs["Alpha"], shader.inputs["Alpha"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return mat


def sky_pixel(u, v):
    # equirectangular gradient: pale gold at the horizon, cyan in the middle,
    # and clean blue above.  Broad hand-placed bands keep it graphic rather
    # than looking like a generated photograph.
    top = (0.18, 0.50, 0.88)
    middle = (0.28, 0.84, 0.94)
    horizon = (1.00, 0.87, 0.48)
    if v < 0.50:
        # Lower hemisphere stays cool and unobtrusive, reaching the warm
        # horizon at the equator.
        t = v / 0.50
        lower = (0.20, 0.65, 0.82)
        base = tuple(lower[i] * (1.0 - t) + horizon[i] * t for i in range(3))
    else:
        t = (v - 0.50) / 0.50
        if t < 0.48:
            local = t / 0.48
            base = tuple(horizon[i] * (1.0 - local) + middle[i] * local for i in range(3))
        else:
            local = (t - 0.48) / 0.52
            base = tuple(middle[i] * (1.0 - local) + top[i] * local for i in range(3))

    # A restrained sun glow and a few long cloud bands.
    dx = min(abs(u - 0.50), 1.0 - abs(u - 0.50))
    dy = v - 0.52
    glow = max(0.0, 1.0 - math.sqrt((dx / 0.20) ** 2 + (dy / 0.13) ** 2))
    band = max(0.0, math.sin(u * math.pi * 8.0 + v * 5.0))
    band *= max(0.0, 1.0 - abs(v - 0.58) / 0.16) * 0.045
    rgb = [min(1.0, base[i] + glow * (0.22, 0.14, 0.03)[i] + band) for i in range(3)]
    return (*rgb, 1.0)


def export_selected(path):
    bpy.ops.wm.obj_export(
        filepath=str(path),
        export_selected_objects=True,
        export_materials=True,
        export_triangulated_mesh=True,
        forward_axis="NEGATIVE_Y",
        up_axis="Z",
    )


def create_sky():
    image = make_image("prism_clear_sky", 1024, 512, sky_pixel)
    mat = material_with_texture("PrismClearSkyMaterial", image, 1.0)
    bpy.ops.mesh.primitive_uv_sphere_add(segments=64, ring_count=32, radius=1.0)
    sphere = bpy.context.object
    sphere.name = "prism_clear_sphere"
    sphere.data.materials.append(mat)
    # Render the inside of the dome with outward culling enabled.
    bpy.context.view_layer.objects.active = sphere
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.flip_normals()
    bpy.ops.object.mode_set(mode="OBJECT")
    export_selected(OUT / "prism_clear_sphere.obj")
    bpy.data.objects.remove(sphere, do_unlink=True)


def create_text(name, body, target_width, extrusion, target_height=None):
    image = make_image(
        "goal_clear_text",
        64,
        64,
        lambda _u, _v: (0.98, 0.99, 1.0, 1.0),
    )
    mat = material_with_texture("GoalClearTextMaterial", image, 0.35)
    bpy.ops.object.text_add(location=(0, 0, 0))
    text = bpy.context.object
    text.name = name
    text.data.body = body
    if FONT.exists():
        text.data.font = bpy.data.fonts.load(str(FONT))
    text.data.align_x = "CENTER"
    text.data.align_y = "CENTER"
    text.data.size = 1.0
    text.data.extrude = extrusion
    text.data.bevel_depth = extrusion * 0.16
    text.data.bevel_resolution = 2
    text.data.fill_mode = "BOTH"
    text.data.materials.append(mat)
    bpy.context.view_layer.objects.active = text
    bpy.ops.object.convert(target="MESH")
    # A word can be fitted by width, but individual glyphs must share a common
    # height.  Fitting every glyph to the same width enlarged narrow letters
    # such as L and destroyed the baseline/size balance.
    scale = (target_height / max(text.dimensions.y, 0.001)
             if target_height is not None
             else target_width / max(text.dimensions.x, 0.001))
    text.scale = (scale, scale, scale)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    text.rotation_euler.x = -math.pi * 0.5
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    uv = text.data.uv_layers.active or text.data.uv_layers.new(name="UVMap")
    for loop in uv.data:
        loop.uv = (0.5, 0.5)
    export_selected(OUT / f"{name}.obj")


clear_scene()
create_sky()
create_text("goal_clear", "GOAL CLEAR", 7.2, 0.075)
create_text("press_enter_stage_select", "PRESS ENTER", 3.6, 0.045)
for index, glyph in enumerate("GOALCLEAR"):
    create_text(f"goal_clear_letter_{index:02d}", glyph, 0.72, 0.075,
                target_height=1.32)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "clear_presentation.blend"))
print(f"CLEAR_PRESENTATION_EXPORTED:{OUT}")
