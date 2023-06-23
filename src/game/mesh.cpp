#include "mesh.hpp"

namespace game {

// clang-format off
const glm::vec3 CUBE_VERTICES[] = {
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

const glm::vec3 NORMALS[] = {
  {0,  1,  0},
  {0, -1,  0},
  {1,  0,  0},
  {-1, 0,  0},
  {0,  0,  1},
  {0,  0, -1},
};

const glm::vec2 TEX_COORDS[] = {
  {0, 1},
  {1, 1},
  {0, 0},
  {1, 0},
};
// clang-format on

std::array<Face, 6> g_MESH_FACES;
const std::array<uint32_t, 6> g_FACE_INDICES = {0, 1, 3, 2, 3, 1};

void InitFaces() {
  for (size_t i = 0; i < g_MESH_FACES.size(); i++) {
    Face &face = g_MESH_FACES[i];
    glm::vec3 normal = NORMALS[i];

    for (size_t j = 0; j < face.vertices.size(); j++) {
      Vertex &vertex = face.vertices[j];
      vertex.position = CUBE_VERTICES[CUBE_INDICES[i][j]];
      vertex.normal = normal;
      vertex.texCoord = TEX_COORDS[j];
    }
  }
}

} // namespace game
