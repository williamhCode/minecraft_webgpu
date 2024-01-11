#pragma once
#include "gfx/context.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "webgpu/webgpu_cpp.h"
#include "util/frustum.hpp"
#include <array>

struct GameState;

namespace gfx {

class Sun {
public:
  static constexpr int numCascades = 5;

private:
  gfx::Context *m_ctx;
  GameState *m_state;

  std::array<glm::mat4, numCascades> m_projs;
  std::array<glm::mat4, numCascades> m_viewProjs;

  wgpu::Buffer m_sunDirBuffer;
  wgpu::Buffer m_sunViewProjsBuffer;

  bool shouldRender;
  bool shouldUpdate;

  bool shouldRenderFirst;
  bool shouldUpdateFirst;

  static constexpr float minTime = 0.3;  // time for shadow maps to update

  // these two are calculated from riseTurn
  glm::vec3 dir;  // direction is ground to sun
  glm::vec3 up;

  glm::mat4 GetView();
  void UpdateDirAndUp();

public:
  // distance player has to travel for shadow map and chunks to update
  static constexpr int updateDist = 10;
  static constexpr float areaLength = 16 * 40;

  glm::vec2 riseTurn;  // rise and turn of sun angle

  wgpu::BindGroup bindGroup;

  float timeSinceUpdate = 1;

  Sun() = default;
  Sun(gfx::Context *ctx, GameState *state, glm::vec2 riseTurn);

  void InvokeUpdate();
  bool ShouldRender();
  bool ShouldRenderFirst();

  void Update();
  glm::vec3 GetDir() {
    return dir;
  }

  util::Frustum GetFrustum(int cascadeLevel);
};

} // namespace gfx
