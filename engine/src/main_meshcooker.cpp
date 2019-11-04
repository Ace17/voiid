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
  (void)outputPathLightmap;

  auto mesh = loadMesh(input);

  auto renderMesh = convertToRenderMesh(mesh);

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

  return 0;
}

