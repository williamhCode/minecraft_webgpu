#include "sun.hpp"
#include "game.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float2.hpp"
#include "util/webgpu-util.hpp"

namespace gfx {

Sun::Sun(gfx::Context *ctx, GameState *state, glm::vec3 dir)
    : m_ctx(ctx), m_state(state), m_dir(dir) {
  m_sunDirBuffer = util::CreateUniformBuffer(m_ctx->device, sizeof(glm::vec3), &dir);

  const glm::vec2 depth_range(0, 512);
  m_proj = glm::ortho(-m_area, m_area, -m_area, m_area, depth_range.x, depth_range.y);

  auto viewProj = m_proj * MakeView();
  m_sunViewProjBuffer =
    util::CreateUniformBuffer(m_ctx->device, sizeof(glm::mat4), &viewProj);

  bindGroup = dawn::utils::MakeBindGroup(
    ctx->device, ctx->pipeline.sunBGL,
    {
      {0, m_sunDirBuffer},
      {1, m_sunViewProjBuffer},
    }
  );
}

glm::mat4 Sun::MakeView() {
  const float distance = 64;

  // auto offset = glm::vec3(glm::vec2(-m_dir) * distance, 0.0);
  // auto center = m_state->player.GetPosition() - offset;
  auto center = m_state->player.GetPosition();
  center.z = 100;

  return glm::lookAt(center + m_dir * distance, center, m_state->player.camera.up);
}

void Sun::Update() {
  auto viewProj = m_proj * MakeView();
  m_ctx->queue.WriteBuffer(m_sunViewProjBuffer, 0, &viewProj, sizeof(viewProj));
}

void Sun::SetDir(glm::vec3 dir) {
  m_dir = dir;
  m_ctx->queue.WriteBuffer(m_sunDirBuffer, 0, &m_dir, sizeof(m_dir));
}

} // namespace gfx
