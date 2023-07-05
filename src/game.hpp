#pragma once

#include <GLFW/glfw3.h>
#include "game/chunk_manager.hpp"
#include "glm-include.hpp"

#include "game/player.hpp"
#include "util/context.hpp"

struct GameState {
  game::Player player;
  std::unique_ptr<game::ChunkManager> chunkManager;
};

class Game {
private:
  GLFWwindow *m_window;
  glm::uvec2 m_size;
  glm::uvec2 m_FBSize;

  util::Context m_ctx;
  GameState m_state;

  glm::vec2 m_lastMousePos;
  float m_dt;

public:
  Game();
  ~Game();
  void KeyCallback(int key, int scancode, int action, int mods);
  void MouseButtonCallback(int button, int action, int mods);
  void CursorPosCallback(double xpos, double ypos);
  void Run();
  bool KeyPressed(int key);
};
