# this script is meant to be run from blender

import bpy
import sys
import traceback

argv = sys.argv
argv = argv[argv.index("--") + 1:]

outputMeshPath = argv[0]

def run():

  # Force triangulation of all objects before export
  for obj in bpy.context.scene.objects:
    #sys.stderr.write(f"{obj.type} {obj.name}\t\t\t\t{obj}\n")
    mod = obj.modifiers.new("Toto", 'TRIANGULATE')

  bpy.ops.export_scene.fbx(filepath=outputMeshPath,
          axis_up="Z",
          axis_forward="Y",
          use_custom_props=True,
          bake_anim=False)

try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)

