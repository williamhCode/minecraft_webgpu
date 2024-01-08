#include "sun.hpp"

#include "dawn/utils/WGPUHelpers.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/gtx/transform.hpp"

#include "game.hpp"
#include "util/webgpu-util.hpp"

namespace gfx {

Sun::Sun(gfx::Context *ctx, GameState *state, glm::vec2 riseTurn)
    : m_ctx(ctx), m_state(state), riseTurn(riseTurn) {
  UpdateDirAndUp();
  m_sunDirBuffer = util::CreateUniformBuffer(m_ctx->device, sizeof(glm::vec3), &dir);

  glm::vec2 depth_range(1, 1024);
  auto halfLength = areaLength / 2;
  for (size_t i = 0; i < numCascades; i++) {
    m_projs[i] = glm::ortho<float>(
      -halfLength, halfLength, -halfLength, halfLength, depth_range.x, depth_range.y
    );
    halfLength /= 2;
  }

  m_sunViewProjsBuffer =
    util::CreateStorageBuffer(m_ctx->device, sizeof(glm::mat4) * numCascades);

  auto numCascadesBuffer =
    util::CreateUniformBuffer(m_ctx->device, sizeof(numCascades), &numCascades);

  bindGroup = dawn::utils::MakeBindGroup(
    ctx->device, ctx->pipeline.sunBGL,
    {
      {0, m_sunDirBuffer},
      {1, m_sunViewProjsBuffer},
      {2, numCascadesBuffer},
    }
  );

  InvokeUpdate();
}

glm::mat4 Sun::GetView() {
  const float distance = 512;

  auto centerPos = m_state->player.GetPosition();
  centerPos.z = 100;
  auto eyePos = centerPos + dir * distance;

  auto view = glm::lookAt(eyePos, centerPos, up);
  return view;
}

void Sun::UpdateDirAndUp() {
  auto riseMat = glm::rotate(glm::radians(-riseTurn.x), glm::vec3(0, 1, 0));
  auto turnMat = glm::rotate(glm::radians(riseTurn.y), glm::vec3(0, 0, 1));
  auto transformMat = turnMat * riseMat;
  dir = transformMat * glm::vec4(1, 0, 0, 1);
  up = transformMat * glm::vec4(0, 0, 1, 1);
}

void Sun::InvokeUpdate() {
  shouldUpdate = true;
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

  if (timeSinceUpdate > minTime && shouldUpdate) {
    UpdateDirAndUp();
    m_ctx->queue.WriteBuffer(m_sunDirBuffer, 0, &dir, sizeof(dir));
    auto view = GetView();
    for (size_t i = 0; i < numCascades; i++) {
      m_viewProjs[i] = m_projs[i] * view;
      auto stride = sizeof(glm::mat4);
      m_ctx->queue.WriteBuffer(
        m_sunViewProjsBuffer, stride * i, &m_viewProjs[i], stride
      );
    }

    timeSinceUpdate = 0;
    shouldUpdate = false;

    shouldRender = true;
  }
}

util::Frustum Sun::GetFrustum(int cascadeLevel) {
  return util::Frustum(m_viewProjs[cascadeLevel]);
}

} // namespace gfx
