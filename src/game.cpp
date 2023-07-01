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
#include "util/pipeline/simple.hpp"
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

  // init wgpu
  m_handle = util::Handle::Init(m_window);

  // init pipeline
  auto pipeline = util::CreatePipelineSimple(m_handle);

  // init objects
  util::Camera camera(
    &m_handle,
    glm::vec3(0, 0.0, 105.0),
    glm::vec3(glm::radians(0.0f), 0, 0),
    glm::radians(45.0f),
    (float)m_size.x / m_size.y,
    0.1,
    2000
  );

  m_player = game::Player(camera);

  game::InitMesh();
  game::InitTextures(m_handle);
  game::Chunk::InitSharedData();

  BindGroupLayout layout2 = pipeline.GetBindGroupLayout(2);
  m_chunkManager = std::make_unique<game::ChunkManager>(&m_handle, layout2);

  // Create bindings -------------------------------------------
  BindGroup bindGroup0;
  {
    BindGroupEntry entry{
      .binding = 0,
      .buffer = m_player.camera.uniformBuffer,
      .size = sizeof(glm::mat4),
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = pipeline.GetBindGroupLayout(0),
      .entryCount = 1,
      .entries = &entry,
    };
    bindGroup0 = m_handle.device.CreateBindGroup(&bindGroupDesc);
  }

  glm::mat4 modelMatix(1.0);
  BufferDescriptor bufferDesc{
    .usage = BufferUsage::CopyDst | BufferUsage::Uniform,
    .size = sizeof(glm::mat4),
  };
  Buffer buffer = m_handle.device.CreateBuffer(&bufferDesc);
  m_handle.queue.WriteBuffer(buffer, 0, &modelMatix, sizeof(modelMatix));

  BindGroup bindGroup1;
  {
    std::vector<BindGroupEntry> entries{
      BindGroupEntry{
        .binding = 0,
        .textureView = game::g_blocksTextureView,
      },
      BindGroupEntry{
        .binding = 1,
        .sampler = game::g_blocksSampler,
      },
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = pipeline.GetBindGroupLayout(1),
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bindGroup1 = m_handle.device.CreateBindGroup(&bindGroupDesc);
  }

  // setup rendering
  util::Renderer renderer(&m_handle, m_FBSize);

  // game loop
  util::Timer timer;

  while (!glfwWindowShouldClose(m_window)) {
    m_dt = timer.Tick();
    double fps = timer.GetFps();
    // print fps to cout
    // std::cout << "FPS: " << fps << "\r" << std::flush;

    // error callback
    m_handle.device.Tick();

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
    m_player.Move(moveDir * m_dt);
    m_player.Update();

    // m_chunkManager.Update(glm::vec2(m_player.GetPosition()));

    // begin render --------------------------------------------------------
    RenderPassEncoder passEncoder = renderer.Begin({0.5, 0.8, 0.9, 1.0});
    passEncoder.SetPipeline(pipeline);

    passEncoder.SetBindGroup(0, bindGroup0);
    passEncoder.SetBindGroup(1, bindGroup1);

    m_chunkManager->Render(passEncoder);

    // end render -----------------------------------------------------------
    renderer.End();
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
        m_player.GetPosition(), m_player.GetDirection(), 10, *m_chunkManager
      );
      if (castData.has_value()) {
        auto [pos, dir] = castData.value();
        m_chunkManager->SetBlock(pos, game::BlockId::AIR);
      }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      auto castData = game::Raycast(
        m_player.GetPosition(), m_player.GetDirection(), 10, *m_chunkManager
      );
      if (castData.has_value()) {
        auto [pos, dir] = castData.value();
        glm::ivec3 placePos = pos + game::g_DIR_OFFSETS[dir];
        m_chunkManager->SetBlock(placePos, game::BlockId::DIRT);
      }
    }
  }
}

void Game::CursorPosCallback(double xpos, double ypos) {
  glm::vec2 currMousePos(xpos, ypos);
  glm::vec2 delta = currMousePos - m_lastMousePos;
  m_lastMousePos = currMousePos;
  m_player.Look(delta * 0.003f);
}

bool Game::KeyPressed(int key) {
  return glfwGetKey(m_window, key) == GLFW_PRESS;
}

void Game::Run() {
}
