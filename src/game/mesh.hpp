#pragma once
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include <array>

namespace game {

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texCoord;
};

struct Face {
  std::array<Vertex, 4> vertices;
};

struct FaceIndex {
  std::array<uint16_t, 6> indices;
};

extern std::array<Face, 6> g_MESH_FACES;
extern const std::array<uint16_t, 6> g_FACE_INDICES;

void InitFaces();

}
