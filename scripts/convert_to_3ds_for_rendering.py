# this script is meant to be run from blender

import bpy
import sys
import traceback
import my_3ds_exporter

argv = sys.argv
argv = argv[argv.index("--") + 1:]

output3dsPath = argv[0]
outputPngPath = argv[1]

def run():
  bpy.ops.object.mode_set(mode='OBJECT')

  #----------------------------------------
  bpy.ops.object.select_by_type(type='MESH')
  bpy.ops.object.convert(target='MESH')

  #----------------------------------------------------------------------------
  # join everything into one single object
  #----------------------------------------------------------------------------
  if len(bpy.context.selected_objects) > 1:
    bpy.ops.object.join()

  assert(len(bpy.context.selected_objects) == 1)

  # remove unused meshes
  for mesh in bpy.data.meshes:
    if mesh.users == 0:
      bpy.data.meshes.remove(mesh)

  assert(len(bpy.data.meshes) == 1)
  ob = bpy.data.meshes[0]
  ob.uv_textures[0].name = "TextureUV"
  bpy.ops.mesh.uv_texture_add()
  ob.uv_textures[1].name = "AtlasUV"

  # apply all transforms
  bpy.ops.object.transform_apply(location=True, scale=True, rotation=True)

  # ambient occlusion enable
  bpy.data.worlds["World"].ambient_color[0] = 0.1
  bpy.data.worlds["World"].ambient_color[1] = 0.1
  bpy.data.worlds["World"].ambient_color[2] = 0.1
  bpy.data.worlds["World"].light_settings.use_ambient_occlusion=True
  bpy.data.worlds["World"].light_settings.ao_factor=1.0
  bpy.data.worlds["World"].light_settings.use_environment_light=True
  bpy.data.worlds["World"].light_settings.environment_energy=0.1
  bpy.data.worlds["World"].light_settings.samples = 5

  #----------------------------------------------------------------------------
  # baking
  #----------------------------------------------------------------------------
  bpy.ops.object.mode_set(mode='EDIT')
  bpy.ops.mesh.select_all(action='SELECT')

  # Create new image
  bpy.ops.image.new(name="myLightMap", width=2048, height=2048)

  # Compute lightmap UV coords
  bpy.ops.uv.lightmap_pack()

  # Set bake margins
  bpy.context.scene.render.bake_margin = 2

  # Set the UV coordinates
  ob.uv_textures['TextureUV'].active_render = True
  ob.uv_textures['AtlasUV'].active_render = False
  ob.uv_textures.active_index = 1

  # Specify the bake type
  bpy.data.scenes["Scene"].render.bake_type = "FULL"

  # Exit edit mode
  bpy.ops.object.mode_set(mode='OBJECT')

  # Set the target image
  for d in ob.uv_textures['AtlasUV'].data:
     d.image = bpy.data.images["myLightMap"]

  # Enter edit mode
  bpy.ops.object.mode_set(mode='EDIT')
  bpy.ops.mesh.select_all(action='SELECT')

  #Bake the image
  bpy.ops.object.bake_image()

  bpy.data.images["myLightMap"].save_render(outputPngPath, bpy.context.scene)

  # Set the UV coordinates
  ob.uv_textures['TextureUV'].active_render = False
  ob.uv_textures['AtlasUV'].active_render = True
  ob.uv_textures.active_index = 1

  # bpy.ops.wm.save_mainfile(filepath="/tmp/debug.blend")

  my_3ds_exporter.saveAs3ds(bpy.context.scene, output3dsPath)

  # bpy.ops.wm.save_mainfile(filepath="/tmp/debug.blend")

try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)
