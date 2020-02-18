#include <stdio.h>

#include "base/mesh.h"
#include "render/rendermesh.h"

namespace
{
bool startsWith(string s, string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

RenderMesh convertToRenderMesh(vector<Mesh> const& meshes)
{
  RenderMesh r;

  static auto convert = [] (Mesh::Vertex const& src)
    {
      RenderMesh::Vertex r;
      r.x = src.x;
      r.y = src.y;
      r.z = src.z;
      r.nx = src.nx;
      r.ny = src.ny;
      r.nz = src.nz;
      r.diffuse_u = src.u;
      r.diffuse_v = src.v;
      return r;
    };

  for(auto& mesh : meshes)
  {
    if(startsWith(mesh.name, "f."))
      continue;

    for(auto& face : mesh.faces)
    {
      r.vertices.push_back(convert(mesh.vertices[face.i1]));
      r.vertices.push_back(convert(mesh.vertices[face.i2]));
      r.vertices.push_back(convert(mesh.vertices[face.i3]));
    }
  }

  return r;
}
}

int main(int argc, const char* argv[])
{
  if(argc != 4)
    return 1;

  const auto input = argv[1];
  const auto outputPathMesh = argv[2];
  const auto outputPathLightmap = argv[3];

  auto mesh = loadMesh(input);

  auto renderMesh = convertToRenderMesh(mesh);

  {
    FILE* fp = fopen(outputPathMesh, "wb");

    for(auto& vertex : renderMesh.vertices)
    {
      fprintf(fp, "%.6f %.6f %.6f - %.6f %.6f %.6f - %.6f %.6f - %.6f %.6f\n",
              vertex.x,
              vertex.y,
              vertex.z,
              vertex.nx,
              vertex.ny,
              vertex.nz,
              vertex.diffuse_u,
              vertex.diffuse_v,
              vertex.lightmap_u,
              vertex.lightmap_v
              );
    }

    fclose(fp);
  }

  {
    static uint8_t gray_png[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x08, 0x06, 0x00, 0x00, 0x00, 0xc4, 0x0f, 0xbe, 0x8b, 0x00, 0x00, 0x00, 0x16, 0x49, 0x44, 0x41, 0x54, 0x18, 0xd3, 0x63, 0x6c, 0x68, 0x68, 0xf8, 0xcf, 0x80, 0x07, 0x30, 0x31, 0x10, 0x00, 0xc3, 0x43, 0x01, 0x00, 0x95, 0x62, 0x02, 0x8f, 0x72, 0x61, 0x0a, 0x14, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };
    FILE* fp = fopen(outputPathLightmap, "wb");
    fwrite(gray_png, 1, sizeof gray_png, fp);
    fclose(fp);
  }

  return 0;
}

