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
  static constexpr u_int32_t numCascades = 5;

private:
  gfx::Context *m_ctx;
  GameState *m_state;

  glm::vec3 m_dir;  // direction is ground to sun
  std::array<glm::mat4, numCascades> m_projs;
  std::array<glm::mat4, numCascades> m_views;
  std::array<glm::mat4, numCascades> m_viewProjs;

  wgpu::Buffer m_sunDirBuffer;
  wgpu::Buffer m_sunViewProjsBuffer;

  bool shouldRender = false;
  bool shouldUpdate = false;
  float minTime = 0.2;

  void UpdateViews();

public:
  static constexpr float areaLength = 16 * 160;

  wgpu::BindGroup bindGroup;

  float timeSinceUpdate = 1;

  Sun() = default;
  Sun(gfx::Context *ctx, GameState *state, glm::vec3 dir);

  void InvokeUpdate();
  bool ShouldRender();

  void Update();
  glm::vec3 GetDir() {
    return m_dir;
  }
  void SetDir(glm::vec3 dir);

  util::Frustum GetFrustum(int cascadeLevel);
};

} // namespace gfx
