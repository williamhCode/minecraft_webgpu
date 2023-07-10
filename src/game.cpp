#include "game.hpp"

#include <numeric>
#include <tuple>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "GLFW/glfw3.h"
#include "game/block.hpp"
#include "game/chunk.hpp"
#include "game/chunk_manager.hpp"
#include "game/direction.hpp"
#include "game/mesh.hpp"
#include "game/ray.hpp"
#include "glm/gtx/string_cast.hpp"
#include "util/context.hpp"
#include "util/renderer.hpp"
#include "util/timer.hpp"
#include "util/webgpu-util.hpp"
#include "util/load.hpp"
#include "util/pipeline.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace wgpu;

void _KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  Game *game = reinterpret_cast<Game *>(glfwGetWindowUserPointer(window));
  game->KeyCallback(key, scancode, action, mods);
}

void _MouseButtonCallback(GLFWwindow *window, int button, int action, int mods) {
  Game *game = reinterpret_cast<Game *>(glfwGetWindowUserPointer(window));
  game->MouseButtonCallback(button, action, mods);
}

void _CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
  Game *game = reinterpret_cast<Game *>(glfwGetWindowUserPointer(window));
  game->CursorPosCallback(xpos, ypos);
}

Game::Game() {
  // window ------------------------------------
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    std::exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  m_size = {1100, 800};
  // m_size = {600, 400};
  m_window = glfwCreateWindow(m_size.x, m_size.y, "Learn WebGPU", NULL, NULL);
  if (!m_window) {
    std::cerr << "Could not open window!" << std::endl;
    std::exit(1);
  }
  glfwSetWindowUserPointer(m_window, this);

  glfwSetKeyCallback(m_window, _KeyCallback);
  glfwSetMouseButtonCallback(m_window, _MouseButtonCallback);
  glfwSetCursorPosCallback(m_window, _CursorPosCallback);

  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  int FBWidth, FBHeight;
  glfwGetFramebufferSize(m_window, &FBWidth, &FBHeight);
  m_FBSize = {FBWidth, FBHeight};

  double xpos, ypos;
  glfwGetCursorPos(m_window, &xpos, &ypos);
  m_lastMousePos = glm::vec2(xpos, ypos);
  // end window ----------------------------------

  // init wgpu context
  m_ctx = util::Context(m_window, m_FBSize);

  // setup game state
  util::Camera camera(
    &m_ctx,
    glm::vec3(0, 0.0, 105.0),
    glm::vec3(glm::radians(0.0f), 0, 0),
    glm::radians(50.0f),
    (float)m_size.x / m_size.y,
    0.1,
    2000
  );
  m_state.player = game::Player(camera);

  game::InitMesh();
  game::Chunk::InitSharedData();
  m_state.chunkManager = std::make_unique<game::ChunkManager>(&m_ctx);

  // setup rendering
  util::Renderer renderer(&m_ctx, m_FBSize);

  // game loop
  util::Timer timer;

  while (!glfwWindowShouldClose(m_window)) {
    m_dt = timer.Tick();
    double fps = timer.GetFps();
    // print fps to cout
    std::cout << "FPS: " << fps << "\r" << std::flush;

    // error callback
    m_ctx.device.Tick();

    glfwPollEvents();

    // update --------------------------------------------------------
    glm::vec3 moveDir(0);
    if (KeyPressed(GLFW_KEY_W))
      moveDir.y += 1;
    if (KeyPressed(GLFW_KEY_S))
      moveDir.y -= 1;
    if (KeyPressed(GLFW_KEY_D))
      moveDir.x += 1;
    if (KeyPressed(GLFW_KEY_A))
      moveDir.x -= 1;
    if (KeyPressed(GLFW_KEY_SPACE))
      moveDir.z += 1;
    if (KeyPressed(GLFW_KEY_LEFT_SHIFT))
      moveDir.z -= 1;
    m_state.player.Move(moveDir * m_dt);
    m_state.player.Update();

    // m_state.chunkManager.Update(glm::vec2(m_state.player.GetPosition()));

    // render
    renderer.Render(m_state);
    renderer.Present();
  }
}

Game::~Game() {
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void Game::KeyCallback(int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
  }
}

void Game::MouseButtonCallback(int button, int action, int mods) {
  if (action == GLFW_PRESS) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      auto castData = game::Raycast(
        m_state.player.GetPosition(),
        m_state.player.GetDirection(),
        10,
        *m_state.chunkManager
      );
      if (castData.has_value()) {
        auto [pos, dir] = castData.value();
        m_state.chunkManager->SetBlock(pos, game::BlockId::AIR);
      }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      auto castData = game::Raycast(
        m_state.player.GetPosition(),
        m_state.player.GetDirection(),
        10,
        *m_state.chunkManager
      );
      if (castData.has_value()) {
        auto [pos, dir] = castData.value();
        glm::ivec3 placePos = pos + game::g_DIR_OFFSETS[dir];
        m_state.chunkManager->SetBlock(placePos, game::BlockId::DIRT);
      }
    }
  }
}

void Game::CursorPosCallback(double xpos, double ypos) {
  glm::vec2 currMousePos(xpos, ypos);
  glm::vec2 delta = currMousePos - m_lastMousePos;
  m_lastMousePos = currMousePos;
  m_state.player.Look(delta * 0.003f);
}

bool Game::KeyPressed(int key) {
  return glfwGetKey(m_window, key) == GLFW_PRESS;
}

void Game::Run() {
}
