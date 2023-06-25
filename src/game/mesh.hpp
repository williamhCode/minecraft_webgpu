#pragma once
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include <webgpu/webgpu_glfw.h>
#include <array>

namespace game {

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;
  glm::vec2 texLoc;
};

struct Face {
  std::array<Vertex, 4> vertices;
};

struct Cube{
  std::array<Face, 6> faces;
};

struct FaceIndex {
  std::array<uint32_t, 6> indices;
};

extern std::array<Face, 6> g_MESH_FACES;
extern const std::array<uint32_t, 6> g_FACE_INDICES;

void InitMesh();

}
