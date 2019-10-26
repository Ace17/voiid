# this script is meant to be run from blender
# Preprocess a blender file so it's fully standalone

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
  makeStandalone()
  bpy.ops.wm.save_mainfile(filepath=outputBlendPath)

def ensureInitialStateIsClean():
  bpy.ops.object.select_all(action='DESELECT')
  bpy.context.view_layer.objects.active = None

  # ensure we're not in 'edit mode'
  for mesh in bpy.data.meshes:
    if mesh.is_editmode == True:
        print("mesh ", mesh, "is in edit mode", file=sys.stderr)
    assert(mesh.is_editmode == False)

  # ensure everything is deselected
  assert(len(bpy.context.selected_objects) == 0)

def makeStandalone():
  for o in bpy.data.objects:
      o.make_local()
  for o in bpy.data.meshes:
      o.make_local()
  for o in bpy.data.materials:
      o.make_local()
  for o in bpy.data.textures:
      o.make_local()
  for o in bpy.data.images:
      o.make_local()

  bpy.ops.object.select_all(action='SELECT')
  bpy.ops.object.duplicates_make_real()
  bpy.ops.object.make_single_user(type='SELECTED_OBJECTS', object=True, obdata=True)
  bpy.ops.object.transform_apply(location=True, scale=True, rotation=True)
  bpy.ops.object.select_all(action='DESELECT')

  bpy.ops.file.pack_all()

try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)

