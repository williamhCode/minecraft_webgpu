#include "game.hpp"

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "game/block.hpp"
#include "game/chunk.hpp"
#include "game/mesh.hpp"
#include "util/pipeline/simple.hpp"
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
  glfwSwapInterval(0);
  m_size = {1024, 640};
  m_window = glfwCreateWindow(m_size.x, m_size.y, "Learn WebGPU", NULL, NULL);
  if (!m_window) {
    std::cerr << "Could not open window!" << std::endl;
    std::exit(1);
  }
  glfwSwapInterval(0);

  glfwSetWindowUserPointer(m_window, this);

  glfwSetKeyCallback(m_window, _KeyCallback);
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

  // init objects
  m_camera = util::Camera(
    &m_handle, glm::vec3(0, -20.0, 20.0), glm::vec3(glm::radians(0.0f), 0, 0),
    glm::radians(45.0f), (float)m_size.x / m_size.y, 0.1, 500
  );

  game::InitFaces(); // init mesh faces
  game::InitTextures(m_handle);
  game::Chunk chunk(&m_handle);

  // init pipeline
  auto pipeline = util::CreatePipelineSimple(m_handle);

  // Create bindings -------------------------------------------
  BindGroup bindGroup0;
  {
    BindGroupEntry entry{
      .binding = 0,
      .buffer = m_camera.GetBuffer(),
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
    BindGroupEntry entry{
      .binding = 0,
      .buffer = buffer,
      .size = sizeof(glm::mat4),
    };
    BindGroupDescriptor bindGroupDesc{
      .layout = pipeline.GetBindGroupLayout(1),
      .entryCount = 1,
      .entries = &entry,
    };
    bindGroup1 = m_handle.device.CreateBindGroup(&bindGroupDesc);
  }

  BindGroup bindGroup2;
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
      .layout = pipeline.GetBindGroupLayout(2),
      .entryCount = entries.size(),
      .entries = entries.data(),
    };
    bindGroup2 = m_handle.device.CreateBindGroup(&bindGroupDesc);
  }

  // Create the depth texture -----------------------------------------
  TextureDescriptor depthTextureDesc{
    .usage = TextureUsage::RenderAttachment,
    .size = {m_FBSize.x, m_FBSize.y},
    .format = TextureFormat::Depth24Plus,
  };
  Texture depthTexture = m_handle.device.CreateTexture(&depthTextureDesc);

  TextureViewDescriptor depthTextureViewDesc{};
  TextureView depthTextureView = depthTexture.CreateView(&depthTextureViewDesc);

  // game loop
  double time = glfwGetTime();
  double prev_time = time;

  while (!glfwWindowShouldClose(m_window)) {
    time = glfwGetTime();
    m_dt = time - prev_time;
    prev_time = time;
    // std::cout << m_dt << std::endl;

    m_handle.device.Tick();

    // std::cout << "Position: " << glm::to_string(m_camera.GetPosition()) << "\n";
    // std::cout << "Orentation: " << glm::to_string(m_camera.GetOrientation()) <<
    // "\n\n";

    glfwPollEvents();

    glm::vec3 move_dir(0);
    if (KeyPressed(GLFW_KEY_W))
      move_dir.y += 1;
    if (KeyPressed(GLFW_KEY_S))
      move_dir.y -= 1;
    if (KeyPressed(GLFW_KEY_A))
      move_dir.x -= 1;
    if (KeyPressed(GLFW_KEY_D))
      move_dir.x += 1;
    if (KeyPressed(GLFW_KEY_SPACE))
      move_dir.z += 1;
    if (KeyPressed(GLFW_KEY_LEFT_SHIFT))
      move_dir.z -= 1;
    m_camera.Move(move_dir * 50.0f * m_dt);
    m_camera.Update();

    TextureView nextTexture = m_handle.swapChain.GetCurrentTextureView();
    if (!nextTexture) {
      std::cerr << "Cannot acquire next swap chain texture" << std::endl;
      std::exit(1);
    }

    CommandEncoderDescriptor commandEncoderDesc{};
    CommandEncoder commandEncoder =
      m_handle.device.CreateCommandEncoder(&commandEncoderDesc);

    RenderPassColorAttachment colorAttachment{
      .view = nextTexture,
      .loadOp = LoadOp::Clear,
      .storeOp = StoreOp::Store,
      .clearValue = Color{0.1, 0.1, 0.1, 1.0},
    };
    RenderPassDepthStencilAttachment depthStencilAttachment{
      .view = depthTextureView,
      .depthLoadOp = LoadOp::Clear,
      .depthStoreOp = StoreOp::Store,
      .depthClearValue = 1.0f,
      .depthReadOnly = false,
    };
    RenderPassDescriptor renderPassDesc{
      .colorAttachmentCount = 1,
      .colorAttachments = &colorAttachment,
      .depthStencilAttachment = &depthStencilAttachment,
    };
    RenderPassEncoder passEncoder = commandEncoder.BeginRenderPass(&renderPassDesc);

    passEncoder.SetPipeline(pipeline);

    passEncoder.SetBindGroup(0, bindGroup0);
    passEncoder.SetBindGroup(1, bindGroup1);
    passEncoder.SetBindGroup(2, bindGroup2);

    chunk.Render(passEncoder);

    passEncoder.End();

    CommandBufferDescriptor cmdBufferDescriptor{};
    CommandBuffer command = commandEncoder.Finish(&cmdBufferDescriptor);
    m_handle.queue.Submit(1, &command);

    m_handle.swapChain.Present();
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

void Game::CursorPosCallback(double xpos, double ypos) {
  glm::vec2 currMousePos(xpos, ypos);
  glm::vec2 delta = currMousePos - m_lastMousePos;
  m_lastMousePos = currMousePos;
  m_camera.Look(delta * 0.003f);
}

bool Game::KeyPressed(int key) { return glfwGetKey(m_window, key) == GLFW_PRESS; }

void Game::Run() {}
