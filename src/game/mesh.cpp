#include "mesh.hpp"

namespace game {

// clang-format off
const glm::ivec3 CUBE_VERTICES[] = {
  {0, 0, 0},
  {1, 0, 0},
  {1, 1, 0},
  {0, 1, 0},
  {0, 0, 1},
  {1, 0, 1},
  {1, 1, 1},
  {0, 1, 1},
};

const uint32_t CUBE_INDICES[6][4] = {
  {2, 3, 7, 6}, // north (+y)
  {0, 1, 5, 4}, // south (-y)
  {1, 2, 6, 5}, // east (+x)
  {3, 0, 4, 7}, // west (-x)
  {4, 5, 6, 7}, // top (+z)
  {3, 2, 1, 0}, // bottom (-z)
};

const glm::ivec3 NORMALS[] = {
  {0,  1,  0},
  {0, -1,  0},
  {1,  0,  0},
  {-1, 0,  0},
  {0,  0,  1},
  {0,  0, -1},
};

const glm::uvec2 TEX_COORDS[] = {
  {0, 1},
  {1, 1},
  {1, 0},
  {0, 0},
};
// clang-format on

Cube g_CUBE;
const std::array<uint32_t, 6> g_FACE_INDICES = {0, 1, 2, 0, 2, 3};

void InitMesh() {
  for (size_t i = 0; i < g_CUBE.faces.size(); i++) {
    Face &face = g_CUBE.faces[i];
    glm::ivec3 normal = NORMALS[i];

    for (size_t j = 0; j < face.vertices.size(); j++) {
      Vertex &vertex = face.vertices[j];
      vertex.position = CUBE_VERTICES[CUBE_INDICES[i][j]];
      vertex.normal = normal;
      vertex.uv = TEX_COORDS[j];
    }
  }
}

} // namespace game
