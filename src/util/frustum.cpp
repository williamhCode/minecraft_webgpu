#include "frustum.hpp"

namespace util {

Frustum::Frustum(const glm::mat4 &mat) {
  auto viewProj = glm::transpose(mat);
  // left, right, bottom, top, near, far
  planes[0] = viewProj[3] + viewProj[1];
  planes[1] = viewProj[3] - viewProj[1];
  planes[2] = viewProj[3] + viewProj[2];
  planes[3] = viewProj[3] - viewProj[2];
  planes[4] = viewProj[3] + viewProj[0];
  planes[5] = viewProj[3] - viewProj[0];

  // Normalize the plane equations
  for (auto &plane : planes) {
    plane = glm::normalize(plane);
  }
}

bool Frustum::Intersects(const AABB &aabb) {
  for (const auto &plane : planes) {
    // Calculate the most positive corner (in the direction of the plane normal)
    glm::vec3 positiveVertex = aabb.min;
    if (plane.x >= 0)
      positiveVertex.x = aabb.max.x;
    if (plane.y >= 0)
      positiveVertex.y = aabb.max.y;
    if (plane.z >= 0)
      positiveVertex.z = aabb.max.z;

    // If the most positive corner is on the outside of the plane, the AABB is outside
    // the frustum
    if (glm::dot(glm::vec3(plane), positiveVertex) + plane.w < 0)
      return false;
  }

  // If we haven't found an outside plane, the AABB is inside (or intersects) the
  // frustum
  return true;
}

} // namespace util
