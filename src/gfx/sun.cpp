#include "sun.hpp"
#include "game.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float2.hpp"
#include "util/webgpu-util.hpp"
#include <iostream>

namespace gfx {

Sun::Sun(gfx::Context *ctx, GameState *state, glm::vec3 dir)
    : m_ctx(ctx), m_state(state), m_dir(dir) {
  m_sunDirBuffer = util::CreateUniformBuffer(m_ctx->device, sizeof(glm::vec3), &dir);

  const glm::vec2 depth_range(1, 1024);
  m_proj = glm::ortho(-area, area, -area, area, depth_range.x, depth_range.y);

  m_sunViewProjBuffer = util::CreateUniformBuffer(m_ctx->device, sizeof(glm::mat4));

  bindGroup = dawn::utils::MakeBindGroup(
    ctx->device, ctx->pipeline.sunBGL,
    {
      {0, m_sunDirBuffer},
      {1, m_sunViewProjBuffer},
    }
  );

  TryUpdate();
}

glm::mat4 Sun::MakeView() {
  const float distance = 512;

  auto centerPos = m_state->player.GetPosition();
  centerPos.z = 100;
  auto eyePos = centerPos + m_dir * distance;
  auto view = glm::lookAt(eyePos, centerPos, m_state->player.camera.up);

  // center the shadow map
  auto viewProj = m_proj * view;
  auto centerView = viewProj * glm::vec4(centerPos, 1.0);
  auto centerFixed = glm::vec3(centerView.xy() - 0.5f, centerView.z);
  auto centerFixedW = (glm::inverse(viewProj) * glm::vec4(centerFixed, 1.0)).xyz();

  eyePos = centerFixedW + m_dir * distance;
  return glm::lookAt(eyePos, centerFixedW, m_state->player.camera.up);
}

void Sun::TryUpdate() {
  if (timeSinceUpdate < minTime) return;

  auto viewProj = m_proj * MakeView();
  m_ctx->queue.WriteBuffer(m_sunViewProjBuffer, 0, &viewProj, sizeof(viewProj));

  timeSinceUpdate = 0;
  shouldRender = true;
}

bool Sun::ShouldRender() {
  if (shouldRender) {
    shouldRender = false;
    return true;
  }
  return false;
}

void Sun::Update() {
  timeSinceUpdate += m_state->dt;
}

void Sun::SetDir(glm::vec3 dir) {
  m_dir = dir;
  m_ctx->queue.WriteBuffer(m_sunDirBuffer, 0, &m_dir, sizeof(m_dir));
}

} // namespace gfx
