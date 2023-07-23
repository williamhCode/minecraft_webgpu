#pragma once

#include "util/camera.hpp"

namespace game {

class Player {
private:
  static constexpr float m_SPEED = 100;

public:
  util::Camera camera;

  Player() = default;
  Player(util::Camera &camera);
  void Look(glm::vec2 delta);
  void Move(glm::vec3 moveDir);
  void Update();
  glm::vec3 GetPosition();
  glm::vec3 GetDirection();
};

} // namespace game
