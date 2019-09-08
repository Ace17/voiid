# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
# Copyright (C) Bob Holcomb
# Contributors: Campbell Barton, Bob Holcomb, Richard Lärkäng, Damien McGinnes, Mark Stijnman

# Exports the scene to a 3DS file,
# but without the 11-characters object name limitation.

import bpy
import sys
import traceback

def saveAs3ds(scene, filepath):

    import bpy
    import mathutils

    from bpy_extras.io_utils import create_derived_objects, free_derived_objects

    """Save the Blender scene to a 3ds file."""

    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode='OBJECT')

    # Initialize the main chunk (primary):
    primary = _3ds_chunk(PRIMARY)
    # Add version chunk:
    version_chunk = _3ds_chunk(VERSION)
    version_chunk.add_variable("version", _3ds_uint(3))
    primary.add_subchunk(version_chunk)

    # init main object info chunk:
    object_info = _3ds_chunk(OBJECTINFO)

    mesh_objects = []

    objects = scene.objects

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

            if data:
                data.transform(mat)
                mesh_objects.append((ob_derived, data, mat))

            for propName in ob_derived.keys():
                sys.stderr.write("prop: '" + propName + "'\n")

        if free:
            free_derived_objects(ob)

    # Give all objects a unique ID and build a dictionary from object name to object id:
    """
    name_to_id = {}
    for ob, data in mesh_objects:
        name_to_id[ob.name]= len(name_to_id)
    #for ob in empty_objects:
    #    name_to_id[ob.name]= len(name_to_id)
    """

    # Create object chunks for all meshes:
    i = 0
    for ob, blender_mesh, matrix in mesh_objects:
        # create a new object chunk
        object_chunk = _3ds_chunk(OBJECT)

        # set the object name
        myName = ob.name.encode("ASCII", "replace")
        object_chunk.add_variable("name", _3ds_string(myName))

        # make a mesh chunk out of the mesh:
        object_chunk.add_subchunk(make_mesh_chunk(blender_mesh, matrix))

        # ensure the mesh has no over sized arrays
        # skip ones that do!, otherwise we cant write since the array size wont
        # fit into USHORT.
        if object_chunk.validate():
            object_info.add_subchunk(object_chunk)
        else:
            print("Object can't be written into a 3DS file")

        if not blender_mesh.users:
            bpy.data.meshes.remove(blender_mesh)
        #blender_mesh.vertices = None

        i += i

    # Add main object info chunk to primary chunk:
    primary.add_subchunk(object_info)

    # COMMENTED OUT FOR 2.42 RELEASE!! CRASHES 3DS MAX
    # # Add main keyframe data chunk to primary chunk:
    # primary.add_subchunk(kfdata)

    # At this point, the chunk hierarchy is completely built.

    # Check the size:
    primary.get_size()

    # Open the file for writing:
    file = open(filepath, 'wb')

    # Recursively write the chunks to file:
    primary.write(file)

    # Close the file:
    file.close()

    # Clear name mapping vars, could make locals too
    del name_unique[:]
    name_mapping.clear()

    # Debugging only: dump the chunk hierarchy:
    #primary.dump()

######################################################
# Data Structures
######################################################

#Some of the chunks that we will export
#----- Primary Chunk, at the beginning of each file
PRIMARY = 0x4D4D

#------ Main Chunks
OBJECTINFO = 0x3D3D  # This gives the version of the mesh and is found right before the material and object information
VERSION = 0x0002  # This gives the version of the .3ds file
KFDATA = 0xB000  # This is the header for all of the key frame info

#------ sub defines of OBJECTINFO
MATERIAL = 45055  # 0xAFFF // This stored the texture info
OBJECT = 16384  # 0x4000 // This stores the faces, vertices, etc...

#>------ sub defines of MATERIAL
MATNAME = 0xA000  # This holds the material name
MATAMBIENT = 0xA010  # Ambient color of the object/material
MATDIFFUSE = 0xA020  # This holds the color of the object/material
MATSPECULAR = 0xA030  # SPecular color of the object/material
MATSHINESS = 0xA040  # ??

MAT_DIFFUSEMAP = 0xA200  # This is a header for a new diffuse texture
MAT_OPACMAP = 0xA210  # head for opacity map
MAT_BUMPMAP = 0xA230  # read for normal map
MAT_SPECMAP = 0xA204  # read for specularity map

#>------ sub defines of MAT_???MAP
MATMAPFILE = 0xA300  # This holds the file name of a texture

MAT_MAP_TILING = 0xa351   # 2nd bit (from LSB) is mirror UV flag
MAT_MAP_USCALE = 0xA354   # U axis scaling
MAT_MAP_VSCALE = 0xA356   # V axis scaling
MAT_MAP_UOFFSET = 0xA358  # U axis offset
MAT_MAP_VOFFSET = 0xA35A  # V axis offset
MAT_MAP_ANG = 0xA35C      # UV rotation around the z-axis in rad

RGB1 = 0x0011
RGB2 = 0x0012

#>------ sub defines of OBJECT
OBJECT_MESH = 0x4100  # This lets us know that we are reading a new object
OBJECT_LIGHT = 0x4600  # This lets un know we are reading a light object
OBJECT_CAMERA = 0x4700  # This lets un know we are reading a camera object

#>------ sub defines of CAMERA
OBJECT_CAM_RANGES = 0x4720      # The camera range values

#>------ sub defines of OBJECT_MESH
OBJECT_VERTICES = 0x4110  # The objects vertices
OBJECT_FACES = 0x4120  # The objects faces
OBJECT_MATERIAL = 0x4130  # This is found if the object has a material, either texture map or color
OBJECT_UV = 0x4140  # The UV texture coordinates
OBJECT_TRANS_MATRIX = 0x4160  # The Object Matrix

#>------ sub defines of KFDATA
KFDATA_KFHDR = 0xB00A
KFDATA_KFSEG = 0xB008
KFDATA_KFCURTIME = 0xB009
KFDATA_OBJECT_NODE_TAG = 0xB002

#>------ sub defines of OBJECT_NODE_TAG
OBJECT_NODE_ID = 0xB030
OBJECT_NODE_HDR = 0xB010
OBJECT_PIVOT = 0xB013
OBJECT_INSTANCE_NAME = 0xB011
POS_TRACK_TAG = 0xB020
ROT_TRACK_TAG = 0xB021
SCL_TRACK_TAG = 0xB022

import struct

# So 3ds max can open files, limit names to 12 in length
# this is verry annoying for filenames!
name_unique = []  # stores str, ascii only
name_mapping = {}  # stores {orig: byte} mapping

def sane_name(name):
    name_fixed = name_mapping.get(name)
    if name_fixed is not None:
        return name_fixed

    # strip non ascii chars
    new_name_clean = new_name = name.encode("ASCII", "replace").decode("ASCII")[:12]
    i = 0

    while new_name in name_unique:
        new_name = new_name_clean + ".%.3d" % i
        i += 1

    # note, appending the 'str' version.
    name_unique.append(new_name)
    name_mapping[name] = new_name = new_name.encode("ASCII", "replace")
    return new_name

def uv_key(uv):
    return round(uv[0], 6), round(uv[1], 6)

# size defines:
SZ_SHORT = 2
SZ_INT = 4
SZ_FLOAT = 4

class _3ds_ushort(object):
    """Class representing a short (2-byte integer) for a 3ds file.
    *** This looks like an unsigned short H is unsigned from the struct docs - Cam***"""
    __slots__ = ("value", )

    def __init__(self, val=0):
        self.value = val

    def get_size(self):
        return SZ_SHORT

    def write(self, file):
        file.write(struct.pack("<H", self.value))

    def __str__(self):
        return str(self.value)

class _3ds_uint(object):
    """Class representing an int (4-byte integer) for a 3ds file."""
    __slots__ = ("value", )

    def __init__(self, val):
        self.value = val

    def get_size(self):
        return SZ_INT

    def write(self, file):
        file.write(struct.pack("<I", self.value))

    def __str__(self):
        return str(self.value)

class _3ds_float(object):
    """Class representing a 4-byte IEEE floating point number for a 3ds file."""
    __slots__ = ("value", )

    def __init__(self, val):
        self.value = val

    def get_size(self):
        return SZ_FLOAT

    def write(self, file):
        file.write(struct.pack("<f", self.value))

    def __str__(self):
        return str(self.value)

class _3ds_string(object):
    """Class representing a zero-terminated string for a 3ds file."""
    __slots__ = ("value", )

    def __init__(self, val):
        assert(type(val) == bytes)
        self.value = val

    def get_size(self):
        return (len(self.value) + 1)

    def write(self, file):
        binary_format = "<%ds" % (len(self.value) + 1)
        file.write(struct.pack(binary_format, self.value))

    def __str__(self):
        return self.value

class _3ds_point_3d(object):
    """Class representing a three-dimensional point for a 3ds file."""
    __slots__ = "x", "y", "z"

    def __init__(self, point):
        self.x, self.y, self.z = point

    def get_size(self):
        return 3 * SZ_FLOAT

    def write(self, file):
        file.write(struct.pack('<3f', self.x, self.y, self.z))

    def __str__(self):
        return '(%f, %f, %f)' % (self.x, self.y, self.z)

class _3ds_point_uv(object):
    """Class representing a UV-coordinate for a 3ds file."""
    __slots__ = ("uv", )

    def __init__(self, point):
        self.uv = point

    def get_size(self):
        return 2 * SZ_FLOAT

    def write(self, file):
        data = struct.pack('<2f', self.uv[0], self.uv[1])
        file.write(data)

    def __str__(self):
        return '(%g, %g)' % self.uv

class _3ds_face(object):
    """Class representing a face for a 3ds file."""
    __slots__ = ("vindex", )

    def __init__(self, vindex):
        self.vindex = vindex

    def get_size(self):
        return 4 * SZ_SHORT

    # no need to validate every face vert. the oversized array will
    # catch this problem

    def write(self, file):
        # The last zero is only used by 3d studio
        file.write(struct.pack("<4H", self.vindex[0], self.vindex[1], self.vindex[2], 0))

    def __str__(self):
        return "[%d %d %d]" % (self.vindex[0], self.vindex[1], self.vindex[2])

class _3ds_array(object):
    """Class representing an array of variables for a 3ds file.

    Consists of a _3ds_ushort to indicate the number of items, followed by the items themselves.
    """
    __slots__ = "values", "size"

    def __init__(self):
        self.values = []
        self.size = SZ_SHORT

    # add an item:
    def add(self, item):
        self.values.append(item)
        self.size += item.get_size()

    def get_size(self):
        return self.size

    def validate(self):
        return len(self.values) <= 65535

    def write(self, file):
        _3ds_ushort(len(self.values)).write(file)
        for value in self.values:
            value.write(file)

    # To not overwhelm the output in a dump, a _3ds_array only
    # outputs the number of items, not all of the actual items.
    def __str__(self):
        return '(%d items)' % len(self.values)

class _3ds_named_variable(object):
    """Convenience class for named variables."""

    __slots__ = "value", "name"

    def __init__(self, name, val=None):
        self.name = name
        self.value = val

    def get_size(self):
        if self.value is None:
            return 0
        else:
            return self.value.get_size()

    def write(self, file):
        if self.value is not None:
            self.value.write(file)

    def dump(self, indent):
        if self.value is not None:
            print(indent * " ",
                  self.name if self.name else "[unnamed]",
                  " = ",
                  self.value)

#the chunk class
class _3ds_chunk(object):
    """Class representing a chunk in a 3ds file.

    Chunks contain zero or more variables, followed by zero or more subchunks.
    """
    __slots__ = "ID", "size", "variables", "subchunks"

    def __init__(self, chunk_id=0):
        self.ID = _3ds_ushort(chunk_id)
        self.size = _3ds_uint(0)
        self.variables = []
        self.subchunks = []

    def add_variable(self, name, var):
        """Add a named variable.

        The name is mostly for debugging purposes."""
        self.variables.append(_3ds_named_variable(name, var))

    def add_subchunk(self, chunk):
        """Add a subchunk."""
        self.subchunks.append(chunk)

    def get_size(self):
        """Calculate the size of the chunk and return it.

        The sizes of the variables and subchunks are used to determine this chunk\'s size."""
        tmpsize = self.ID.get_size() + self.size.get_size()
        for variable in self.variables:
            tmpsize += variable.get_size()
        for subchunk in self.subchunks:
            tmpsize += subchunk.get_size()
        self.size.value = tmpsize
        return self.size.value

    def validate(self):
        for var in self.variables:
            func = getattr(var.value, "validate", None)
            if (func is not None) and not func():
                return False

        for chunk in self.subchunks:
            func = getattr(chunk, "validate", None)
            if (func is not None) and not func():
                return False

        return True

    def write(self, file):
        """Write the chunk to a file.

        Uses the write function of the variables and the subchunks to do the actual work."""
        #write header
        self.ID.write(file)
        self.size.write(file)
        for variable in self.variables:
            variable.write(file)
        for subchunk in self.subchunks:
            subchunk.write(file)

    def dump(self, indent=0):
        """Write the chunk to a file.

        Dump is used for debugging purposes, to dump the contents of a chunk to the standard output.
        Uses the dump function of the named variables and the subchunks to do the actual work."""
        print(indent * " ",
              "ID=%r" % hex(self.ID.value),
              "size=%r" % self.get_size())
        for variable in self.variables:
            variable.dump(indent + 1)
        for subchunk in self.subchunks:
            subchunk.dump(indent + 1)

######################################################
# EXPORT
######################################################

class tri_wrapper(object):
    """Class representing a triangle.

    Used when converting faces to triangles"""

    __slots__ = "vertex_index", "mat", "image", "faceuvs", "offset"

    def __init__(self, vindex=(0, 0, 0), mat=None, image=None, faceuvs=None):
        self.vertex_index = vindex
        self.mat = mat
        self.image = image
        self.faceuvs = faceuvs
        self.offset = [0, 0, 0]  # offset indices

def extract_triangles(mesh):
    """Extract triangles from a mesh.

    If the mesh contains quads, they will be split into triangles."""
    tri_list = []
    do_uv = bool(mesh.tessface_uv_textures)

    img = None
    for i, face in enumerate(mesh.tessfaces):
        f_v = face.vertices

        uf = mesh.tessface_uv_textures.active.data[i] if do_uv else None

        if do_uv:
            f_uv = uf.uv
            img = uf.image if uf else None
            if img is not None:
                img = img.name

        # if f_v[3] == 0:
        if len(f_v) == 3:
            new_tri = tri_wrapper((f_v[0], f_v[1], f_v[2]), face.material_index, img)
            if (do_uv):
                new_tri.faceuvs = uv_key(f_uv[0]), uv_key(f_uv[1]), uv_key(f_uv[2])
            tri_list.append(new_tri)

        else:  # it's a quad
            new_tri = tri_wrapper((f_v[0], f_v[1], f_v[2]), face.material_index, img)
            new_tri_2 = tri_wrapper((f_v[0], f_v[2], f_v[3]), face.material_index, img)

            if (do_uv):
                new_tri.faceuvs = uv_key(f_uv[0]), uv_key(f_uv[1]), uv_key(f_uv[2])
                new_tri_2.faceuvs = uv_key(f_uv[0]), uv_key(f_uv[2]), uv_key(f_uv[3])

            tri_list.append(new_tri)
            tri_list.append(new_tri_2)

    return tri_list

def remove_face_uv(verts, tri_list):
    """Remove face UV coordinates from a list of triangles.

    Since 3ds files only support one pair of uv coordinates for each vertex, face uv coordinates
    need to be converted to vertex uv coordinates. That means that vertices need to be duplicated when
    there are multiple uv coordinates per vertex."""

    # initialize a list of UniqueLists, one per vertex:
    #uv_list = [UniqueList() for i in xrange(len(verts))]
    unique_uvs = [{} for i in range(len(verts))]

    # for each face uv coordinate, add it to the UniqueList of the vertex
    for tri in tri_list:
        for i in range(3):
            # store the index into the UniqueList for future reference:
            # offset.append(uv_list[tri.vertex_index[i]].add(_3ds_point_uv(tri.faceuvs[i])))

            context_uv_vert = unique_uvs[tri.vertex_index[i]]
            uvkey = tri.faceuvs[i]

            offset_index__uv_3ds = context_uv_vert.get(uvkey)

            if not offset_index__uv_3ds:
                offset_index__uv_3ds = context_uv_vert[uvkey] = len(context_uv_vert), _3ds_point_uv(uvkey)

            tri.offset[i] = offset_index__uv_3ds[0]

    # At this point, each vertex has a UniqueList containing every uv coordinate that is associated with it
    # only once.

    # Now we need to duplicate every vertex as many times as it has uv coordinates and make sure the
    # faces refer to the new face indices:
    vert_index = 0
    vert_array = _3ds_array()
    uv_array = _3ds_array()
    index_list = []
    for i, vert in enumerate(verts):
        index_list.append(vert_index)

        pt = _3ds_point_3d(vert.co)  # reuse, should be ok
        uvmap = [None] * len(unique_uvs[i])
        for ii, uv_3ds in unique_uvs[i].values():
            # add a vertex duplicate to the vertex_array for every uv associated with this vertex:
            vert_array.add(pt)
            # add the uv coordinate to the uv array:
            # This for loop does not give uv's ordered by ii, so we create a new map
            # and add the uv's later
            # uv_array.add(uv_3ds)
            uvmap[ii] = uv_3ds

        # Add the uv's in the correct order
        for uv_3ds in uvmap:
            # add the uv coordinate to the uv array:
            uv_array.add(uv_3ds)

        vert_index += len(unique_uvs[i])

    # Make sure the triangle vertex indices now refer to the new vertex list:
    for tri in tri_list:
        for i in range(3):
            tri.offset[i] += index_list[tri.vertex_index[i]]
        tri.vertex_index = tri.offset

    return vert_array, uv_array, tri_list

def make_faces_chunk(tri_list, mesh):
    """Make a chunk for the faces."""

    face_chunk = _3ds_chunk(OBJECT_FACES)
    face_list = _3ds_array()

    for i, tri in enumerate(tri_list):
        face_list.add(_3ds_face(tri.vertex_index))

    face_chunk.add_variable("faces", face_list)

    return face_chunk

def make_vert_chunk(vert_array):
    """Make a vertex chunk out of an array of vertices."""
    vert_chunk = _3ds_chunk(OBJECT_VERTICES)
    vert_chunk.add_variable("vertices", vert_array)
    return vert_chunk

def make_matrix_4x3_chunk(matrix):
    matrix_chunk = _3ds_chunk(OBJECT_TRANS_MATRIX)
    for vec in matrix.col:
        for f in vec[:3]:
            matrix_chunk.add_variable("matrix_f", _3ds_float(f))
    return matrix_chunk

def make_mesh_chunk(mesh, matrix):
    """Make a chunk out of a Blender mesh."""

    # Extract the triangles from the mesh:
    tri_list = extract_triangles(mesh)

    if mesh.tessface_uv_textures:
        # Remove the face UVs and convert it to vertex UV:
        vert_array, uv_array, tri_list = remove_face_uv(mesh.vertices, tri_list)
    else:
        # Add the vertices to the vertex array:
        vert_array = _3ds_array()
        for vert in mesh.vertices:
            vert_array.add(_3ds_point_3d(vert.co))
        # no UV at all:
        uv_array = None

    # create the chunk:
    mesh_chunk = _3ds_chunk(OBJECT_MESH)

    # add vertex chunk:
    mesh_chunk.add_subchunk(make_vert_chunk(vert_array))

    # add faces chunk:
    mesh_chunk.add_subchunk(make_faces_chunk(tri_list, mesh))

    mesh_chunk.add_subchunk(make_matrix_4x3_chunk(matrix))

    return mesh_chunk

