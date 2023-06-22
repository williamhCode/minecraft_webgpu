#include "game.hpp"

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "game/chunk.hpp"
#include "game/mesh.hpp"
#include "util/webgpu-util.hpp"
#include "util/load.hpp"
#include "pipeline/simple.hpp"

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
  m_width = 1024;
  m_height = 640;
  m_window = glfwCreateWindow(m_width, m_height, "Learn WebGPU", NULL, NULL);
  if (!m_window) {
    std::cerr << "Could not open window!" << std::endl;
    std::exit(1);
  }

  glfwSetWindowUserPointer(m_window, this);

  glfwSetKeyCallback(m_window, _KeyCallback);
  glfwSetCursorPosCallback(m_window, _CursorPosCallback);

  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glfwGetFramebufferSize(m_window, &m_FBWidth, &m_FBHeight);
  double xpos, ypos;
  glfwGetCursorPos(m_window, &xpos, &ypos);
  m_lastMousePos = glm::vec2(xpos, ypos);
  // end window ----------------------------------

  // init wgpu
  m_handle = util::Handle::Init(m_window);

  // init objects
  m_camera = util::Camera(
    &m_handle,
    glm::vec3(0, -20.0, 20.0),
    glm::vec3(glm::radians(0.0f), 0, 0),
    glm::radians(45.0f),
    (float)m_width / m_height,
    0.1,
    100
  );

  game::InitFaces();  // init mesh faces
  game::Chunk chunk(&m_handle);

  // init pipeline
  RenderPipeline pipeline = CreatePipeline_simple(m_handle);

  // Create bindings
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


  // Create the depth texture
  TextureDescriptor depthTextureDesc{
    .usage = TextureUsage::RenderAttachment,
    .size = {static_cast<uint32_t>(m_FBWidth), static_cast<uint32_t>(m_FBHeight)},
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
    // std::cout << dt << std::endl;

    m_handle.device.Tick();

    // std::cout << "Position: " << glm::to_string(m_camera.GetPosition()) << "\n";
    // std::cout << "Orentation: " << glm::to_string(m_camera.GetOrientation()) << "\n\n";

    glfwPollEvents();

    glm::vec3 move_dir(0);
    float speed = 10;
    if (KeyPressed(GLFW_KEY_W))
      move_dir.y += speed;
    if (KeyPressed(GLFW_KEY_S))
      move_dir.y -= speed;
    if (KeyPressed(GLFW_KEY_A))
      move_dir.x -= speed;
    if (KeyPressed(GLFW_KEY_D))
      move_dir.x += speed;
    if (KeyPressed(GLFW_KEY_SPACE))
      move_dir.z += speed;
    if (KeyPressed(GLFW_KEY_LEFT_SHIFT))
      move_dir.z -= speed;
    m_camera.Move(move_dir * m_dt);
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
