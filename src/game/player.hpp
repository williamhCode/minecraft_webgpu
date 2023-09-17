#pragma once

#include "util/camera.hpp"

namespace game {

class Player {
public:
  util::Camera camera;
  float speed = 10;

  Player() = default;
  Player(util::Camera &camera);
  void Look(glm::vec2 delta);
  void Move(glm::vec3 moveDir);
  void Update();
  glm::vec3 GetPosition();
  glm::vec3 GetDirection();
};

} // namespace game
