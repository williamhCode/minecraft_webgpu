#pragma once
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include <webgpu/webgpu_glfw.h>
#include <array>

namespace game {

struct Vertex {
  glm::ivec3 position;
  glm::ivec3 normal;
  glm::uvec2 uv;
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

extern Cube g_CUBE;
extern const std::array<uint32_t, 6> g_FACE_INDICES;

void InitMesh();

}
