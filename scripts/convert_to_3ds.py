# this script is meant to be run from blender
import bpy
import sys
import traceback

argv = sys.argv
argv = argv[argv.index("--") + 1:]

outputPath = argv[0]

def run():
  bpy.ops.object.mode_set(mode='OBJECT', toggle=False)
  bpy.ops.object.select_all(action='DESELECT')
  bpy.ops.object.select_all(action='SELECT')
  
  bpy.ops.object.convert(target='MESH')
  
  print("keys before: ", len(bpy.context.scene.objects.keys()))
 
#    # join everything into one single object
#    if len(bpy.context.scene.objects.keys()) > 1:
#      bpy.ops.object.join()
#    
#    print("keys after: ", len(bpy.context.scene.objects.keys()))
 
#    bpy.ops.object.mode_set(mode='OBJECT', toggle=False)"
 
  # apply all transforms"
  bpy.ops.object.select_all(action='SELECT')
  bpy.ops.object.transform_apply(location=True, scale=True, rotation=True)

  bpy.ops.export_scene.autodesk_3ds(filepath=outputPath)

try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)
