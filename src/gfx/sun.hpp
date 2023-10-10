#pragma once
#include "gfx/context.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "webgpu/webgpu_cpp.h"
#include <optional>

struct GameState;

namespace gfx {

class Sun {
private:
  gfx::Context *m_ctx;
  GameState *m_state;

  glm::vec3 m_dir;  // player -> sun
  glm::mat4 m_proj;

  wgpu::Buffer m_sunDirBuffer;
  wgpu::Buffer m_sunViewProjBuffer;

  bool shouldRender = false;
  float minTime = 0.3;

  glm::mat4 MakeView();

public:
  static constexpr float area = 16 * 40;
  wgpu::BindGroup bindGroup;

  float timeSinceUpdate = 1;

  Sun() = default;
  Sun(gfx::Context *ctx, GameState *state, glm::vec3 dir);

  void TryUpdate();
  bool ShouldRender();

  void Update();
  glm::vec3 GetDir() {
    return m_dir;
  }
  void SetDir(glm::vec3 dir);
};

} // namespace gfx
