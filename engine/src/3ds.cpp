// Copyright (C) 2018 - Sebastien Alaiwan <sebastien.alaiwan@gmail.com>
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

#include <stdint.h>
#include <memory>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>

#include "3ds.h"

using namespace std;

namespace tds
{
struct ByteStream
{
  ByteStream(istream& input) : fp(input)
  {
  }

  size_t tell()
  {
    return fp.tellg();
  }

  bool eof()
  {
    return fp.eof();
  }

  int le(int byteCount)
  {
    if(byteCount > 4 || byteCount < 0)
      throw runtime_error("illegal byte count, must be [0 .. 4]");

    uint8_t dataBuffer[4];
    fp.read((char*)dataBuffer, byteCount);

    if(fp.gcount() != byteCount)
      throw runtime_error("unexpected EOF");

    int r = 0;

    for(int i = 0; i < byteCount; ++i)
      r += (dataBuffer[i] << (8 * i));

    return r;
  }

  int int32()
  {
    return le(4);
  }

  int int16()
  {
    return le(2);
  }

  int uint16()
  {
    auto val = le(2);

    if(val < 0)
      val += (1 << 15);

    return val;
  }

  float float_()
  {
    float data;
    fp.read((char*)&data, sizeof data);
    return data;
  }

  string asciiz()
  {
    string s;

    while(1)
    {
      char c;
      fp.read(&c, 1);

      if(fp.gcount() != 1)
        throw runtime_error("Unexpected end of file found");

      if(c == 0)
        break;

      s += c;
    }

    return s;
  }

  void skipTo(size_t offset)
  {
    if(offset < (size_t)fp.tellg())
      throw runtime_error("Can't seek backwards");

    while((size_t)fp.tellg() < offset && !fp.eof())
    {
      char c;
      fp.read(&c, 1);
    }
  }

  istream& fp;
};

class Parser
{
public:
  struct Listener
  {
    virtual void onVertexXYZ(float /*x*/, float /*y*/, float /*z*/) {};
    virtual void onVertexUV(float /*U*/, float /*V*/) {};
    virtual void on3dFace(int /*A*/, int /*B*/, int /*C*/, uint32_t /*Flags*/) {};
    virtual void onFaceMaterial(string /*Name*/, int /*Number*/) {};
    virtual void onMaterialName(string /*Name*/) {};
    virtual void onObjName(string /*Name*/) {};
    virtual void onTransformMatrix(float /*m*/[4][4]) {};
    virtual void onEndObject() {};
    virtual void onMapFileName(string /*filename*/) {};
  };

  Parser(Listener* listener, ByteStream* stream) :
    m_listener(*listener), m_stream(*stream)
  {
  }

  struct Chunk
  {
    size_t end;
  };

  void parseChunk()
  {
    auto const offset = m_stream.tell();
    auto const type = (TYPE)m_stream.uint16();
    auto const size = m_stream.int32();

    auto const chunk = Chunk {
      offset + size
    };

    if(m_stream.eof())
      return;
    switch(type)
    {
    case TYPE::MAIN:
      parseChunks(chunk);
      break;
    case TYPE::VERSION:
      skipChunk(chunk);
      break;
    case TYPE::OBJMESH:
      parseChunks(chunk);
      break;
    case TYPE::OBJBLOCK:
      readObjBlock();
      parseChunks(chunk);
      m_listener.onEndObject();
      break;
    case TYPE::TRIMESH:
      parseChunks(chunk);
      break;
    case TYPE::VERTLIST:
      readVertList();
      parseChunks(chunk);
      break;
    case TYPE::FACELIST:
      readFaceList();
      parseChunks(chunk);
      break;
    case TYPE::FACEMAT:
      readFaceMat();
      break;
    case TYPE::MAPLIST:
      readMapList();
      parseChunks(chunk);
      break;
    case TYPE::MATERIAL:
      parseChunks(chunk);
      break;
    case TYPE::MATNAME:
      {
        auto const name = m_stream.asciiz();
        m_listener.onMaterialName(name);
      }
      break;
    case TYPE::TEXTURE:
      parseChunks(chunk);
      break;
    case TYPE::MAPFILE:
      readMapFile();
      break;
    case TYPE::TRMATRIX:
      readTransformMatrix();
      break;
    default:
      skipChunk(chunk);
      parseChunks(chunk);
      break;
    }
  }

  enum class TYPE
  {
    VERSION = 0x0002,
    RGBF = 0x0010,
    RGBB = 0x0011,

    PERCENTW = 0x0030,
    PERCENTF = 0x0031,

    PRJ = 0xC23D,
    MLI = 0x3DAA,

    MAIN = 0x4D4D,
    OBJMESH = 0x3D3D,
    ONEUNIT = 0x0100,
    BKGCOLOR = 0x1200,
    AMBCOLOR = 0x2100,
    DEFAULT_VIEW = 0x3000,
    VIEW_TOP = 0x3010,
    VIEW_BOTTOM = 0x3020,
    VIEW_LEFT = 0x3030,
    VIEW_RIGHT = 0x3040,
    VIEW_FRONT = 0x3050,
    VIEW_BACK = 0x3060,
    VIEW_USER = 0x3070,
    VIEW_CAMERA = 0x3080,
    OBJBLOCK = 0x4000,
    TRIMESH = 0x4100,
    VERTLIST = 0x4110,
    VERTFLAGS = 0x4111,
    FACELIST = 0x4120,
    FACEMAT = 0x4130,
    MAPLIST = 0x4140,
    SMOOLIST = 0x4150,
    TRMATRIX = 0x4160,
    MESHCOLOR = 0x4165,
    TXTINFO = 0x4170,
    LIGHT = 0x4600,
    SPOTLIGHT = 0x4610,
    CAMERA = 0x4700,
    HIERARCHY = 0x4F00,

    VIEWPORT_LAYOUT_OLD = 0x7000,
    VIEWPORT_LAYOUT = 0x7001,
    VIEWPORT_DATA_OLD = 0x7010,
    VIEWPORT_DATA = 0x7011,
    VIEWPORT_DATA3 = 0x7012,
    VIEWPORT_SIZE = 0x7020,
    NETWORK_VIEW = 0X7030,

    MATERIAL = 0xAFFF,
    MATNAME = 0xA000,
    AMBIENT = 0xA010,
    DIFFUSE = 0xA020,
    SPECULAR = 0xA030,
    TEXTURE = 0xA200,
    BUMPMAP = 0xA230,
    MAPFILE = 0xA300,
    KEYFRAMER = 0xB000,
    AMBIENTKEY = 0xB001,
    TRACKINFO = 0xB002,
    TRACKOBJNAME = 0xB010,
    TRACKPIVOT = 0xB013,
    TRACKPOS = 0xB020,
    TRACKROTATE = 0xB021,
    TRACKSCALE = 0xB022,
    TRACKMORPH = 0xB026,
    TRACKHIDE = 0xB029,
    OBJNUMBER = 0xB030,
    TRACKCAMERA = 0xB003,
    TRACKFOV = 0xB023,
    TRACKROLL = 0xB024,
    TRACKCAMTGT = 0xB004,
    TRACKLIGHT = 0xB005,
    TRACKLIGTGT = 0xB006,
    TRACKSPOTL = 0xB007,
    FRAMES = 0xB008
  };

private:
  void parseChunks(Chunk parent)
  {
    while(m_stream.tell() < parent.end)
      parseChunk();
  }

  void readObjBlock()
  {
    string ObjName = m_stream.asciiz();

    m_listener.onObjName(ObjName);
  }

  void readVertList()
  {
    auto const vertexCount = m_stream.int16();

    for(int i = 0; i < vertexCount; ++i)
    {
      auto const x = m_stream.float_();
      auto const y = m_stream.float_();
      auto const z = m_stream.float_();

      m_listener.onVertexXYZ(x, y, z);
    }
  }

  void readFaceList()
  {
    auto const numFaces = m_stream.uint16();

    for(int i = 0; i < numFaces; ++i)
    {
      auto const c1 = m_stream.int16();
      auto const c2 = m_stream.int16();
      auto const c3 = m_stream.int16();
      auto const c4 = m_stream.int16();

      m_listener.on3dFace(c1, c2, c3, c4);
    }
  }

  void readMapList()
  {
    auto const numVertices = m_stream.int16();

    for(int i = 0; i < numVertices; ++i)
    {
      auto const u = m_stream.float_();
      auto const v = m_stream.float_();

      m_listener.onVertexUV(u, v);
    }
  }

  void readFaceMat()
  {
    auto const matName = m_stream.asciiz();

    auto const numFaces = m_stream.int16();

    for(int i = 0; i < numFaces; ++i)
    {
      auto const iFace = m_stream.int16();
      m_listener.onFaceMaterial(matName, iFace);
    }
  }

  void readMapFile()
  {
    auto const mapFileName = m_stream.asciiz();
    m_listener.onMapFileName(mapFileName);
  }

  void readTransformMatrix()
  {
    float transform[4][4];

    for(int j = 0; j < 4; j++)
      for(int i = 0; i < 3; i++)
        transform[j][i] = m_stream.float_();

    transform[0][3] = 0;
    transform[1][3] = 0;
    transform[2][3] = 0;
    transform[3][3] = 1;

    m_listener.onTransformMatrix(transform);
  }

  void skipChunk(Chunk chunk)
  {
    m_stream.skipTo(chunk.end);
  }

  Listener& m_listener;
  ByteStream& m_stream;
};

struct MeshLoader : Parser::Listener
{
  MeshLoader(std::vector<Mesh>& meshes_) : meshes(meshes_)
  {
  }

  void onVertexXYZ(float x, float y, float z) override
  {
    m_Mesh.vertices.push_back({ x, y, z, 0, 0 });
  }

  void on3dFace(int A, int B, int C, uint32_t /*Flags*/) override
  {
    m_Mesh.faces.push_back({ A, B, C });
  }

  void onVertexUV(float U, float V) override
  {
    m_Mesh.vertices[m_idx].u = U;
    m_Mesh.vertices[m_idx].v = V;
    m_idx++;
  }

  void onObjName(string Name) override
  {
    m_Mesh.name = Name;
  }

  void onEndObject() override
  {
    meshes.push_back(m_Mesh);
    m_Mesh = Mesh();
    m_idx = 0;
  }

private:
  Mesh m_Mesh;
  std::vector<Mesh>& meshes;
  int m_idx = 0;
};

static
std::vector<Mesh> load(istream& fp)
{
  auto bs = ByteStream(fp);

  std::vector<Mesh> meshes;
  MeshLoader loader(meshes);
  Parser parser(&loader, &bs);
  parser.parseChunk();
  return meshes;
}

std::vector<Mesh> load(Span<uint8_t const> buffer)
{
  auto const s = string(buffer.data, buffer.data + buffer.len);
  stringstream mem(s);

  return load(mem);
}

std::vector<Mesh> load(string filename)
{
  auto fp = ifstream(filename, std::ios::binary);

  if(!fp.is_open())
    throw runtime_error("Can't open file '" + filename + "'");

  return load(fp);
}
}

std::vector<Mesh> loadMesh(char const* path)
{
  return tds::load(path);
}

