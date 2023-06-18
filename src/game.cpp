#include "game.hpp"

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include "util/webgpu-util.hpp"
#include "util/util.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace wgpu;

struct MyUniforms {
  glm::mat4 projectionMatrix;
  glm::mat4 viewMatrix;
  glm::mat4 modelMatrix;
  glm::vec4 color;
  float time;
  float _pad[3];
};

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
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  m_width = 1024;
  m_height = 640;
  this->m_window = glfwCreateWindow(m_width, m_height, "Learn WebGPU", NULL, NULL);
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

  m_handle = util::Handle::init(m_window);

  // shader module
  ShaderModule shaderModule =
    util::loadShaderModule(RESOURCE_DIR "/shader.wgsl", m_handle.device);

  // create render pipeline ------------------------------------------------ //

  // PipelineLayout -----------------------------
  BindGroupLayoutEntry bindingLayout{
    // The binding index as used in the @binding attribute in the shader
    .binding = 0,
    .visibility = ShaderStage::Vertex | ShaderStage::Fragment,
    .buffer{
      .type = BufferBindingType::Uniform,
      .hasDynamicOffset = true,
      .minBindingSize = sizeof(MyUniforms),
    },
  };
  BindGroupLayoutDescriptor bindGroupLayoutDesc{
    .entryCount = 1,
    .entries = &bindingLayout,
  };
  BindGroupLayout bindGroupLayout = m_handle.device.CreateBindGroupLayout(&bindGroupLayoutDesc);
  PipelineLayoutDescriptor layoutDesc{
    .bindGroupLayoutCount = 1,
    .bindGroupLayouts = &bindGroupLayout,
  };
  PipelineLayout pipelineLayout = m_handle.device.CreatePipelineLayout(&layoutDesc);

  // Vertex State ---------------------------------
  std::vector<VertexAttribute> vertexAttribs{
    {
      .format = VertexFormat::Float32x3,
      .offset = 0,
      .shaderLocation = 0,
    },
    {
      .format = VertexFormat::Float32x3,
      .offset = 3 * sizeof(float),
      .shaderLocation = 1,
    },
  };
  VertexBufferLayout vertexBufferLayout{
    .arrayStride = 6 * sizeof(float),
    .stepMode = VertexStepMode::Vertex,
    .attributeCount = static_cast<uint32_t>(vertexAttribs.size()),
    .attributes = vertexAttribs.data(),
  };
  VertexState vertexState{
    .module = shaderModule,
    .entryPoint = "vs_main",
    .bufferCount = 1,
    .buffers = &vertexBufferLayout,
  };

  // Primitve State --------------------------------
  PrimitiveState primitiveState{
    .topology = PrimitiveTopology::TriangleList,
    .stripIndexFormat = IndexFormat::Undefined,
    .frontFace = FrontFace::CCW,
    .cullMode = CullMode::None,
  };

  // Depth Stencil State ---------------------------
  TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
  DepthStencilState depthStencilState{
    .format = depthTextureFormat,
    .depthWriteEnabled = true,
    .depthCompare = CompareFunction::Less,
    .stencilReadMask = 0,
    .stencilWriteMask = 0,
  };

  // Fragment State --------------------------------
  BlendState blendState{
    .color{
      .operation = BlendOperation::Add,
      .srcFactor = BlendFactor::SrcAlpha,
      .dstFactor = BlendFactor::OneMinusSrcAlpha,
    },
    .alpha{
      .operation = BlendOperation::Add,
      .srcFactor = BlendFactor::Zero,
      .dstFactor = BlendFactor::One,
    },
  };
  ColorTargetState colorTarget{
    .format = m_handle.swapChainFormat,
    .blend = &blendState,
    .writeMask = ColorWriteMask::All,
  };
  FragmentState fragmentState{
    .module = shaderModule,
    .entryPoint = "fs_main",
    .targetCount = 1,
    .targets = &colorTarget,
  };

  // finally, create Render Pipeline
  RenderPipelineDescriptor pipelineDesc{
    .layout = pipelineLayout,
    .vertex = vertexState,
    .primitive = primitiveState,
    .depthStencil = &depthStencilState,
    .fragment = &fragmentState,
  };
  RenderPipeline pipeline = m_handle.device.CreateRenderPipeline(&pipelineDesc);

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

  // Create uniform buffer
  uint32_t uniformStride = std::max(
    (uint32_t)sizeof(MyUniforms),
    (uint32_t)m_handle.deviceLimits.minUniformBufferOffsetAlignment);
  bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
  bufferDesc.size = sizeof(MyUniforms);
  // bufferDesc.size = uniformStride + sizeof(MyUniforms);
  Buffer uniformBuffer = m_handle.device.CreateBuffer(&bufferDesc);

  m_camera = util::Camera(
    glm::vec3(0, -4.0, 3.0),
    glm::vec3(-0.5, 0, 0),
    glm::radians(45.0f),
    (float)m_width / m_height,
    0.1,
    100);

  MyUniforms uniforms{
    .projectionMatrix = m_camera.getProjection(),
    .viewMatrix = m_camera.getView(),
    .modelMatrix = glm::mat4(1.0),
    .color = {0.0f, 1.0f, 0.4f, 1.0f},
    .time = 0.0f,
  };
  m_handle.queue.WriteBuffer(uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

  // Create a binding
  BindGroupEntry binding{
    .binding = 0,
    .buffer = uniformBuffer,
    .offset = 0,
    .size = sizeof(MyUniforms),
  };
  // A bind group contains one or multiple bindings
  BindGroupDescriptor bindGroupDesc{
    // .layout = bindGroupLayout,
    .layout = pipeline.GetBindGroupLayout(0),
    // There must be as many bindings as declared in the layout!
    // .entryCount = bindGroupLayoutDesc.entryCount,
    .entryCount = 1,
    .entries = &binding,
  };
  BindGroup bindGroup = m_handle.device.CreateBindGroup(&bindGroupDesc);

  // Create the depth texture
  TextureDescriptor depthTextureDesc{
    .usage = TextureUsage::RenderAttachment,
    .size = {static_cast<uint32_t>(m_FBWidth), static_cast<uint32_t>(m_FBHeight)},
    .format = depthTextureFormat,
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

    uniforms.viewMatrix = m_camera.getView();
    m_handle.queue.WriteBuffer(
      uniformBuffer,
      offsetof(MyUniforms, viewMatrix),
      &uniforms.viewMatrix,
      sizeof(MyUniforms::viewMatrix));

    TextureView nextTexture = m_handle.swapChain.GetCurrentTextureView();
    if (!nextTexture) {
      std::cerr << "Cannot acquire next swap chain texture" << std::endl;
      std::exit(1);
    }

    CommandEncoderDescriptor commandEncoderDesc{.label = "Render encoder"};
    CommandEncoder commandEncoder = m_handle.device.CreateCommandEncoder(&commandEncoderDesc);

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
      indexBuffer, IndexFormat::Uint16, 0, indexBuffer.GetSize());

    uint32_t dynamicOffset = 0;

    // Set binding group
    // dynamicOffset = 0 * uniformStride;
    passEncoder.SetBindGroup(0, bindGroup, 1, &dynamicOffset);
    passEncoder.DrawIndexed(indexCount, 1, 0, 0, 0);

    // Set binding group with a different uniform offset
    // dynamicOffset = 1 * uniformStride;
    // renderPass.SetBindGroup(0, bindGroup, 1, &dynamicOffset);
    // renderPass.DrawIndexed(indexCount, 1, 0, 0, 0);

    passEncoder.End();

    CommandBufferDescriptor cmdBufferDescriptor{.label = "Command buffer"};
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
