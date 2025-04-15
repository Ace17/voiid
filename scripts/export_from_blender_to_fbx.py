# this script is meant to be run from blender

import bpy
import sys
import traceback

argv = sys.argv
argv = argv[argv.index("--") + 1:]

outputMeshPath = argv[0]

def run():

  bpy.ops.export_scene.fbx(filepath=outputMeshPath,
    bake_anim=False,
    global_scale = 1.0,
    apply_unit_scale = True,
    apply_scale_options = 'FBX_SCALE_NONE',
    use_space_transform = True,
    bake_space_transform = False,
    object_types = {'LIGHT', 'EMPTY', 'MESH'},
    use_mesh_modifiers = True,
    use_mesh_modifiers_render = True,
    mesh_smooth_type = 'OFF',
    use_subsurf = False,
    use_mesh_edges = False,
    use_tspace = True,
    use_triangles = True,
    use_custom_props=True,
    add_leaf_bones = True,
    primary_bone_axis = 'Y',
    secondary_bone_axis = 'X',
    use_armature_deform_only = False,
    armature_nodetype = 'NULL',
    path_mode = 'AUTO',
    embed_textures = False,
    batch_mode = 'OFF',
    use_batch_own_dir = True,
    axis_forward = 'Y',
    axis_up = 'Z')

try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)

