# this script is meant to be run from blender

import bpy
import bmesh
import sys
import traceback

from bpy_extras.io_utils import create_derived_objects, free_derived_objects

argv = sys.argv
argv = argv[argv.index("--") + 1:]

outputMeshPath = argv[0]

def run():
  bpy.ops.object.select_by_type(type='MESH')

  if len(bpy.context.selected_objects) > 0:
    bpy.context.view_layer.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.convert(target='MESH')

  bpy.ops.object.select_by_type(type='FONT')
  for obj in bpy.context.selected_objects:
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.convert(target='MESH')

  # remove unused meshes
  for mesh in bpy.data.meshes:
    if mesh.users == 0:
      bpy.data.meshes.remove(mesh)

  # apply all transforms
  bpy.ops.object.transform_apply(location=True, scale=True, rotation=True)

  exportMeshes(bpy.context.scene, outputMeshPath)

def exportMeshes(scene, filepath=""):
    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode='OBJECT')

    file = open(filepath, 'w')
    mesh_objects = []

    for obj in scene.objects:
        free, derived = create_derived_objects(scene, obj)

        if derived is None:
            continue

        for ob_derived, matrix in derived:
            if obj.type == 'EMPTY':
                pass
            elif obj.type == 'LIGHT':
                light = obj.data
                # file.write("light: \"%s\"\n" % str(light.name))
                # file.write("%s " % str(round(obj.location.x, 6)))
                # file.write("%s " % str(round(obj.location.y, 6)))
                # file.write("%s\n" % str(round(obj.location.z, 6)))
                # file.write("%s " % str(round(light.color.r, 2)))
                # file.write("%s " % str(round(light.color.g, 2)))
                # file.write("%s\n" % str(round(light.color.b, 2)))
                # file.write("\n")
            elif obj.type in {'MESH', 'CURVE', 'SURFACE', 'FONT', 'META'}:
                theMesh = ob_derived.to_mesh()

                if theMesh is None:
                    continue

                theMesh.transform(matrix)
                try:
                    dumpMesh(theMesh, file, ob_derived)
                except Exception:
                    sys.stderr.write("Skipping mesh: '" + theMesh.name + "'\n")

                file.write("\n")
            else:
                sys.stderr.write("Skipping unknown type: '%s'\n" % str(obj.type))

        if free:
            free_derived_objects(obj)

    file.close()

class Vertex(object):
    def __init__(self, pos, normal, uv):
        self.pos = pos
        self.normal = normal
        self.uv = uv

class Triangle(object):
    def __init__(self, a, b, c, mat):
        self.mat = mat
        self.a = a
        self.b = b
        self.c = c

def extractTriangles(mesh):
    mesh.calc_loop_triangles()

    if len(mesh.uv_layers) == 0:
        sys.stderr.write("Mesh '" + mesh.name + "' has no UV layer\n")

    uv_layer = mesh.uv_layers[0]

    triangles = {}
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
        if not tri.material_index in triangles:
            triangles[tri.material_index] = []
        triangles[tri.material_index].append(Triangle(
            Vertex(xyz0, n0, uv0),
            Vertex(xyz1, n1, uv1),
            Vertex(xyz2, n2, uv2),
            tri.material_index
            ))

    return triangles

def dumpMesh(mesh, file, obj):

    file.write("obj: \"" + obj.name + "\"\n")
    triangles = extractTriangles(mesh)
    for propName in obj.keys():
        if propName == "_RNA_UI":
            continue
        file.write("prop: \"" + propName + "\"=\"" + str(obj[propName]) + "\"\n")

    for material_index in triangles.keys():
        file.write("material: \"" + mesh.materials[material_index].name + "\"\n")
        for tri in triangles[material_index]:
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
                file.write(line + "\n")

try:
  run()
  sys.exit(0)
except Exception:
  traceback.print_exc()
  sys.exit(1)

