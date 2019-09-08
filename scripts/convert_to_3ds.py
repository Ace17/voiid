# this script is meant to be run from blender

import bpy
import sys
import traceback
import my_3ds_exporter

argv = sys.argv
argv = argv[argv.index("--") + 1:]

outputPath = argv[0]

def run():
  bpy.ops.object.select_all(action='DESELECT')
  bpy.ops.object.select_all(action='SELECT')
  
  bpy.context.view_layer.objects.active = bpy.data.objects[0]
  bpy.ops.object.convert(target='MESH')
  
  # apply all transforms
  bpy.ops.object.select_all(action='SELECT')
  bpy.ops.object.transform_apply(location=True, scale=True, rotation=True)

  my_3ds_exporter.saveAs3ds(bpy.context.scene, outputPath)


try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)
