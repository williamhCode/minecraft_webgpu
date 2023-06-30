#include "player.hpp"

namespace game {

Player::Player(util::Camera &camera) : camera(camera) {
}

void Player::Look(glm::vec2 delta) {
  camera.orientation += glm::vec3(-delta.y, 0, -delta.x);
  float pi_4 = glm::radians(90.0f);
  camera.orientation.x = std::max(-pi_4, std::min(camera.orientation.x, pi_4));
}

void Player::Move(glm::vec3 moveDir) {
  moveDir *= m_SPEED;
  moveDir = glm::rotateZ(moveDir, camera.orientation.z);
  camera.position += moveDir;
}

void Player::Update() {
  camera.Update();
}

glm::vec3 Player::GetPosition() {
  return camera.position;
}

} // namespace game
