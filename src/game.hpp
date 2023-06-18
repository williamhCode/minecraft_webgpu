#pragma once

#include <GLFW/glfw3.h>
#include "glm-include.hpp"

#include "util/camera.hpp"
#include "util/handle.hpp"

class Game {
private:
  GLFWwindow *m_window;
  int m_width;
  int m_height;
  int m_FBWidth;
  int m_FBHeight;
  float m_dt;

  util::Handle m_handle;

  util::Camera m_camera;
  glm::vec2 m_lastMousePos;

public:
  Game();
  ~Game();
  void keyCallback(int key, int scancode, int action, int mods);
  void cursorPosCallback(double xpos, double ypos);
  void run();
};
