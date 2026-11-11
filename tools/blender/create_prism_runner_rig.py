"""Convert the current Prism Runner into an animation-ready Blender asset.

The runtime OBJ remains untouched so the current game appearance is preserved.
The generated BLEND/GLB contain an armature and deterministic rigid-ish weights
that can be refined when animation clips are authored.
"""

from pathlib import Path
import bpy


ROOT = Path(__file__).resolve().parents[2]
PLAYER_DIR = ROOT / "project" / "Resources" / "Player"
SOURCE_OBJ = PLAYER_DIR / "prism_runner.obj"
RIG_BLEND = PLAYER_DIR / "prism_runner_rigged.blend"
RIG_GLB = PLAYER_DIR / "prism_runner_rigged.glb"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def import_runner():
    bpy.ops.wm.obj_import(filepath=str(SOURCE_OBJ), forward_axis="NEGATIVE_Y", up_axis="Z")
    meshes = [obj for obj in bpy.context.selected_objects if obj.type == "MESH"]
    if not meshes:
        raise RuntimeError("Prism Runner OBJ did not contain a mesh")
    mesh = meshes[0]
    mesh.name = "PrismRunner_Mesh"
    mesh.data.name = "PrismRunner_Geometry"
    return mesh


def make_armature():
    armature_data = bpy.data.armatures.new("PrismRunner_Armature")
    armature = bpy.data.objects.new("PrismRunner_Rig", armature_data)
    bpy.context.collection.objects.link(armature)
    armature.show_in_front = True
    bpy.context.view_layer.objects.active = armature
    armature.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")

    def bone(name, head, tail, parent=None):
        result = armature_data.edit_bones.new(name)
        result.head = head
        result.tail = tail
        if parent:
            result.parent = armature_data.edit_bones[parent]
        return result

    # The CG2 model is authored in the X/Y gameplay plane, with Z as depth.
    bone("Root", (0.0, -0.46, 0.0), (0.0, -0.31, 0.0))
    bone("Pelvis", (0.0, -0.31, 0.0), (0.0, -0.10, 0.0), "Root")
    bone("Spine", (0.0, -0.10, 0.0), (0.0, 0.18, 0.0), "Pelvis")
    bone("Head", (0.0, 0.18, 0.0), (0.0, 0.47, 0.0), "Spine")
    bone("Arm_Near", (0.0, 0.10, -0.12), (0.05, -0.16, -0.24), "Spine")
    bone("Arm_Far", (0.0, 0.10, 0.12), (0.05, -0.16, 0.24), "Spine")
    bone("Leg_Near", (0.0, -0.15, -0.08), (0.02, -0.43, -0.14), "Pelvis")
    bone("Leg_Far", (0.0, -0.15, 0.08), (0.02, -0.43, 0.14), "Pelvis")
    bone("Backpack", (-0.12, 0.10, 0.0), (-0.28, 0.18, 0.0), "Spine")

    bpy.ops.object.mode_set(mode="OBJECT")
    return armature


def assign_weights(mesh, armature):
    names = [
        "Root", "Pelvis", "Spine", "Head", "Arm_Near", "Arm_Far",
        "Leg_Near", "Leg_Far", "Backpack",
    ]
    groups = {name: mesh.vertex_groups.new(name=name) for name in names}

    for vertex in mesh.data.vertices:
        x, y, z = vertex.co
        # Use one dominant bone per vertex to preserve the intentionally blocky
        # hard-surface construction instead of bending armour like rubber.
        if y >= 0.16:
            group = "Head"
        elif y <= -0.13:
            group = "Leg_Near" if z < 0.0 else "Leg_Far"
        elif abs(z) >= 0.13 and y < 0.16:
            group = "Arm_Near" if z < 0.0 else "Arm_Far"
        elif x <= -0.22 and y >= -0.02:
            group = "Backpack"
        elif y < -0.04:
            group = "Pelvis"
        else:
            group = "Spine"
        groups[group].add([vertex.index], 1.0, "REPLACE")

    modifier = mesh.modifiers.new("PrismRunnerArmature", "ARMATURE")
    modifier.object = armature
    mesh.parent = armature


def add_animation_slots(armature):
    # Empty named actions establish the clip contract without changing the rest pose.
    for name in ("Idle", "Run", "Jump", "Land", "Damage", "Respawn"):
        action = bpy.data.actions.new(name=name)
        action.use_fake_user = True
    armature["animation_clip_contract"] = "Idle,Run,Jump,Land,Damage,Respawn"
    armature["forward_axis_runtime"] = "+X"


def save_and_export(mesh, armature):
    bpy.ops.object.select_all(action="DESELECT")
    mesh.select_set(True)
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    bpy.ops.wm.save_as_mainfile(filepath=str(RIG_BLEND))
    bpy.ops.export_scene.gltf(
        filepath=str(RIG_GLB),
        export_format="GLB",
        use_selection=True,
        export_skins=True,
        export_animations=True,
        export_yup=True,
    )


clear_scene()
runner = import_runner()
rig = make_armature()
assign_weights(runner, rig)
add_animation_slots(rig)
save_and_export(runner, rig)
print(f"PRISM_RUNNER_RIG_EXPORTED:{RIG_BLEND}:{RIG_GLB}")
