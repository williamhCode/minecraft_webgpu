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
  static constexpr float m_area = 16 * 10;

  gfx::Context *m_ctx;
  GameState *m_state;

  glm::vec3 m_dir;  // player -> sun
  glm::mat4 m_proj;

  wgpu::Buffer m_sunDirBuffer;
  wgpu::Buffer m_sunViewProjBuffer;

  glm::mat4 MakeView();

public:
  wgpu::BindGroup bindGroup;

  Sun() = default;
  Sun(gfx::Context *ctx, GameState *state, glm::vec3 dir);
  void Update();
  glm::vec3 GetDir() {
    return m_dir;
  }
  void SetDir(glm::vec3 dir);
};

} // namespace gfx
