#include <map>
#include <string.h> // memcpy

#include "base/mesh.h"
#include "base/span.h"
#include "base/util.h" // setExtension
#include "misc/file.h" // exists

#include "rendermesh.h"

namespace
{
bool startsWith(string s, string prefix)
{
  return s.substr(0, prefix.size()) == prefix;
}

RenderMesh convertToRenderMesh(vector<Mesh> const& meshes, vector<string>& textures)
{
  RenderMesh r;

  static auto convert = [] (Mesh::Vertex const& src)
    {
      SingleRenderMesh::Vertex r;
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

  std::map<std::string, SingleRenderMesh> singlesByMaterial;

  for(auto& mesh : meshes)
  {
    if(startsWith(mesh.name, "f."))
      continue;

    if(mesh.material == "invisible" || mesh.material.empty())
      continue;

    SingleRenderMesh& single = singlesByMaterial[mesh.material];

    for(auto& face : mesh.faces)
    {
      single.vertices.push_back(convert(mesh.vertices[face.i1]));
      single.vertices.push_back(convert(mesh.vertices[face.i2]));
      single.vertices.push_back(convert(mesh.vertices[face.i3]));
    }
  }

  for(auto& single : singlesByMaterial)
  {
    textures.push_back(single.first);

    if(single.second.vertices.empty())
      continue;

    r.singleMeshes.push_back(single.second);
  }

  return r;
}

void writeRenderMesh(string path, const RenderMesh& renderMesh)
{
  std::vector<uint8_t> data;

  auto write = [&] (const void* ptr, size_t size)
    {
      const auto i = data.size();
      data.resize(i + size);
      memcpy(&data[i], ptr, size);
    };

  for(auto& single : renderMesh.singleMeshes)
  {
    const int num = (int)single.vertices.size();
    write(&num, 4);

    for(auto& vertex : single.vertices)
      write(&vertex, sizeof vertex);
  }

  File::write(path, data);
}
}

int main(int argc, const char* argv[])
{
  if(argc != 4)
    return 1;

  const auto input = string(argv[1]);
  const auto textureDir = string(argv[2]);
  const auto outputPathMesh = argv[3];

  auto importedMesh = importMesh(input);

  std::vector<string> textureFiles;
  auto renderMesh = convertToRenderMesh(importedMesh.meshes, textureFiles);
  writeRenderMesh(outputPathMesh, renderMesh);

  int meshIndex = 0;

  for(auto& single : renderMesh.singleMeshes)
  {
    static uint8_t gray_png[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x08, 0x06, 0x00, 0x00, 0x00, 0xc4, 0x0f, 0xbe, 0x8b, 0x00, 0x00, 0x00, 0x16, 0x49, 0x44, 0x41, 0x54, 0x18, 0xd3, 0x63, 0x6c, 0x68, 0x68, 0xf8, 0xcf, 0x80, 0x07, 0x30, 0x31, 0x10, 0x00, 0xc3, 0x43, 0x01, 0x00, 0x95, 0x62, 0x02, 0x8f, 0x72, 0x61, 0x0a, 0x14, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };

    {
      auto outputPathLightmap = setExtension(outputPathMesh, to_string(meshIndex) + ".lightmap.png");
      File::write(outputPathLightmap, gray_png);
    }

    {
      auto const outputPathDiffuse = setExtension(outputPathMesh, to_string(meshIndex) + ".diffuse.png");

      if(textureFiles[meshIndex] == "")
        textureFiles[meshIndex] = "mesh.png";

      auto const inputPathDiffuse = textureDir + "/" + textureFiles[meshIndex];

      if(File::exists(inputPathDiffuse))
      {
        auto diffusePngData = File::read(inputPathDiffuse);
        File::write(outputPathDiffuse, { (uint8_t*)diffusePngData.data(), (int)diffusePngData.size() });
      }
      else
      {
        File::write(outputPathDiffuse, gray_png);
      }
    }

    ++meshIndex;
  }

  return 0;
}

