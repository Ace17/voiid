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
  bpy.ops.object.select_by_type(type='MESH')

  if len(bpy.context.selected_objects) > 0:
    bpy.context.view_layer.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.convert(target='MESH')

  bpy.ops.object.select_by_type(type='FONT')
  for obj in bpy.context.selected_objects:
    bpy.context.view_layer.objects.active = obj
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
    bpy.context.view_layer.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.join()

  assert(len(bpy.context.selected_objects) == 1)

  # remove unused meshes
  for mesh in bpy.data.meshes:
    if mesh.users == 0:
      bpy.data.meshes.remove(mesh)

  # apply all transforms
  bpy.ops.object.transform_apply(location=True, scale=True, rotation=True)

  # Delete faces with material 'nope' (i.e "don't render me")
  deleteNonRenderedFaces()

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
  bmesh.ops.delete(bm, geom=faces_select, context='FACES_ONLY')

  # copy bmesh to mesh
  bm.to_mesh(me)

def exportMesh(scene, filepath=""):
    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode='OBJECT')

    mesh_objects = []

    for obj in scene.objects:
        free, derived = create_derived_objects(scene, obj)

        if derived is None:
            continue

        for ob_derived, mat in derived:
            # sys.stderr.write("obj.type=" + str(obj.type) + "\n")
            if obj.type not in {'MESH', 'CURVE', 'SURFACE', 'FONT', 'META'}:
                continue

            data = ob_derived.to_mesh()

            if data is None:
                continue

            matrix = mat
            data.transform(matrix)
            mesh_objects.append((ob_derived, data, matrix))

        if free:
            free_derived_objects(obj)

    # sys.stderr.write("mesh_objects.len=" + str(len(mesh_objects)) + "\n")
    assert(len(mesh_objects) == 1)

    # Create object chunks for all meshes:
    (obj, blender_mesh, matrix) = mesh_objects[0]

    # make a mesh chunk out of the mesh:
    dumpMesh(blender_mesh, filepath)

class Vertex(object):
    def __init__(self, pos, normal, uv):
        self.pos = pos
        self.normal = normal
        self.uv = uv

class Triangle(object):
    def __init__(self, a, b, c):
        self.a = a
        self.b = b
        self.c = c

def extractTriangles(mesh):

    mesh.calc_loop_triangles()

    uv_layer = mesh.uv_layers[0]

    triangles = []
    for tri in mesh.loop_triangles:
        uv0 = uv_layer.data[tri.loops[0]].uv
        uv1 = uv_layer.data[tri.loops[1]].uv
        uv2 = uv_layer.data[tri.loops[2]].uv
        xyz0 = mesh.vertices[tri.vertices[0]].co
        xyz1 = mesh.vertices[tri.vertices[1]].co
        xyz2 = mesh.vertices[tri.vertices[2]].co
        n0 = mesh.vertices[tri.vertices[0]].normal
        n1 = mesh.vertices[tri.vertices[1]].normal
        n2 = mesh.vertices[tri.vertices[2]].normal
        triangles.append(Triangle(
            Vertex(xyz0, n0, uv0),
            Vertex(xyz1, n1, uv1),
            Vertex(xyz2, n2, uv2)
            ))

    return triangles

def dumpMesh(mesh, filepath):
    triangles = extractTriangles(mesh)

    file = open(filepath, 'w')
    file.write("# x y z - nx ny nz - u1 v1\n")

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
            line += str(round(vertex.uv[0], 6)) # u1
            line += " "
            line += str(round(vertex.uv[1], 6)) # v1
            line += " - "
            line += str(round(vertex.uv[0], 6)) # u1
            line += " "
            line += str(round(vertex.uv[1], 6)) # v1
            file.write(line + "\n")
        file.write("\n")
    file.close()

try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)

