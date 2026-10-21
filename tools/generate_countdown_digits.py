import bpy
import math
import os

OUTPUT_DIR = r"C:\CG2\CG2\project\Resources\Title"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def export_digit(digit: str):
    clear_scene()
    curve = bpy.data.curves.new(type="FONT", name=f"Countdown{digit}Curve")
    curve.body = digit
    curve.align_x = "CENTER"
    curve.align_y = "CENTER"
    curve.size = 3.0
    curve.extrude = 0.13
    curve.bevel_depth = 0.025
    curve.bevel_resolution = 2
    curve.resolution_u = 8

    obj = bpy.data.objects.new(f"Countdown{digit}", curve)
    bpy.context.collection.objects.link(obj)
    obj.rotation_euler.x = math.radians(90.0)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

    material = bpy.data.materials.new(name="CountdownDigitMaterial")
    # Runtime側のSetColorでGO!と同じ色を付けるため、素材自体は白にする。
    material.diffuse_color = (1.0, 1.0, 1.0, 1.0)
    material.metallic = 0.0
    material.roughness = 0.62
    obj.data.materials.append(material)

    output_path = os.path.join(OUTPUT_DIR, f"countdown_{digit}.obj")
    bpy.ops.wm.obj_export(
        filepath=output_path,
        export_selected_objects=True,
        export_materials=True,
        export_uv=False,
        export_normals=True,
        export_triangulated_mesh=True,
        forward_axis="NEGATIVE_Z",
        up_axis="Y",
    )


for value in ("1", "2", "3"):
    export_digit(value)
