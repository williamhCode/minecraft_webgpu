#include "game.hpp"

#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>

#include "game/block.hpp"
#include "game/chunk.hpp"
#include "game/chunk_manager.hpp"
#include "game/direction.hpp"
#include "game/mesh.hpp"
#include "game/ray.hpp"
#include "glm/gtx/string_cast.hpp"
#include "gfx/context.hpp"
#include "gfx/renderer.hpp"
#include "util/timer.hpp"
#include "gfx/pipeline.hpp"

#include <iostream>

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
  // glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

  // GLFWmonitor *primary = glfwGetPrimaryMonitor();
  // const GLFWvidmode *mode = glfwGetVideoMode(primary);
  // glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  // glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  // glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  // glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
  // m_state.size = {2560, 1440};

  GLFWmonitor* primary = NULL;
  // m_state.size = {960, 540};
  m_state.size = {1200, 800};
  // m_state.size = {1400, 1000};

  m_window =
    glfwCreateWindow(m_state.size.x, m_state.size.y, "Learn WebGPU", primary, NULL);
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
  // m_state.fbSize = {FBWidth, FBHeight};
  m_state.fb_size = {FBWidth, FBHeight};

  std::cout << "Window size: " << glm::to_string(m_state.size) << std::endl;
  std::cout << "Framebuffer size: " << glm::to_string(m_state.fb_size) << std::endl;

  double xpos, ypos;
  glfwGetCursorPos(m_window, &xpos, &ypos);
  m_lastMousePos = glm::vec2(xpos, ypos);
  // end window ----------------------------------

  // init wgpu context
  m_ctx = gfx::Context(m_window, m_state.fb_size);

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
    glm::radians(50.0f), (float)m_state.size.x / m_state.size.y, 0.1, 2000
  );
  m_state.player = game::Player(camera);

  game::InitMesh();
  game::Chunk::InitSharedData();
  // initialize in same memory to avoid changing of memory address, because it passes its own pointer to members when initializing
  new (&m_state.chunkManager) game::ChunkManager(&m_ctx, &m_state);

  // auto sunDir = glm::normalize(glm::vec3(1, 1, 1));
  auto sunRiseTurn = glm::vec2(45, 0);
  m_state.sun = gfx::Sun(&m_ctx, &m_state, sunRiseTurn);

  m_state.currBlock = game::BlockId::Dirt;
  // setup rendering
  gfx::Renderer renderer(&m_ctx, &m_state);

  // game loop
  util::Timer timer;

  while (!glfwWindowShouldClose(m_window)) {
    m_state.dt = timer.Tick();
    m_state.fps = timer.GetFps();

    // error callback
    m_ctx.device.Tick();

    glfwPollEvents();

    // update --------------------------------------------------------
    if (m_state.focused) {
      glm::vec3 moveDir(0);
      if (KeyPressed(GLFW_KEY_W)) moveDir.x += 1;
      if (KeyPressed(GLFW_KEY_S)) moveDir.x -= 1;
      if (KeyPressed(GLFW_KEY_A)) moveDir.y += 1;
      if (KeyPressed(GLFW_KEY_D)) moveDir.y -= 1;
      if (KeyPressed(GLFW_KEY_SPACE)) moveDir.z += 1;
      if (KeyPressed(GLFW_KEY_LEFT_SHIFT)) moveDir.z -= 1;
      m_state.player.Move(moveDir * m_state.dt);
    }
    m_state.player.Update();

    m_state.sun.Update();
    m_state.chunkManager.Update(glm::vec2(m_state.player.GetPosition()));

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
    // any ctrl key
    if (mods & GLFW_MOD_CONTROL) {
      if (m_state.player.speed == 10)
        m_state.player.speed = 100;
      else
        m_state.player.speed = 10;
    }

    for (auto blockId = game::BlockId::Air; blockId < game::BlockId::Last;
         blockId = (game::BlockId)((int)blockId + 1)) {
      auto num = -1 + (int)blockId;
      if (key == GLFW_KEY_0 + num && num < 10) m_state.currBlock = blockId;
    }
  }
}

void Game::MouseButtonCallback(int button, int action, int mods) {
  if (!m_state.focused) return;

  if (action == GLFW_PRESS) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      auto castData = game::Raycast(
        m_state.player.GetPosition(), m_state.player.GetDirection(), 10,
        m_state.chunkManager
      );
      if (castData) {
        auto [pos, dir] = *castData;
        m_state.chunkManager.SetBlockAndUpdate(pos, game::BlockId::Air);
        m_state.sun.InvokeUpdate();
      }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      auto castData = game::Raycast(
        m_state.player.GetPosition(), m_state.player.GetDirection(), 10,
        m_state.chunkManager
      );
      if (castData) {
        auto [pos, dir] = *castData;
        glm::ivec3 placePos = pos + game::g_DIR_OFFSETS[dir];
        m_state.chunkManager.SetBlockAndUpdate(placePos, m_state.currBlock);
        m_state.sun.InvokeUpdate();
      }
    }
  }
}

void Game::CursorPosCallback(double xpos, double ypos) {
  glm::vec2 currMousePos(xpos, ypos);
  glm::vec2 delta = currMousePos - m_lastMousePos;
  m_lastMousePos = currMousePos;
  if (!m_state.focused) return;
  m_state.player.Look(delta * 0.003f);
}

bool Game::KeyPressed(int key) {
  return glfwGetKey(m_window, key) == GLFW_PRESS;
}
