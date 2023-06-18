#include "game.hpp"

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "util/webgpu-util.hpp"
#include "util/util.hpp"
#include "pipeline/simple.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace wgpu;

void _keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
  Game *game = reinterpret_cast<Game *>(glfwGetWindowUserPointer(window));
  game->keyCallback(key, scancode, action, mods);
}

void _cursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
  Game *game = reinterpret_cast<Game *>(glfwGetWindowUserPointer(window));
  game->cursorPosCallback(xpos, ypos);
}

Game::Game() {
  // window
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    std::exit(1);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  m_width = 1024;
  m_height = 640;
  m_window = glfwCreateWindow(m_width, m_height, "Learn WebGPU", NULL, NULL);
  if (!m_window) {
    std::cerr << "Could not open window!" << std::endl;
    std::exit(1);
  }
  glfwSetWindowUserPointer(m_window, this);
  // callbacks
  glfwSetKeyCallback(m_window, _keyCallback);
  glfwSetCursorPosCallback(m_window, _cursorPosCallback);

  glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glfwGetFramebufferSize(m_window, &m_FBWidth, &m_FBHeight);
  double xpos, ypos;
  glfwGetCursorPos(m_window, &xpos, &ypos);
  m_lastMousePos = glm::vec2(xpos, ypos);

  // init wgpu
  m_handle = util::Handle::init(m_window);

  // shader module
  auto pipeline = createPipeline_simple(m_handle);

  // data
  std::vector<float> pointData;
  std::vector<uint16_t> indexData;

  bool success = loadGeometry(RESOURCE_DIR "/pyramid.txt", pointData, indexData, 3);
  if (!success) {
    std::cerr << "Could not load geometry!" << std::endl;
    std::exit(1);
  }

  int indexCount = static_cast<int>(indexData.size());

  // Create vertex buffer
  BufferDescriptor bufferDesc{
    .usage = BufferUsage::CopyDst | BufferUsage::Vertex,
    .size = pointData.size() * sizeof(float),
  };
  Buffer vertexBuffer = m_handle.device.CreateBuffer(&bufferDesc);
  m_handle.queue.WriteBuffer(vertexBuffer, 0, pointData.data(), bufferDesc.size);

  // Create index buffer
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
  bufferDesc.size = indexData.size() * sizeof(float);
  Buffer indexBuffer = m_handle.device.CreateBuffer(&bufferDesc);
  m_handle.queue.WriteBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);

  // Camera
  m_camera = util::Camera(
    glm::vec3(0, -4.0, 3.0),
    glm::vec3(-0.5, 0, 0),
    glm::radians(45.0f),
    (float)m_width / m_height,
    0.1,
    100
  );

  // Create uniform buffers
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  bufferDesc.size = sizeof(glm::mat4);
  Buffer uniformBuffer0 = m_handle.device.CreateBuffer(&bufferDesc);
  Buffer uniformBuffer1 = m_handle.device.CreateBuffer(&bufferDesc);
  glm::mat4 model(1.0);
  m_handle.queue.WriteBuffer(uniformBuffer1, 0, &model, sizeof(model));

  // Create bindings
  BindGroup bindGroup0;
  {
    BindGroupEntry binding{
      .binding = 0,
      .buffer = uniformBuffer0,
      .size = sizeof(glm::mat4),
    };
    // A bind group contains one or multiple bindings
    BindGroupDescriptor bindGroupDesc{
      .layout = pipeline.GetBindGroupLayout(0),
      .entryCount = 1,
      .entries = &binding,
    };
    bindGroup0 = m_handle.device.CreateBindGroup(&bindGroupDesc);
  }
  BindGroup bindGroup1;
  {
    BindGroupEntry binding{
      .binding = 0,
      .buffer = uniformBuffer1,
      .size = sizeof(glm::mat4),
    };
    // A bind group contains one or multiple bindings
    BindGroupDescriptor bindGroupDesc{
      .layout = pipeline.GetBindGroupLayout(1),
      .entryCount = 1,
      .entries = &binding,
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

  double time = glfwGetTime();
  double prev_time = time;

  while (!glfwWindowShouldClose(m_window)) {
    time = glfwGetTime();
    m_dt = time - prev_time;
    prev_time = time;
    // std::cout << dt << std::endl;

    glfwPollEvents();

    glm::vec3 move_dir(0);
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
      move_dir.y += 5;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
      move_dir.y -= 5;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
      move_dir.x += 5;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
      move_dir.x -= 5;
    if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
      move_dir.z += 5;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
      move_dir.z -= 5;
    m_camera.move(move_dir * m_dt);
    m_camera.update();

    auto viewProj = m_camera.getViewProj();
    m_handle.queue.WriteBuffer(uniformBuffer0, 0, &viewProj, sizeof(viewProj));

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

    passEncoder.SetVertexBuffer(0, vertexBuffer, 0, vertexBuffer.GetSize());
    passEncoder.SetIndexBuffer(
      indexBuffer, IndexFormat::Uint16, 0, indexBuffer.GetSize()
    );

    passEncoder.SetBindGroup(0, bindGroup0);
    passEncoder.SetBindGroup(1, bindGroup1);
    passEncoder.DrawIndexed(indexCount);

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

void Game::keyCallback(int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(m_window, GLFW_TRUE);
  }
}

void Game::cursorPosCallback(double xpos, double ypos) {
  glm::vec2 currMousePos(xpos, ypos);
  glm::vec2 delta = currMousePos - m_lastMousePos;
  m_lastMousePos = currMousePos;
  m_camera.look(delta * 0.003f);
}

void Game::run() {}
