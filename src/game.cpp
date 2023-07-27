#include "game.hpp"

#include <numeric>
#include <tuple>

#include "GLFW/glfw3.h"
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_wgpu.h"

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
  // disable high-dpi for macOS
  glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

  m_state.size = {1100, 800};
  m_window =
    glfwCreateWindow(m_state.size.x, m_state.size.y, "Learn WebGPU", NULL, NULL);
  if (!m_window) {
    std::cerr << "Could not open window!" << std::endl;
    std::exit(1);
  }

  // fix bug with high_dpi=False not setting the correct framebuffer size
  // glfwPollEvents();

  glfwSetWindowUserPointer(m_window, this);

  glfwSetKeyCallback(m_window, _KeyCallback);
  glfwSetMouseButtonCallback(m_window, _MouseButtonCallback);
  glfwSetCursorPosCallback(m_window, _CursorPosCallback);

  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  m_state.focused = true;

  int FBWidth, FBHeight;
  glfwGetFramebufferSize(m_window, &FBWidth, &FBHeight);
  m_state.fbSize = {FBWidth, FBHeight};

  double xpos, ypos;
  glfwGetCursorPos(m_window, &xpos, &ypos);
  m_lastMousePos = glm::vec2(xpos, ypos);
  // end window ----------------------------------

  // init wgpu context
  m_ctx = util::Context(m_window, m_state.fbSize);

  // setup imgui -----------------------------------------
  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  ImGui_ImplGlfw_InitForOther(m_window, true);
  ImGui_ImplWGPU_Init(
    m_ctx.device.Get(), 1, (WGPUTextureFormat)m_ctx.swapChainFormat,
    WGPUTextureFormat_Undefined
  );
  // end imgui ----------------------------------------------

  // setup game state
  util::Camera camera(
    &m_ctx, glm::vec3(0, 0.0, 105.0), glm::vec3(glm::radians(0.0f), 0, 0),
    glm::radians(50.0f), (float)m_state.size.x / m_state.size.y, 0.1, 1000
  );
  m_state.player = game::Player(camera);

  game::InitMesh();
  game::Chunk::InitSharedData();
  m_state.chunkManager = std::make_unique<game::ChunkManager>(&m_ctx);

  // setup rendering
  util::Renderer renderer(&m_ctx, &m_state);

  // game loop
  util::Timer timer;
  // float time = 0;

  while (!glfwWindowShouldClose(m_window)) {
    m_state.dt = timer.Tick();
    m_state.fps = timer.GetFps();

    // error callback
    m_ctx.device.Tick();

    glfwPollEvents();

    // update --------------------------------------------------------
    if (m_state.focused) {
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
      m_state.player.Move(moveDir * m_state.dt);
    }
    m_state.player.Update();

    m_state.chunkManager->Update(glm::vec2(m_state.player.GetPosition()));

    // render
    renderer.Render();
    renderer.Present();
  }
}

Game::~Game() {
  glfwDestroyWindow(m_window);
  glfwTerminate();
}

void Game::KeyCallback(int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_ESCAPE) {
      if (m_state.focused) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_state.focused = false;
      } else {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        m_state.focused = true;
      }
    }
    if (key == GLFW_KEY_Q) {
      glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_G) {
      m_state.showStats = !m_state.showStats;
    }
  }
}

void Game::MouseButtonCallback(int button, int action, int mods) {
  if (!m_state.focused)
    return;

  if (action == GLFW_PRESS) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      auto castData = game::Raycast(
        m_state.player.GetPosition(), m_state.player.GetDirection(), 10,
        *m_state.chunkManager
      );
      if (castData) {
        auto [pos, dir] = *castData;
        m_state.chunkManager->SetBlock(pos, game::BlockId::Air);
      }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      auto castData = game::Raycast(
        m_state.player.GetPosition(), m_state.player.GetDirection(), 10,
        *m_state.chunkManager
      );
      if (castData) {
        auto [pos, dir] = *castData;
        glm::ivec3 placePos = pos + game::g_DIR_OFFSETS[dir];
        m_state.chunkManager->SetBlock(placePos, game::BlockId::Grass);
      }
    }
  }
}

void Game::CursorPosCallback(double xpos, double ypos) {
  glm::vec2 currMousePos(xpos, ypos);
  glm::vec2 delta = currMousePos - m_lastMousePos;
  m_lastMousePos = currMousePos;
  if (!m_state.focused)
    return;
  m_state.player.Look(delta * 0.003f);
}

bool Game::KeyPressed(int key) {
  return glfwGetKey(m_window, key) == GLFW_PRESS;
}

void Game::Run() {
}
