# this script is meant to be run from blender

import bpy
import sys
import traceback

argv = sys.argv
argv = argv[argv.index("--") + 1:]

outputBlendPath = argv[0]

def run():
  # Isolate ourselves from the current obj/edit mode
  # and the current selection.
  ensureInitialStateIsClean()
  bpy.ops.wm.save_mainfile(filepath=outputBlendPath)

def ensureInitialStateIsClean():
  # ensure we're not in 'edit mode'
  for mesh in bpy.data.meshes:
    if mesh.is_editmode == True:
        print("mesh ", mesh, "is in edit mode", file=sys.stderr)
    assert(mesh.is_editmode == False)

  # ensure everything is deselected
  assert(len(bpy.context.selected_objects) == 0)

try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)

