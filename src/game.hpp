#pragma once

#include <GLFW/glfw3.h>
#include "game/block.hpp"
#include "game/chunk_manager.hpp"
#include "glm-include.hpp"

#include "game/player.hpp"
#include "util/context.hpp"
#include "gfx/renderer.hpp"

struct GameState {
  glm::uvec2 size;
  glm::uvec2 fbSize;

  glm::vec3 sunDir;

  bool focused = true;

  bool showStats = false;
  float dt;
  float fps;

  game::Player player;
  std::unique_ptr<game::ChunkManager> chunkManager;

  game::BlockId currBlock;
};

class Game {
private:
  GLFWwindow *m_window;

  util::Context m_ctx;
  GameState m_state;

  glm::vec2 m_lastMousePos;

public:
  Game();
  ~Game();
  void KeyCallback(int key, int scancode, int action, int mods);
  void MouseButtonCallback(int button, int action, int mods);
  void CursorPosCallback(double xpos, double ypos);
  void Run();
  bool KeyPressed(int key);
};
