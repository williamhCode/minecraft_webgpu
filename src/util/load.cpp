#include "load.hpp"
#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <vector>

using namespace wgpu;

namespace util {

std::vector<ModelVertex> LoadObj(const std::string &path) {
  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(path)) {
    if (!reader.Error().empty()) {
      std::cerr << "TinyObjReader: " << reader.Error() << std::endl;
    }
    std::exit(1);
  }

  if (!reader.Warning().empty()) {
    std::cout << "TinyObjReader: " << reader.Warning() << std::endl;
  }

  auto &attrib = reader.GetAttrib();
  auto &shapes = reader.GetShapes();
  // auto &materials = reader.GetMaterials();

  std::vector<ModelVertex> vertexData;

  for (const auto &shape : shapes) {
    size_t offset = vertexData.size();
    vertexData.resize(offset + shape.mesh.indices.size());

    for (size_t i = 0; i < vertexData.size(); ++i) {
      const tinyobj::index_t &idx = shape.mesh.indices[i];

      vertexData[offset + i].position = {
        attrib.vertices[3 * idx.vertex_index + 0],
        -attrib.vertices[3 * idx.vertex_index + 2],
        attrib.vertices[3 * idx.vertex_index + 1],
      };

      vertexData[offset + i].normal = {
        attrib.normals[3 * idx.normal_index + 0],
        -attrib.normals[3 * idx.normal_index + 2],
        attrib.normals[3 * idx.normal_index + 1],
      };

      vertexData[offset + i].color = {
        attrib.colors[3 * idx.vertex_index + 0],
        attrib.colors[3 * idx.vertex_index + 1],
        attrib.colors[3 * idx.vertex_index + 2],
      };

      vertexData[offset + i].uv = {
        attrib.texcoords[2 * idx.texcoord_index + 0],
        1 - attrib.texcoords[2 * idx.texcoord_index + 1],
      };
    }
  }

  return vertexData;
}

} // namespace util
