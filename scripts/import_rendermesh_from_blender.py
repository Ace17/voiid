# this script is meant to be run from blender

import bpy
import bmesh
import sys
import traceback

from bpy_extras.io_utils import create_derived_objects, free_derived_objects

argv = sys.argv
argv = argv[argv.index("--") + 1:]

outputMeshPath = argv[0]
outputPngPath = argv[1]

def run():
  #----------------------------------------
  bpy.ops.object.select_by_type(type='MESH')

  if len(bpy.context.selected_objects) > 0:
    bpy.context.scene.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.convert(target='MESH')

  bpy.ops.object.select_by_type(type='FONT')
  for obj in bpy.context.selected_objects:
    bpy.context.scene.objects.active = obj
    bpy.ops.object.convert(target='MESH')

  # remove non-displayed objects (triggers)
  for obj in bpy.data.objects:
    if obj.name.startswith("f."):
      bpy.data.objects.remove(obj)

  #----------------------------------------------------------------------------
  # join everything into one single object
  #----------------------------------------------------------------------------
  bpy.ops.object.select_by_type(type='MESH')
  if len(bpy.context.selected_objects) > 0:
    bpy.context.scene.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.join()

  assert(len(bpy.context.selected_objects) == 1)

  # remove unused meshes
  for mesh in bpy.data.meshes:
    if mesh.users == 0:
      bpy.data.meshes.remove(mesh)

  # apply all transforms
  bpy.ops.object.transform_apply(location=True, scale=True, rotation=True)

  assert(len(bpy.data.meshes) == 1)

  ob = bpy.data.meshes[0]
  ob.uv_textures[0].name = "DiffuseUV"
  bpy.ops.mesh.uv_texture_add()
  ob.uv_textures[1].name = "LightmapUV"

  # ambient occlusion enable
  # bpy.data.worlds["World"].ambient_color[0] = 0.1
  # bpy.data.worlds["World"].ambient_color[1] = 0.1
  # bpy.data.worlds["World"].ambient_color[2] = 0.1
  # bpy.data.worlds["World"].light_settings.use_ambient_occlusion=True
  # bpy.data.worlds["World"].light_settings.ao_factor=1.0
  # bpy.data.worlds["World"].light_settings.use_environment_light=True
  # bpy.data.worlds["World"].light_settings.environment_energy=0.1
  # bpy.data.worlds["World"].light_settings.samples = 5

  #----------------------------------------------------------------------------
  # baking
  #----------------------------------------------------------------------------

  # Delete faces with material 'nope' (i.e "don't render me")
  deleteNonRenderedFaces()

  bpy.ops.object.mode_set(mode='EDIT')
  bpy.ops.mesh.select_all(action='SELECT')

  # Create new image
  bpy.ops.image.new(name="myLightMap", width=512, height=512)

  # Compute lightmap UV coords
  bpy.ops.uv.lightmap_pack()

  # Set bake margins
  bpy.context.scene.render.bake_margin = 2

  # Set the UV coordinates
  ob.uv_textures['DiffuseUV'].active_render = True
  ob.uv_textures['LightmapUV'].active_render = False
  ob.uv_textures.active_index = 1

  # Specify the bake type
  bpy.data.scenes["Scene"].render.bake_type = "FULL"

  # Exit edit mode
  bpy.ops.object.mode_set(mode='OBJECT')

  # Set the target image
  for d in ob.uv_textures['LightmapUV'].data:
     d.image = bpy.data.images["myLightMap"]

  # Enter edit mode
  bpy.ops.object.mode_set(mode='EDIT')
  bpy.ops.mesh.select_all(action='SELECT')

  #Bake the image
  bpy.ops.object.bake_image()

  bpy.data.images["myLightMap"].save_render(outputPngPath, bpy.context.scene)

  # Set the UV coordinates
  ob.uv_textures['DiffuseUV'].active_render = False
  ob.uv_textures['LightmapUV'].active_render = True
  ob.uv_textures.active_index = 1

  exportMesh(bpy.context.scene, outputMeshPath)

  # bpy.ops.wm.save_mainfile(filepath="/tmp/debug.blend")

def deleteNonRenderedFaces():
  # Get the active mesh
  bpy.ops.object.mode_set(mode='OBJECT')
  obj = bpy.context.object

  if len(obj.material_slots) == 0:
      return

  # Get the active mesh
  me = obj.data

  # Get a BMesh representation
  bm = bmesh.new()   # create an empty BMesh
  bm.from_mesh(me)   # fill it in from a Mesh

  faces_select = []
  for f in bm.faces:
      # sys.stderr.write("material_index=" + str(f.material_index) + "\n")
      if obj.material_slots[f.material_index].name == "nope":
          faces_select.append(f)
  bmesh.ops.delete(bm, geom=faces_select, context=3) # ONLY_FACES

  # copy bmesh to mesh
  bm.to_mesh(me)

def exportMesh(scene, filepath=""):
    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode='OBJECT')

    # Make a list of all materials used in the selected meshes (use a dictionary,
    # each material is added once):
    materialDict = {}
    mesh_objects = []

    objects = (ob for ob in scene.objects if ob.is_visible(scene))

    for ob in objects:
        # get derived objects
        free, derived = create_derived_objects(scene, ob)

        if derived is None:
            continue

        for ob_derived, mat in derived:
            if ob.type not in {'MESH', 'CURVE', 'SURFACE', 'FONT', 'META'}:
                continue

            try:
                data = ob_derived.to_mesh(scene, True, 'PREVIEW')
            except:
                data = None

            if data is None:
                continue

            matrix = mat
            data.transform(matrix)
            mesh_objects.append((ob_derived, data, matrix))

            # get material/image tuples.
            mat_ls = data.materials
            mat_ls_len = len(mat_ls)

            if data.tessface_uv_textures:
                if not mat_ls:
                    mat = mat_name = None

                for f, uf in zip(data.tessfaces, data.tessface_uv_textures.active.data):
                    if mat_ls:
                        mat_index = f.material_index
                        if mat_index >= mat_ls_len:
                            mat_index = f.mat = 0
                        mat = mat_ls[mat_index]
                        mat_name = None if mat is None else mat.name
                    # else there already set to none

                    img = uf.image
                    img_name = None if img is None else img.name

                    materialDict.setdefault((mat_name, img_name), (mat, img))

            else:
                for mat in mat_ls:
                    if mat:  # material may be None so check its not.
                        materialDict.setdefault((mat.name, None), (mat, None))

                for f in data.tessfaces:
                    if f.material_index >= mat_ls_len:
                        f.material_index = 0

        if free:
            free_derived_objects(ob)

    assert(len(mesh_objects) == 1)

    # Create object chunks for all meshes:
    (ob, blender_mesh, matrix) = mesh_objects[0]

    # make a mesh chunk out of the mesh:
    dumpMesh(blender_mesh, filepath)

class Vertex(object):
    def __init__(self, pos, normal, uv1, uv2):
        self.pos = pos
        self.normal = normal
        self.uv1 = uv1
        self.uv2 = uv2

class Triangle(object):
    def __init__(self, a, b, c):
        self.a = a
        self.b = b
        self.c = c

def extractTriangles(mesh):
    assert(mesh.tessface_uv_textures)

    vertices = []
    for k in mesh.vertices.values():
        vertices.append(k)

    triangles = []
    for i, face in enumerate(mesh.tessfaces):
        idx = face.vertices

        # diffuse
        uv1 = mesh.tessface_uv_textures[0].data[i].uv

        # lightmap
        uv2 = mesh.tessface_uv_textures[1].data[i].uv

        if len(idx) == 3:
            triangles.append(Triangle(
                Vertex(vertices[idx[0]].co, face.normal, uv1[0], uv2[0]),
                Vertex(vertices[idx[1]].co, face.normal, uv1[1], uv2[1]),
                Vertex(vertices[idx[2]].co, face.normal, uv1[2], uv2[2])
                ))
        else:  # it's a quad
            triangles.append(Triangle(
                Vertex(vertices[idx[0]].co, face.normal, uv1[0], uv2[0]),
                Vertex(vertices[idx[1]].co, face.normal, uv1[1], uv2[1]),
                Vertex(vertices[idx[2]].co, face.normal, uv1[2], uv2[2])
                ))
            triangles.append(Triangle(
                Vertex(vertices[idx[0]].co, face.normal, uv1[0], uv2[0]),
                Vertex(vertices[idx[2]].co, face.normal, uv1[2], uv2[2]),
                Vertex(vertices[idx[3]].co, face.normal, uv1[3], uv2[3])
                ))

    return triangles

def dumpMesh(mesh, filepath):
    triangles = extractTriangles(mesh)

    file = open(filepath, 'w')
    file.write("# x y z - nx ny nz - u1 v1 - u2 v2\n")

    for tri in triangles:
        for vertex in (tri.a, tri.b, tri.c):
            line = ""
            line += str(round(vertex.pos.x, 6))
            line += " "
            line += str(round(vertex.pos.y, 6))
            line += " "
            line += str(round(vertex.pos.z, 6))
            line += " - "
            line += str(round(vertex.normal.x, 2))
            line += " "
            line += str(round(vertex.normal.y, 2))
            line += " "
            line += str(round(vertex.normal.z, 2))
            line += " - "
            line += str(round(vertex.uv1[0], 6)) # u1
            line += " "
            line += str(round(vertex.uv1[1], 6)) # v1
            line += " - "
            line += str(round(vertex.uv2[0], 6)) # u1
            line += " "
            line += str(round(vertex.uv2[1], 6)) # v1
            file.write(line + "\n")
        file.write("\n")

    file.close()


try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)

