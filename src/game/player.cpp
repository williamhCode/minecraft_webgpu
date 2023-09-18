#include "player.hpp"

namespace game {

Player::Player(util::Camera &camera) : camera(camera) {
}

void Player::Look(glm::vec2 delta) {
  camera.orientation += glm::vec3(0, delta.y, -delta.x);
  float pi_4 = glm::radians(89.9f);
  camera.orientation.y = std::max(-pi_4, std::min(camera.orientation.y, pi_4));
}

void Player::Move(glm::vec3 moveDir) {
  moveDir *= speed;
  moveDir = glm::rotateZ(moveDir, camera.orientation.z);
  camera.position += moveDir;
}

void Player::Update() {
  camera.Update();
}

glm::vec3 Player::GetPosition() {
  return camera.position;
}

glm::vec3 Player::GetDirection() {
  return camera.direction;
}

} // namespace game
