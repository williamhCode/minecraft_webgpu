#pragma once

#include <GLFW/glfw3.h>
#include "game/block.hpp"
#include "game/chunk_manager.hpp"
#include "gfx/sun.hpp"

#include "game/player.hpp"
#include "gfx/context.hpp"


struct GameState {
  glm::uvec2 size;
  glm::uvec2 fb_size;

  bool focused = true;

  bool showStats = false;
  float dt;
  float fps;

  game::Player player;
  game::ChunkManager chunkManager;

  gfx::Sun sun;

  game::BlockId currBlock;
};

class Game {
private:
  GLFWwindow *m_window;

  gfx::Context m_ctx;
  GameState m_state;

  glm::vec2 m_lastMousePos;

public:
  Game();
  ~Game();
  void KeyCallback(int key, int scancode, int action, int mods);
  void MouseButtonCallback(int button, int action, int mods);
  void CursorPosCallback(double xpos, double ypos);
  bool KeyPressed(int key);
};
